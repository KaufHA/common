#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_light_effect_base.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include <WiFi.h>
#endif

#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#endif

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static const int PORT = 4048;

DDPComponent::DDPComponent() {}
DDPComponent::~DDPComponent() {}
void DDPComponent::setup() {}

void DDPComponent::loop() {
  std::vector<uint8_t> payload;
  DDP_Packet packet;

  while (uint16_t packet_size = udp_->parsePacket()) {
    payload.resize(packet_size);

    if (!udp_->read(&payload[0], payload.size())) {
      continue;
    }

    if (!this->process_(&payload[0], payload.size())) {
      return;
    }

  }
}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {

  if (light_effects_.count(light_effect)) {
    return;
  }

  if (!udp_) {
    ESP_LOGD(TAG, "Starting UDP listening for DDP.");
    udp_ = make_unique<WiFiUDP>();
    if (!udp_->begin(PORT)) {
      ESP_LOGE(TAG, "Cannot bind DDP to port %d.", PORT);
      mark_failed();
      return;
    }
  }

  light_effects_.insert(light_effect);

}

void DDPComponent::remove_effect(DDPLightEffectBase *light_effect) {

  if (!light_effects_.count(light_effect)) {
    return;
  }

  light_effects_.erase(light_effect);

  // if no more effects left, stop udp listening
  if ( (light_effects_.size() == 0) && udp_) {
    ESP_LOGD(TAG, "Stopping UDP listening for DDP.");
    udp_->stop();
    udp_->reset();
  }

}

bool DDPComponent::process_(const uint8_t *payload, uint16_t size) {

  // ignore packet if data offset != [00 00 00 00].  This likely means the device is receiving a DDP packet not meant for it.
  if ( (payload[4] != 0) || (payload[5] != 0) || (payload[6] != 0) || (payload[7] != 0) ) {
    ESP_LOGV(TAG, "Ignoring DDP Packet with non-zero data offset.");
    return false;
  }

  // first 10 bytes are the header, so consider them used from the get-go
  uint16_t used = 10;

  // run through all registered effects, each takes required data per their size starting at packet address determined by used.
  for (auto *light_effect : light_effects_) {
    if ( used >= size ) {return false; }
    uint16_t new_used = light_effect->process(&payload[0], size, used);
    if (new_used == 0)  { return false; }
    else                { used += new_used; }
  }

  // if any unused data remains, either tree or chain.
  if ( used < size ) {

    if ( this->tree_  ) { return this->forward_tree_( &payload[0], size, used); }
    if ( this->chain_ ) { return this->forward_chain_(&payload[0], size, used); }

  }

  return true;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
