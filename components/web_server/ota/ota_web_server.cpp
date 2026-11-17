#include "ota_web_server.h"
#ifdef USE_WEBSERVER_OTA

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/progmem.h"

#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif

// KAUF: added to compare strings in filename
#include <string>
#include <algorithm>
#ifdef USE_ESP8266
#include <StreamString.h>
#endif

namespace esphome::web_server {

ESPHOME_LOG_TAG(TAG, "web_server.ota");

bool WebServerOTAComponent::canHandle(AsyncWebServerRequest *request) const {
  if (request->method() != HTTP_POST)
    return false;
  // Check if this is an OTA update request
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  bool is_ota_request = request->url_to(url_buf) == "/update";
#else
  bool is_ota_request = request->url() == ESPHOME_F("/update");
#endif

#if defined(USE_WEBSERVER_OTA_DISABLED) && defined(USE_CAPTIVE_PORTAL)
  // IMPORTANT: USE_WEBSERVER_OTA_DISABLED only disables OTA for the web_server component
  // Captive portal can still perform OTA updates - check if request is from active captive portal
  // Note: global_captive_portal is the standard way components communicate in ESPHome
  return is_ota_request && captive_portal::global_captive_portal != nullptr &&
         captive_portal::global_captive_portal->is_active();
#elif defined(USE_WEBSERVER_OTA_DISABLED)
  // OTA disabled for web_server and no captive portal compiled in
  return false;
#else
  // OTA enabled for web_server
  return is_ota_request;
#endif
}

void WebServerOTAComponent::report_ota_progress_(AsyncWebServerRequest *request) {
  const uint32_t now = millis();
  if (now - this->last_ota_progress_ > 1000) {
    float percentage = 0.0f;
    if (request->contentLength() != 0) {
      // Note: Using contentLength() for progress calculation is technically wrong as it includes
      // multipart headers/boundaries, but it's only off by a small amount and we don't have
      // access to the actual firmware size until the upload is complete. This is intentional
      // as it still gives the user a reasonable progress indication.
      percentage = (this->ota_read_length_ * 100.0f) / request->contentLength();
      ESP_LOGD(TAG, "OTA in progress: %0.1f%%", percentage);
    } else {
      ESP_LOGD(TAG, "OTA in progress: %" PRIu32 " bytes read", this->ota_read_length_);
    }
#ifdef USE_OTA_STATE_LISTENER
    // Report progress - use notify_state_deferred_ since we're in web server task
    this->notify_state_deferred_(ota::OTA_IN_PROGRESS, percentage, 0);
#endif
    this->last_ota_progress_ = now;
  }
}

void WebServerOTAComponent::schedule_ota_reboot_() {
  ESP_LOGI(TAG, "OTA update successful!");
  this->set_timeout(100, []() {
    ESP_LOGI(TAG, "Performing OTA reboot now");
    App.safe_reboot();
  });
}

void WebServerOTAComponent::ota_init_(const char *filename) {
  ESP_LOGI(TAG, "OTA Update Start: %s", filename);
  this->ota_read_length_ = 0;
  this->ota_success_ = false;
}

// KAUF: analyze filename and return a nonzero error code if this file should not be flashed.
static uint16_t kauf_check_filename(const PlatformString &filename) {
  std::string str = filename.c_str();
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);

  // kill process if "minimal" is found in string
  if (str.find("minimal") != std::string::npos) {
    ESP_LOGD(TAG, "***** DO NOT TRY TO FLASH TASMOTA-MINIMAL *****");
    return 1;
  }

  // kill process if "wled" is found in string
  if (str.find("wled") != std::string::npos) {
    ESP_LOGD(TAG, "***** DO NOT TRY TO FLASH WLED *****");
    return 2;
  }

  // if used, confirm filename does not conflict with sensor value
#ifdef SENSOR_4M
  if (SENSOR_4M && str.find("-1m") != std::string::npos) {
    ESP_LOGD(TAG, "***** Apparently trying to flash 1M firmware over 4M version *****");
    return 3;
  }

