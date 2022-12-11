#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_addressable_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_addressable_light_effect";

DDPAddressableLightEffect::DDPAddressableLightEffect(const std::string &name) : AddressableLightEffect(name) {}

const std::string &DDPAddressableLightEffect::get_name() { return AddressableLightEffect::get_name(); }

void DDPAddressableLightEffect::start() {

  // backup gamma for restoring when effect ends
  gamma_backup_ = this->state_->get_gamma_correct();

  // set gamma to zero while effect is in ... effect
  this->state_->set_gamma_correct(0.0f);

  // needed to recalculate gamma table
  get_addressable_()->setup_state(this->state_);

  AddressableLightEffect::start();
  DDPLightEffectBase::start();
}

void DDPAddressableLightEffect::stop() {

  // restore backed up gamma value and recalculate gamma table.
  this->state_->set_gamma_correct(gamma_backup_);
  get_addressable_()->setup_state(this->state_);

  DDPLightEffectBase::stop();
  AddressableLightEffect::stop();
}


void DDPAddressableLightEffect::apply(light::AddressableLight &it, const Color &current_color) { }

uint16_t DDPAddressableLightEffect::process(const uint8_t *payload, uint16_t size, uint16_t used) {
  auto *it = get_addressable_();

  int num_pixels = std::min(it->size(), ((size-used)/3));

  ESP_LOGV(TAG, "Applying DDP data for '%s' (size: %d - used: %d - num_pixels: %d)", get_name().c_str(), size, used, num_pixels);

  for (uint16_t i = used; i < used+(num_pixels*3); i+=3) {
    auto output = (*it)[(i-used)/3];
    output.set(Color(payload[i], payload[i+1], payload[i+2], (payload[i] + payload[i+1] + payload[i+2]) / 3));
  }

  it->schedule_show();

  return (num_pixels*3);
}

}  // namespace e131
}  // namespace esphome

#endif  // USE_ARDUINO
