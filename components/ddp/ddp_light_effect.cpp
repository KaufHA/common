#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_light_effect";

DDPLightEffect::DDPLightEffect(const std::string &name) : LightEffect(name) {}

const std::string &DDPLightEffect::get_name() { return LightEffect::get_name(); }

void DDPLightEffect::start() {

  gamma_backup_ = this->state_->get_gamma_correct();
  this->state_->set_gamma_correct(0.0f);

  LightEffect::start();
  DDPLightEffectBase::start();
}

void DDPLightEffect::stop() {

  this->state_->set_gamma_correct(gamma_backup_);

  DDPLightEffectBase::stop();
  LightEffect::stop();
}

void DDPLightEffect::apply() {}

uint16_t DDPLightEffect::process(const uint8_t *payload, uint16_t size, uint16_t used) {

  // for now, we require 3 bytes of data (r, g, b).
  // If there aren't 3 unused bytes, return 0 to indicate error.
  if ( size < (used + 3) ) { return 0; }

  ESP_LOGV(TAG, "Applying DDP data for '%s': (%02x,%02x,%02x) size = %d, used = %d", get_name().c_str(), payload[used], payload[used+1], payload[used+2], size, used);

  float r = (float)payload[used]/255.0f;
  float g = (float)payload[used+1]/255.0f;
  float b = (float)payload[used+2]/255.0f;

  auto call = this->state_->turn_on();

  call.set_color_mode_if_supported(light::ColorMode::RGB);
  call.set_red_if_supported(r);
  call.set_green_if_supported(g);
  call.set_blue_if_supported(b);
  call.set_white_if_supported((r+g+b)/3.0f);
  call.set_brightness_if_supported(std::max(r, std::max(g, b)) );

  call.set_transition_length_if_supported(0);
  call.set_publish(false);
  call.set_save(false);

  call.perform();

  // manually calling loop otherwise we just go straight into the next DDP packet without
  // executing loop and displaying the current one.  Not totally sure why or if there is
  // a better way to fix this, but this works well.
  this->state_->loop();

  return 3;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
