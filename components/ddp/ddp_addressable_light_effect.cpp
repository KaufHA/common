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
  this->gamma_backup_ = this->state_->get_gamma_correct();
  this->next_packet_will_be_first_ = true;

  AddressableLightEffect::start();
  DDPLightEffectBase::start();

  // not automatically active just because enabled
  this->get_addressable_()->set_effect_active(false);

}

void DDPAddressableLightEffect::stop() {

  // restore backed up gamma value and recalculate gamma table.
  this->state_->set_gamma_correct(this->gamma_backup_);
  this->get_addressable_()->setup_state(this->state_);
  this->next_packet_will_be_first_ = true;

  DDPLightEffectBase::stop();
  AddressableLightEffect::stop();
}

void DDPAddressableLightEffect::apply(light::AddressableLight &it, const Color &current_color) {

  // if receiving DDP packets times out, reset to home assistant color.
  // apply function is not needed normally to display changes to the light
  // from Home Assistant, but it is needed to restore value on timeout.
  if ( this->timeout_check() ) {
    ESP_LOGD(TAG,"DDP stream for '%s->%s' timed out.", this->state_->get_name().c_str(), this->get_name().c_str());
    this->next_packet_will_be_first_ = true;

    auto call = this->state_->turn_on();

    call.set_color_mode_if_supported(this->state_->remote_values.get_color_mode());
    call.set_red_if_supported(this->state_->remote_values.get_red());
    call.set_green_if_supported(this->state_->remote_values.get_green());
    call.set_blue_if_supported(this->state_->remote_values.get_blue());
    call.set_brightness_if_supported(this->state_->remote_values.get_brightness());
    call.set_color_brightness_if_supported(this->state_->remote_values.get_color_brightness());

    call.set_white_if_supported(this->state_->remote_values.get_white());
    call.set_cold_white_if_supported(this->state_->remote_values.get_cold_white());
    call.set_warm_white_if_supported(this->state_->remote_values.get_warm_white());

    call.set_publish(false);
    call.set_save(false);

    // restore backed up gamma value and recalculate gamma table.
    this->state_->set_gamma_correct(this->gamma_backup_);
    it.setup_state(this->state_);

    // effect no longer active
    it.set_effect_active(false);

    call.perform();
   }

}

uint16_t DDPAddressableLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {

  // disable gamma on first received packet, not just based on effect being enabled.
  // that way home assistant light can still be used as normal when DDP packets are not
  // being received but effect is still enabled.
  // gamma will be enabled again when effect disabled or on timeout.
  if ( this->next_packet_will_be_first_ && this->disable_gamma_ ) {
    this->state_->set_gamma_correct(0.0f);
    this->get_addressable_()->setup_state(this->state_);
  }

  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();

  auto *it = this->get_addressable_();

  // effect is active once a ddp packet is received.
  it->set_effect_active(true);


  uint16_t num_pixels = std::min(it->size(), ((size-used)/3));
  if ( num_pixels < 1 ) { return 0; }

  ESP_LOGV(TAG, "Applying DDP data for '%s' (size: %d - used: %d - num_pixels: %d)", get_name().c_str(), size, used, num_pixels);

  for (uint16_t i = used; i < used+(num_pixels*3); i+=3) {
    auto output = (*it)[(i-used)/3];
    output.set(Color(payload[i], payload[i+1], payload[i+2]));
  }

  it->schedule_show();

  return (num_pixels*3);
}

}  // namespace e131
}  // namespace esphome

#endif  // USE_ARDUINO
