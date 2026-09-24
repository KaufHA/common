#if defined(USE_ARDUINO) || defined(USE_ESP32)

#include "ddp.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace ddp {

DDPLightEffectBase::DDPLightEffectBase() {}

void DDPLightEffectBase::start() {
  this->suspended_ = false;
  if (this->ddp_) {
    this->ddp_->add_effect(this);
  }
}

void DDPLightEffectBase::stop() {
  if (this->ddp_) {
    this->ddp_->remove_effect(this);
  }
}

void DDPLightEffectBase::suspend() {
  this->suspended_ = true;
  this->next_packet_will_be_first_ = true;
  if (this->ddp_) {
    this->ddp_->remove_effect(this);
  }
}

void DDPLightEffectBase::resume() {
  this->suspended_ = false;
  this->next_packet_will_be_first_ = true;
  if (this->ddp_) {
    this->ddp_->add_effect(this);
  }
}

// returns true if this effect is timed out
// next_packet_will_be_first_ variable keeps it from timing out multiple times
bool DDPLightEffectBase::timeout_check() {

  // don't timeout if timeout is disabled
  if ( this->timeout_ == 0)  { return false; }

  // don't timeout if no ddp stream was ever started
  if ( this->next_packet_will_be_first_ ) { return false; }

  // don't timeout if timeout hasn't been reached
  if ( (millis() - this->last_ddp_time_ms_) <= this->timeout_ ) { return false; }

  return true;

}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP32
