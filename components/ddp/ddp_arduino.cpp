#ifdef USE_ARDUINO

#include "ddp.h"
#include "esphome/core/log.h"

#include <cstdio>

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static const int PORT = 4048;

void DDPComponent::loop() {

  if ( !this->udp_ ) { return; }

  std::vector<uint8_t> payload;

  while (uint16_t packet_size = this->udp_->parsePacket()) {
    payload.resize(packet_size);

    if (!this->udp_->read(&payload[0], payload.size())) {
      continue;
    }

    if (!this->process_(&payload[0], payload.size())) {
      continue;
    }

    if (this->stats_interval_ms_ != 0) {
      IPAddress ip = this->udp_->remoteIP();
      uint16_t port = this->udp_->remotePort();
      char source[32];
      snprintf(source, sizeof(source), "%u.%u.%u.%u:%u", ip[0], ip[1], ip[2], ip[3], port);
      this->note_packet_(source, static_cast<uint16_t>(payload.size()));
    }
  }
}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {

  if (light_effects_.count(light_effect)) {
    return;
  }

  // only the first effect added needs to start udp listening
  // but we still need to add the effect to the set so it can be applied.
  if (this->light_effects_.size() == 0) {
    if (!this->udp_) { this->udp_ = make_unique<WiFiUDP>(); }

    ESP_LOGD(TAG, "Starting UDP listening for DDP.");
    if (!this->udp_->begin(PORT)) {
      ESP_LOGE(TAG, "Cannot bind DDP to port %d.", PORT);
      mark_failed();
      return;
    }
  }

  this->light_effects_.insert(light_effect);
}

void DDPComponent::remove_effect(DDPLightEffectBase *light_effect) {

  if (!this->light_effects_.count(light_effect)) { return; }

  this->light_effects_.erase(light_effect);

  // if no more effects left, stop udp listening
  if ( (this->light_effects_.size() == 0) && this->udp_) {
    ESP_LOGD(TAG, "Stopping UDP listening for DDP.");
    this->udp_->stop();
  }
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
