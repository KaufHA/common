#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace ddp {

DDPLightEffectBase::DDPLightEffectBase() {}

void DDPLightEffectBase::start() {
  if (this->ddp_) {
    this->ddp_->add_effect(this);
  }
}

void DDPLightEffectBase::stop() {
  if (this->ddp_) {
    this->ddp_->remove_effect(this);
  }
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
