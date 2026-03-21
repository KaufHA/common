#include "kauf_esp8266_update.h"

#include "esphome/core/application.h"
#include "esphome/core/version.h"

#include "esphome/components/json/json_util.h"
#include "esphome/components/network/util.h"

namespace esphome::kauf_esp8266_update {

using http_request::http_read_fully;
using http_request::HttpReadStatus;
using http_request::HTTP_STATUS_OK;

static const char *const TAG = "kauf_esp8266_update";

// Wraps UpdateInfo + error for the task→main-loop handoff.
struct TaskResult {
  update::UpdateInfo info;
  const LogString *error_str{nullptr};
};

static const size_t MAX_READ_SIZE = 256;
static constexpr uint32_t INITIAL_CHECK_INTERVAL_ID = 0;
static constexpr uint32_t INITIAL_CHECK_INTERVAL_MS = 10000;
static constexpr uint8_t INITIAL_CHECK_MAX_ATTEMPTS = 6;
static constexpr uint32_t RETRY_INTERVAL_MS = 10 * 60 * 1000; // minutes * seconds * ms
static constexpr uint8_t MAX_RETRIES = 3;
static constexpr uint32_t MIN_INTERVAL_FOR_RETRY_MS = 24 * 60 * 60 * 1000UL; // hours * minutes * seconds * ms

void KaufEsp8266Update::setup() {
  // Check periodically until network is ready
  // Only if update interval is > total retry window to avoid redundant checks
  if (this->get_update_interval() != SCHEDULER_DONT_RUN &&
      this->get_update_interval() > INITIAL_CHECK_INTERVAL_MS * INITIAL_CHECK_MAX_ATTEMPTS) {
    this->initial_check_remaining_ = INITIAL_CHECK_MAX_ATTEMPTS;
    this->set_interval(INITIAL_CHECK_INTERVAL_ID, INITIAL_CHECK_INTERVAL_MS, [this]() {
      bool connected = network::is_connected();
      if (--this->initial_check_remaining_ == 0 || connected) {
        this->cancel_interval(INITIAL_CHECK_INTERVAL_ID);
        if (connected) {
          this->update();
        }
      }
    });
  }
}

void KaufEsp8266Update::update() {
  if (!network::is_connected()) {
    ESP_LOGD(TAG, "Network not connected, skipping update check");
    return;
  }
  // Cancel early-boot interval (no-op if already cancelled by the interval callback itself).
  // Must be after the network check so a boot-time call with no network doesn't kill the interval.
  this->cancel_interval(INITIAL_CHECK_INTERVAL_ID);
  this->cancel_timeout("retry");
  this->update_task(this);
}

void KaufEsp8266Update::update_task(void *params) {
  KaufEsp8266Update *this_update = (KaufEsp8266Update *) params;

  // Allocate once — every path below returns via the single defer at the end.
  // On failure, error_str is set; on success it is nullptr.
  auto *result = new TaskResult();
  auto *info = &result->info;

  ESP_LOGD(TAG, "Checking for update at %s", this_update->source_url_);
  auto container = this_update->request_parent_->get(this_update->source_url_);

  if (container == nullptr || container->status_code != HTTP_STATUS_OK) {
    ESP_LOGE(TAG, "Failed to fetch manifest from %s", this_update->source_url_);
    if (container != nullptr)
      container->end();
    result->error_str = LOG_STR("Failed to fetch manifest");
    goto defer;  // NOLINT(cppcoreguidelines-avoid-goto)
  }

  {
    RAMAllocator<uint8_t> allocator;
    uint8_t *data = allocator.allocate(container->content_length);
    if (data == nullptr) {
      ESP_LOGE(TAG, "Failed to allocate %zu bytes for manifest", container->content_length);
      container->end();
      result->error_str = LOG_STR("Failed to allocate memory for manifest");
      goto defer;  // NOLINT(cppcoreguidelines-avoid-goto)
    }

    auto read_result = http_read_fully(container.get(), data, container->content_length, MAX_READ_SIZE,
                                       this_update->request_parent_->get_timeout());
    if (read_result.status != HttpReadStatus::OK) {
      if (read_result.status == HttpReadStatus::TIMEOUT) {
        ESP_LOGE(TAG, "Timeout reading manifest");
      } else {
        ESP_LOGE(TAG, "Error reading manifest: %d", read_result.error_code);
      }
      allocator.deallocate(data, container->content_length);
      container->end();
      result->error_str = LOG_STR("Failed to read manifest");
      goto defer;  // NOLINT(cppcoreguidelines-avoid-goto)
    }
    size_t read_index = container->get_bytes_read();
    size_t content_length = container->content_length;

    container->end();
    container.reset();  // Release ownership of the container's shared_ptr

    bool valid = false;
    {  // Scope to ensure JsonDocument is destroyed before deallocating buffer
      valid = json::parse_json(data, read_index, [info](JsonObject root) -> bool {
        if (!root[ESPHOME_F("name")].is<const char *>() || !root[ESPHOME_F("version")].is<const char *>()) {
          ESP_LOGE(TAG, "Manifest does not contain required fields");
          return false;
        }
        info->title = root[ESPHOME_F("name")].as<std::string>();
        info->latest_version = root[ESPHOME_F("version")].as<std::string>();
        return true;
      });
    }
    allocator.deallocate(data, content_length);

    if (!valid) {
      ESP_LOGE(TAG, "Failed to parse JSON from %s", this_update->source_url_);
      result->error_str = LOG_STR("Failed to parse manifest JSON");
      goto defer;  // NOLINT(cppcoreguidelines-avoid-goto)
    }

#ifdef ESPHOME_PROJECT_VERSION
    info->current_version = ESPHOME_PROJECT_VERSION;
#else
    info->current_version = ESPHOME_VERSION;
#endif
    // Strip trailing non-digit suffix (e.g. "1.993u" -> "1.993", "1.982(y)" -> "1.982")
    while (!info->current_version.empty() && !std::isdigit((unsigned char) info->current_version.back()))
      info->current_version.pop_back();
    ESP_LOGD(TAG, "Manifest version: %s, current version: %s", info->latest_version.c_str(), info->current_version.c_str());

    // Both URLs are hardcoded in firmware — not read from manifest to prevent MITM spoofing.
    // release_url → GitHub releases page, used by HA as "release notes" link.
    // firmware_url → direct .bin.gz download with {version} substitution, used by web UI.
    info->summary = "The Update button below does not work for this device. To update:\n\n"
                    "1. Click the three-dot menu in the top-right of this popup\n"
                    "2. Select **Device Info**\n"
                    "3. Click **Visit** to open the device web UI\n\n"
                    "Under the **OTA Update** heading in the web UI, there is a link to download the firmware file and a file upload field to apply it. Note: you must be on the same local network as the device to access the web UI.";
    info->release_url = this_update->release_url_;
    if (this_update->firmware_url_ != nullptr) {
      std::string url = this_update->firmware_url_;
      size_t pos = url.find("{version}");
      if (pos != std::string::npos)
        url.replace(pos, 9, info->latest_version);
      info->firmware_url = std::move(url);
    }
  }

defer:
  // Release container before vTaskDelete (which doesn't call destructors)
  container.reset();

  // Defer to the main loop so all update_info_ and state_ writes happen on the
  // same thread as readers (API, MQTT, web server). This is a single defer for
  // both success and error paths to avoid multiple std::function instantiations.
  // Lambda captures only 2 pointers (8 bytes) — fits in std::function SBO on supported toolchains.
  this_update->defer([this_update, result]() {
    if (result->error_str != nullptr) {
      this_update->status_set_error(result->error_str);
      delete result;
      if (this_update->get_update_interval() >= MIN_INTERVAL_FOR_RETRY_MS &&
          this_update->retry_count_ < MAX_RETRIES) {
        this_update->retry_count_++;
        ESP_LOGD(TAG, "Scheduling retry %d/%d in 10 minutes", this_update->retry_count_, MAX_RETRIES);
        this_update->set_timeout("retry", RETRY_INTERVAL_MS, [this_update]() { this_update->update(); });
      }
      return;
    }
    this_update->retry_count_ = 0;

    const std::string &latest = result->info.latest_version;
    const std::string &current = result->info.current_version;

    // Parse "major.minor" version strings and compare numerically.
    // current >= latest means no update needed (handles device ahead of manifest).
    auto parse_version = [](const std::string &v, int &major, int &minor) {
      const char *s = v.c_str();
      major = atoi(s);
      const char *dot = strchr(s, '.');
      minor = dot ? atoi(dot + 1) : 0;
    };
    int cur_maj, cur_min, lat_maj, lat_min;
    parse_version(current, cur_maj, cur_min);
    parse_version(latest, lat_maj, lat_min);
    bool up_to_date = latest.empty() ||
                      (cur_maj > lat_maj) ||
                      (cur_maj == lat_maj && cur_min >= lat_min);

    bool trigger_update_available = false;
    update::UpdateState new_state;
    if (up_to_date) {
      ESP_LOGD(TAG, "Firmware is up to date");
      new_state = update::UPDATE_STATE_NO_UPDATE;
    } else {
      ESP_LOGD(TAG, "Update available: %s -> %s, release URL: %s",
               result->info.current_version.c_str(), result->info.latest_version.c_str(),
               result->info.release_url.c_str());
      new_state = update::UPDATE_STATE_AVAILABLE;
      if (this_update->state_ != update::UPDATE_STATE_AVAILABLE) {
        trigger_update_available = true;
      }
    }

    this_update->update_info_ = std::move(result->info);
    this_update->state_ = new_state;
    delete result;  // Safe: moved-from state is valid for destruction

    this_update->status_clear_error();
    this_update->publish_state();

    if (trigger_update_available) {
      this_update->get_update_available_trigger()->trigger(this_update->update_info_);
    }
  });
}

void KaufEsp8266Update::perform(bool force) {
  // OTA flashing is intentionally not supported — this component is detect-and-alert only.
  const std::string &fw_url = this->update_info_.firmware_url;
  ESP_LOGW(TAG, "Automatic OTA install by Update component is not supported. Download firmware manually from: %s",
           fw_url.empty() ? this->release_url_ : fw_url.c_str());
}

}  // namespace esphome::kauf_esp8266_update
