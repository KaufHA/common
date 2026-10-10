#if defined(USE_ARDUINO) || defined(USE_ESP32)

#include "ddp.h"
#include "ddp_light_effect_base.h"
#include "esphome/components/network/util.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <cstring>

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";

DDPComponent::DDPComponent() {}
DDPComponent::~DDPComponent() {}
void DDPComponent::setup() {}

void DDPComponent::ensure_always_effects_() {
  // On WiFi-backed Arduino targets (including LibreTiny/BK72xx), binding a
  // WiFiUDP socket before the station interface is actually connected can fail
  // badly enough to disrupt boot. The component setup priority is AFTER_WIFI,
  // but WiFi association is asynchronous, so setup ordering alone is not enough.
  // Defer the always-listening DDP registration until ESPHome reports a usable
  // network connection. Existing effect-start behavior is unchanged.
  if (!network::is_connected()) {
    return;
  }

  for (auto *effect : this->always_effects_) {
    if (!effect->is_suspended() && !this->light_effects_.count(effect)) {
      this->add_effect(effect);
    }
  }
}

void DDPComponent::suspend_always_effects() {
  for (auto *effect : this->always_effects_) {
    effect->suspend();
  }
}

void DDPComponent::resume_always_effects() {
  for (auto *effect : this->always_effects_) {
    effect->resume();
  }
}

bool DDPComponent::has_active_stream() const {
  for (auto *effect : this->always_effects_) {
    if (!effect->is_suspended() && effect->is_stream_active()) {
      return true;
    }
  }
  return false;
}

void DDPComponent::poll_effects_() {
  for (auto *effect : this->light_effects_) {
    effect->poll_();
  }
}

void DDPComponent::note_packet_(const char *source, uint16_t size) {
  if (this->stats_interval_ms_ == 0) {
    return;
  }
  this->stats_packets_++;
  this->last_packet_size_ = size;
  if (source != nullptr && source[0] != '\0') {
    std::strncpy(this->last_source_, source, sizeof(this->last_source_) - 1);
    this->last_source_[sizeof(this->last_source_) - 1] = '\0';
    this->have_source_ = true;
  }

  uint32_t now = millis();
  if (this->stats_last_ms_ == 0) {
    this->stats_last_ms_ = now;
    return;
  }

  uint32_t elapsed = now - this->stats_last_ms_;
  if (elapsed < this->stats_interval_ms_) {
    return;
  }

  uint32_t pps = (this->stats_packets_ * 1000) / elapsed;
  if (this->have_source_) {
    ESP_LOGD(TAG, "DDP stats: %u pps, last %u bytes from %s", pps, this->last_packet_size_, this->last_source_);
  } else {
    ESP_LOGD(TAG, "DDP stats: %u pps, last %u bytes", pps, this->last_packet_size_);
  }

  this->stats_packets_ = 0;
  this->stats_last_ms_ = now;
}

bool DDPComponent::process_(const uint8_t *payload, uint16_t size) {

  // size under 10 means we don't even receive a valid header.
  // size under 13 means we don't have enough for even 1 pixel.
  if (size < 13) {
    ESP_LOGE(TAG, "Invalid DDP packet received, too short (size=%d)", size);
    return false;
  }

  // ignore packet if data offset != [00 00 00 00].  This likely means the device is receiving a DDP packet not meant for it.
  // There may be a better way to handle this header field.  One user was receiving packets with non-zero data offset that were
  // screwing up the light effect (flickering).
  if (payload[4] || payload[5] || payload[6] || payload[7]) {
    ESP_LOGE(TAG, "Ignoring DDP Packet with non-zero data offset.");
    return false;
  }

  ESP_LOGV(TAG,
           "DDP packet received (size=%d): - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x [%02x %02x %02x]",
           size, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8],
           payload[9], payload[10], payload[11], payload[12]);

  // first 10 bytes are the header, so consider them used from the get-go
  // if timecode field is used, takes up an additional 4 bytes of header.
  // this component does not handle the timecode field.  If there is a situation
  // where the timecode field is included and cannot be removed, this may need
  // modified to handle the timecode field.  So far, neither WLED nor xLights
  // follow the header spec in general, and neither sends a timecode field.
  uint16_t used = 10;

  // run through all registered effects, each takes required data per their size starting at packet address determined by used.
  for (auto *light_effect : this->light_effects_) {
    if (used >= size) {
      return false;
    }
    uint16_t new_used = light_effect->process_(payload, size, used);
    if (new_used == 0) {
      return false;
    } else {
      used += new_used;
    }
  }

  return true;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP32