  if (!SENSOR_4M && str.find("-4m") != std::string::npos) {
    ESP_LOGD(TAG, "***** Apparently trying to flash 4M firmware over 1M version *****");
    return 3;
  }
#endif // SENSOR_4M

  // check filename does not contain a different product's string
  static const char *const all_products[] = {"plf10", "plf12", "plf14", "srf10", "rgbsw", "bulb", "plug"};
#if defined(KAUF_PRODUCT_PLF12)
  static const char *const my_product = "plf12";
#elif defined(KAUF_PRODUCT_PLF14)
  static const char *const my_product = "plf14";
#elif defined(KAUF_PRODUCT_PLF10)
  static const char *const my_product = "plf10";
#elif defined(KAUF_PRODUCT_BULB)
  static const char *const my_product = "bulb";
#elif defined(KAUF_PRODUCT_SRF10)
  static const char *const my_product = "srf10";
#else
  static const char *const my_product = nullptr;
#endif
  if (my_product != nullptr) {
    for (size_t i = 0; i < sizeof(all_products) / sizeof(all_products[0]); i++) {
      if (strcmp(all_products[i], my_product) == 0)
        continue;
      if (str.find(all_products[i]) != std::string::npos) {
        ESP_LOGD(TAG, "***** Wrong product: filename contains '%s', this is a %s device *****", all_products[i], my_product);
        return 4;
      }
    }
  }

  return 0;
}

void WebServerOTAComponent::handleUpload(AsyncWebServerRequest *request, const PlatformString &filename, size_t index,
                                         uint8_t *data, size_t len, bool final) {
  ota::OTAResponseTypes error_code = ota::OTA_RESPONSE_OK;

  // First byte of a new upload: index==0 with actual data. (web_server_idf
  // fires a separate start-marker call with data==nullptr/len==0 before the
  // first real chunk; gate on len>0 so we only trigger once per upload.)
  if (index == 0 && len > 0) {
    // A stalled upload times out and onDisconnect() frees it, so refusing can't block for long.
    if (this->ota_backend_) {
      ESP_LOGW(TAG, "OTA upload refused: another upload is in progress");
      this->refuse_(request);
      return;
    }

    // KAUF: reject bad firmware files before a backend is opened; the rest of the upload is then ignored below.
    if (uint16_t kauf_error = kauf_check_filename(filename)) {
      this->kauf_ota_error_code = kauf_error;
      this->kauf_error_request_ = request;
      request->onDisconnect([this, request]() {
        if (this->kauf_error_request_ == request) {
          this->kauf_error_request_ = nullptr;
          this->kauf_ota_error_code = 0;
        }
      });
      return;
    }

    this->ota_backend_ = ota::make_ota_backend();
    if (!this->ota_backend_) {
      ESP_LOGE(TAG, "Failed to create OTA backend");
#ifdef USE_OTA_STATE_LISTENER
      this->notify_state_deferred_(ota::OTA_ERROR, 0.0f, static_cast<uint8_t>(ota::OTA_RESPONSE_ERROR_UNKNOWN));
#endif
      return;
    }

    // Web server OTA uses multipart uploads where the actual firmware size
    // is unknown (contentLength includes multipart overhead)
    // Pass 0 to indicate unknown size
    error_code = this->ota_backend_->begin(0);
    if (error_code == ota::OTA_RESPONSE_ERROR_UPDATE_BUSY) {
      // Nothing started: no state notifications.
      ESP_LOGW(TAG, "OTA upload refused: another update is in progress");
      this->ota_backend_.reset();
      this->refuse_(request);
      return;
    }

    // Initialize OTA on first call
    this->ota_init_(filename.c_str());

#ifdef USE_OTA_STATE_LISTENER
    // Notify OTA started - use notify_state_deferred_ since we're in web server task
    this->notify_state_deferred_(ota::OTA_STARTED, 0.0f, 0);
#endif

    if (error_code != ota::OTA_RESPONSE_OK) {
      ESP_LOGE(TAG, "OTA begin failed: %d", error_code);
      this->ota_backend_.reset();
#ifdef USE_OTA_STATE_LISTENER
      this->notify_state_deferred_(ota::OTA_ERROR, 0.0f, static_cast<uint8_t>(error_code));
#endif
      return;
    }

    // An interrupted upload gets no final chunk.
    this->ota_request_ = request;
    request->onDisconnect([this, request]() {
      if (this->ota_request_ == request && this->ota_backend_) {
        ESP_LOGW(TAG, "OTA upload interrupted; aborting");
        this->abort_session_();
      }
    });
  }

  if (!this->ota_backend_ || request != this->ota_request_) {
    return;
  }

  // Process data
  if (len > 0) {
    error_code = this->ota_backend_->write(data, len);
    if (error_code != ota::OTA_RESPONSE_OK) {
      ESP_LOGE(TAG, "OTA write failed: %d", error_code);
      this->ota_backend_->abort();
      this->ota_backend_.reset();
      this->ota_request_ = nullptr;
#ifdef USE_OTA_STATE_LISTENER
      this->notify_state_deferred_(ota::OTA_ERROR, 0.0f, static_cast<uint8_t>(error_code));
#endif
      return;
    }
    this->ota_read_length_ += len;
    this->report_ota_progress_(request);
  }

  // Finalize
  if (final) {
    ESP_LOGD(TAG, "OTA final chunk: index=%zu, len=%zu, total_read=%" PRIu32 ", contentLength=%zu", index, len,
             this->ota_read_length_, request->contentLength());

    // For Arduino framework, the Update library tracks expected size from firmware header
    // If we haven't received enough data, calling end() will fail
    // This can happen if the upload is interrupted or the client disconnects
    error_code = this->ota_backend_->end();
    if (error_code == ota::OTA_RESPONSE_OK) {
      this->ota_success_ = true;
#ifdef USE_OTA_STATE_LISTENER
      // Report completion before reboot - use notify_state_deferred_ since we're in web server task
      this->notify_state_deferred_(ota::OTA_COMPLETED, 100.0f, 0);
#endif
      this->schedule_ota_reboot_();
    } else {
      ESP_LOGE(TAG, "OTA end failed: %d", error_code);
      // Reset the platform updater before the lock is released.
      this->ota_backend_->abort();
#ifdef USE_OTA_STATE_LISTENER
      this->notify_state_deferred_(ota::OTA_ERROR, 0.0f, static_cast<uint8_t>(error_code));
#endif
    }
    this->ota_backend_.reset();
    this->ota_request_ = nullptr;
  }
}

