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

  }
}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {

  if (light_effects_.count(light_effect)) {
    return;
  }

  // only the first effect added needs to start udp listening
  if (this->light_effects_.size() != 0) { return; }

  if (!this->udp_) { this->udp_ = make_unique<WiFiUDP>(); }

  ESP_LOGD(TAG, "Starting UDP listening for DDP.");
  if (!this->udp_->begin(PORT)) {
    ESP_LOGE(TAG, "Cannot bind DDP to port %d.", PORT);
    mark_failed();
    return;
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

bool DDPComponent::process_(const uint8_t *payload, uint16_t size) {

  // size under 10 means we don't even receive a valid header.
  // size under 13 means we don't have enough for even 1 pixel.
  if ( size < 13 ){
    ESP_LOGE(TAG, "Invalid DDP packet received, too short (size=%d)", size);
    return false;
  }

  // ignore packet if data offset != [00 00 00 00].  This likely means the device is receiving a DDP packet not meant for it.
  // There may be a better way to handle this header field.  One user was receiving packets with non-zero data offset that were
  // screwing up the light effect (flickering).
  if ( payload[4] || payload[5] || payload[6] || payload[7] ) {
    ESP_LOGE(TAG, "Ignoring DDP Packet with non-zero data offset.");
    return false;
  }

  ESP_LOGV(TAG, "DDP packet received (size=%d): - %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x [%02x %02x %02x]", size, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11], payload[12] );

  // first 10 bytes are the header, so consider them used from the get-go
  // if timecode field is used, takes up an additional 4 bytes of header.
  // this component does not handle the timecode field.  If there is a situation
  // where the timecode field is included and cannot be removed, this may need
  // modified to handle the timecode field.  So far, neither WLED nor xLights
  // follow the header spec in general, and neither sends a timecode field.
  uint16_t used = 10;

  // run through all registered effects, each takes required data per their size starting at packet address determined by used.
  for (auto *light_effect : this->light_effects_) {
    if ( used >= size ) { return false; }
    uint16_t new_used = light_effect->process_(&payload[0], size, used);
    if (new_used == 0)  { return false; }
    else                { used += new_used; }
  }

  return true;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
