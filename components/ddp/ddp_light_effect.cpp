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

  // backup gamma for restoring when effect ends
  this->gamma_backup_ = this->state_->get_gamma_correct();
  this->next_packet_will_be_first_ = true;

  LightEffect::start();
  DDPLightEffectBase::start();
}

void DDPLightEffect::stop() {

  // restore gamma.
  this->state_->set_gamma_correct(this->gamma_backup_);
  this->next_packet_will_be_first_ = true;

  DDPLightEffectBase::stop();
  LightEffect::stop();
}

void DDPLightEffect::apply() {

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

    // restore backed up gamma value
    this->state_->set_gamma_correct(this->gamma_backup_);
    call.perform();
   }

}

uint16_t DDPLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {

  // at least for now, we require 3 bytes of data (r, g, b).
  // If there aren't 3 unused bytes, return 0 to indicate error.
  if ( size < (used + 3) ) { return 0; }

  // disable gamma on first received packet, not just based on effect being enabled.
  // that way home assistant light can still be used as normal when DDP packets are not
  // being received but effect is still enabled.
  // gamma will be enabled again when effect disabled or on timeout.
  if ( this->next_packet_will_be_first_ && this->disable_gamma_ ) {
    this->state_->set_gamma_correct(0.0f);
  }

  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();

  ESP_LOGV(TAG, "Applying DDP data for '%s->%s': (%02x,%02x,%02x) size = %d, used = %d", this->state_->get_name().c_str(), this->get_name().c_str(), payload[used], payload[used+1], payload[used+2], size, used);

  float red   = (float)payload[used]/255.0f;
  float green = (float)payload[used+1]/255.0f;
  float blue  = (float)payload[used+2]/255.0f;

  float multiplier = this->state_->remote_values.get_brightness();
  float max_val = 0;

  // for DDP_PACKET mode, find largest r,g, or b value in packet and scale that value to brightness
  if ( this->scaling_mode_ == DDP_SCALE_PACKET ) {
    // find largest rgb value in packet
    uint8_t packet_max = 0;
    for ( int i = 10; i < size; i++ ) {
      if ( payload[i] > packet_max ) { packet_max = payload[i]; }
    }
    max_val = (float)packet_max / 255.0f;
  }

  // DDP_PIXEL and DDP_STRIP are the same with bulbs since a "strip" is one pixel.
  if ( (this->scaling_mode_ == DDP_SCALE_STRIP) || (this->scaling_mode_ == DDP_SCALE_PIXEL)) {
    if ( (red >= green) && (red >= blue ) ) { max_val = red;   }
    else if             ( green >= blue )   { max_val = green; }
    else                                    { max_val = blue;  }
  }

  // if we got a max_val, update multiplier
  if ( max_val != 0 ) { multiplier /= max_val; }

  // if we are in any scaling mode, multiply the pixel rgb values by the multiplier.
  if ( this->scaling_mode_ != DDP_NO_SCALING) {
    red   *= multiplier;
    green *= multiplier;
    blue  *= multiplier;
  }

  auto call = this->state_->turn_on();

  call.set_color_mode_if_supported(light::ColorMode::RGB_COLD_WARM_WHITE);
  call.set_color_mode_if_supported(light::ColorMode::RGB_COLOR_TEMPERATURE);
  call.set_color_mode_if_supported(light::ColorMode::RGB_WHITE);
  call.set_color_mode_if_supported(light::ColorMode::RGB);
  call.set_red_if_supported(red);
  call.set_green_if_supported(green);
  call.set_blue_if_supported(blue);
  call.set_brightness_if_supported(std::max(red, std::max(green, blue)) );
  call.set_color_brightness_if_supported(1.0f);

  // disable white channels
  call.set_white_if_supported(0.0f);
  call.set_cold_white_if_supported(0.0f);
  call.set_warm_white_if_supported(0.0f);

  call.set_transition_length_if_supported(0);
  call.set_publish(false);
  call.set_save(false);

  call.perform();

  // manually calling loop otherwise we just go straight into processing the next DDP
  // packet without executing the light loop to display the just-processed packet.
  // Not totally sure why or if there is a better way to fix, but this works.
  this->state_->loop();

  return 3;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