void WebServerOTAComponent::abort_session_() {
  this->ota_backend_->abort();
#ifdef USE_OTA_STATE_LISTENER
  this->notify_state_deferred_(ota::OTA_ABORT, 0.0f, 0);
#endif
  this->ota_backend_.reset();
  this->ota_request_ = nullptr;
}

void WebServerOTAComponent::refuse_(AsyncWebServerRequest *request) {
  this->refused_request_ = request;
  request->onDisconnect([this, request]() {
    if (this->refused_request_ == request)
      this->refused_request_ = nullptr;
  });
}

void WebServerOTAComponent::handleRequest(AsyncWebServerRequest *request) {
  // Use the ota_success_ flag to determine the actual result
  const bool refused = request == this->refused_request_;
  if (refused)
    this->refused_request_ = nullptr;

  // KAUF: pick up the filename-check error for this request, if any
  [[maybe_unused]] uint16_t kauf_error = 0;
  if (request == this->kauf_error_request_) {
    kauf_error = this->kauf_ota_error_code;
    this->kauf_error_request_ = nullptr;
    this->kauf_ota_error_code = 0;
  }

// KAUF: print out error messages.
#ifndef KAUF_SMALLER
  if (kauf_error != 0) {
    // create response
    AsyncResponseStream *stream = request->beginResponseStream("text/html");
    stream->addHeader("Access-Control-Allow-Origin", "*");
    stream->print(ESPHOME_F("<!DOCTYPE html><html><head><meta charset=UTF-8><link rel=icon href=data:></head><body>"));

    stream->print(ESPHOME_F("Update Failed. -- "));

    if (kauf_error == 1) {
      stream->print(ESPHOME_F("You appear to be trying to flash tasmota-minimal, which could brick the device. Rename firmware file to not include the word <b>minimal</b> to override."));
    }
    if (kauf_error == 2) {
      stream->print(ESPHOME_F("You appear to be trying to flash a WLED bin file, which could brick the device. Rename firmware file to not include the word <b>wled</b> or <b>WLED</b> to override."));
    }
    if (kauf_error == 3) {
      stream->print(ESPHOME_F("You appear to be trying to flash a mismatched update file, either -1m over -4m or -4m over -1m. Download the proper update file or remove <b>-1m</b> or <b>-4m</b> from filename to override."));
    }
    if (kauf_error == 4) {
      stream->print(ESPHOME_F("You appear to be trying to flash firmware for a different product. Download the correct firmware or remove the product name from the filename to override."));
    }

    stream->print(ESPHOME_F("</body></html>"));
    request->send(stream);
  }
  else if (refused) {
    AsyncWebServerResponse *response;
    response = request->beginResponse(200, "text/plain", "Update Failed! Another update is in progress, retry later.");
    response->addHeader("Connection", "close");
    request->send(response);
  }
  else if (this->ota_success_) {
    AsyncWebServerResponse *response;
    response = request->beginResponse(200, "text/plain", "Firmware file uploaded successfully. The device will now process the firmware file and reboot itself. You can try to connect to the device over Wi-Fi immediately, but don't power cycle for at least five minutes if the device is not reachable.");
    response->addHeader("Connection", "close");
    request->send(response);
  } else {

    // create response
    AsyncResponseStream *stream = request->beginResponseStream("text/html");
    stream->addHeader("Access-Control-Allow-Origin", "*");
    stream->print(ESPHOME_F("<!DOCTYPE html><html><head><meta charset=UTF-8><link rel=icon href=data:></head><body>"));
    stream->print(ESPHOME_F("Update Failed. This device is now restarting itself automatically, which could resolve the error in some cases.<br><br>"));

    stream->print(ESPHOME_F("</body></html>"));
    request->send(stream);

    // reboot
    this->set_timeout(100, []() { App.safe_reboot(); });
  }

// KAUF: if KAUF_SMALLER is defined, just use stock function
#else
  AsyncWebServerResponse *response;
#ifdef USE_ESP8266
  static const char UPDATE_SUCCESS[] PROGMEM = "Update Successful!";
  static const char UPDATE_FAILED[] PROGMEM = "Update Failed!";
  static const char UPDATE_BUSY[] PROGMEM = "Update Failed! Another update is in progress, retry later.";
  static const char TEXT_PLAIN[] PROGMEM = "text/plain";
  static const char CONNECTION_STR[] PROGMEM = "Connection";
  static const char CLOSE_STR[] PROGMEM = "close";
  const char *msg = refused ? UPDATE_BUSY : (this->ota_success_ ? UPDATE_SUCCESS : UPDATE_FAILED);
  // KAUF: beginResponse_P is deprecated in ESPAsyncWebServer 3.x; the uint8_t*/len overload builds the
  // same PROGMEM-safe AsyncProgmemResponse (plain const char* would use AsyncBasicResponse and read flash as RAM)
  response = request->beginResponse(200, TEXT_PLAIN, reinterpret_cast<const uint8_t *>(msg), strlen_P(msg));
  response->addHeader(CONNECTION_STR, CLOSE_STR);
#else
  const char *msg = refused              ? "Update Failed! Another update is in progress, retry later."
                    : this->ota_success_ ? "Update Successful!"
                                         : "Update Failed!";
  response = request->beginResponse(200, "text/plain", msg);
  response->addHeader("Connection", "close");
#endif
  request->send(response);

// KAUF: end if for printing error messages or alternatively stock functionality.
#endif
}

void WebServerOTAComponent::setup() {
  // Get the global web server base instance and register our handler
  auto *base = web_server_base::global_web_server_base;
  if (base == nullptr) {
    ESP_LOGE(TAG, "WebServerBase not found");
    this->mark_failed();
    return;
  }

  base->add_handler(this);
}

void WebServerOTAComponent::dump_config() { ESP_LOGCONFIG(TAG, "Web Server OTA"); }

}  // namespace esphome::web_server

#endif  // USE_WEBSERVER_OTA
