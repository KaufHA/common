#if defined(USE_ARDUINO) && !defined(USE_ESP32)

#include "ddp.h"
#include "esphome/core/log.h"

#ifdef USE_DDP_STATS
#include <cstdio>
#endif

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static const int PORT = 4048;

void DDPComponent::loop() {

#ifdef USE_DDP_BACKGROUND_LISTEN
  this->update_background_effects_();
  this->poll_effects_();
#endif

  if ( !this->ensure_listening_() ) { return; }

  std::vector<uint8_t> payload;

  while (uint16_t packet_size = this->udp_->parsePacket()) {
    payload.resize(packet_size);

    if (!this->udp_->read(&payload[0], payload.size())) {
      continue;
    }

    if (!this->process_(&payload[0], payload.size())) {
      continue;
    }

#ifdef USE_DDP_STATS
    IPAddress ip = this->udp_->remoteIP();
    uint16_t port = this->udp_->remotePort();
    char source[32];
    snprintf(source, sizeof(source), "%u.%u.%u.%u:%u", ip[0], ip[1], ip[2], ip[3], port);
    this->note_packet_(source, static_cast<uint16_t>(payload.size()));
#endif
  }
}

bool DDPComponent::start_listening_() {
  this->udp_ = make_unique<WiFiUDP>();

  ESP_LOGD(TAG, "Starting UDP listening for DDP.");
  if (!this->udp_->begin(PORT)) {
    ESP_LOGE(TAG, "Cannot bind DDP to port %d.", PORT);
    this->udp_ = nullptr;
    this->mark_failed();
    return false;
  }
  return true;
}

void DDPComponent::stop_listening_() {
  ESP_LOGD(TAG, "Stopping UDP listening for DDP.");
  this->udp_->stop();
  this->udp_ = nullptr;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO && !USE_ESP32
