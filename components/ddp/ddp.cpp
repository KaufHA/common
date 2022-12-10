#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_light_effect_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static const int PORT = 4048;

DDPComponent::DDPComponent() {}
DDPComponent::~DDPComponent() {}
void DDPComponent::setup() {}

void DDPComponent::loop() {

  if ( !udp_ ) { return; }

  std::vector<uint8_t> payload;

  while (uint16_t packet_size = udp_->parsePacket()) {
    payload.resize(packet_size);

    if (!udp_->read(&payload[0], payload.size())) {
      continue;
    }

    if (!this->process_(&payload[0], payload.size())) {
      continue;
    }

  }
}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {

  if (light_effects_.count(light_effect)) {
    return;
  }

  if (!udp_) { udp_ = make_unique<WiFiUDP>(); }

  ESP_LOGD(TAG, "Starting UDP listening for DDP.");
  if (!udp_->begin(PORT)) {
    ESP_LOGE(TAG, "Cannot bind DDP to port %d.", PORT);
    mark_failed();
    return;
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
  }

}

bool DDPComponent::process_(const uint8_t *payload, uint16_t size) {

  // ignore packet if data offset != [00 00 00 00].  This likely means the device is receiving a DDP packet not meant for it.
  if ( (payload[4] != 0) || (payload[5] != 0) || (payload[6] != 0) || (payload[7] != 0) ) {
    ESP_LOGD(TAG, "Ignoring DDP Packet with non-zero data offset.");
    return false;
  }

  if ( size < 10 ){
    ESP_LOGV("KAUF DDP Debug", "Invalid DDP packet received, too short (size=%d)", size);
  }
  else if ( size == 10 ) {
    ESP_LOGV("KAUF DDP Debug", "DDP packet received w/ no channel data, 3 channels required - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9] );
  }
  else if ( size == 11 ) {
    ESP_LOGV("KAUF DDP Debug", "DDP packet received w/ 1 channel data, 3 channels required - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10] );
  }
  else if ( size == 12 ) {
    ESP_LOGV("KAUF DDP Debug", "DDP packet received w/ 2 channel data, 3 channels required - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11] );
  }
  else if ( size == 13) {
    ESP_LOGV("KAUF DDP Debug", "DDP packet received: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x [%02x %02x %02x]", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11], payload[12] );
  }
  else if ( size > 13 ) {
    ESP_LOGV("KAUF DDP Debug", "DDP packet received w/ >3 channel data, using first 3 channels (size=%d) - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x [%02x %02x %02x] %02x", size, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11], payload[12], payload[13] );
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
