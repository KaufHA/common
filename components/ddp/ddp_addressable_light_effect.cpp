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


#ifdef USE_ESP32
  uint16_t num_pixels = std::min((int)it->size(), ((size-used)/3));
#else
  uint16_t num_pixels = min(it->size(), ((size-used)/3));
#endif

  if ( num_pixels < 1 ) { return 0; }

  ESP_LOGV(TAG, "Applying DDP data for '%s' (size: %d - used: %d - num_pixels: %d)", get_name().c_str(), size, used, num_pixels);

  // will be multiplied by RGB values in scale_* scaling modes
  float multiplier = 1.0f;

  // grab scaling multiplier for packet or strip level
  // max out brightness in all but multiply mode, in which brightness is used.
  switch (this->scaling_mode_) {
    case DDP_SCALE_PACKET:
      multiplier = this->scan_packet_and_return_multiplier_(payload,10,size);
      set_max_brightness_();
      break;
    case DDP_SCALE_STRIP:
      multiplier = this->scan_packet_and_return_multiplier_(payload, used, used + (num_pixels*3));
      set_max_brightness_();
      break;
    case DDP_NO_SCALING:  // no scaling requires brightness maxed so that ddp values will be displayed raw.
    case DDP_SCALE_PIXEL: // pixel scaling occurs at the pixel level, no need to scan here but we still need brightness maxed.
      set_max_brightness_();
    default:
      break; // Multiply mode is default ESPHome behavior, no need to do anything to handle it.
  }

  // loop through all pixels being displayed now.
  for (uint16_t i = used; i < used+(num_pixels*3); i+=3) {

    // get RGB value of current pixel.
    uint8_t red   = payload[i];
    uint8_t green = payload[i+1];
    uint8_t blue  = payload[i+2];

    // set multiplier for this pixel if in pixel scaling mode
    if ( this->scaling_mode_ == DDP_SCALE_PIXEL ) {
        uint8_t max_val = 0;

        // find largest value of this pixel's rgb
        if ( (red >= green) && (red >= blue ) ) { max_val = red;   }
        else if             ( green >= blue )   { max_val = green; }
        else                                    { max_val = blue;  }

        // calculate multiplier based on max value
        multiplier = multiplier_from_max_val_(max_val);
    }

    // if we are in any scaling mode, multiply the pixel rgb values by the multiplier.
    // multiply mode uses the brightness from the remote value (set in home assistant) and otherwise uses raw rgb values
    // no_scaling mode uses raw rgb values with max brightness (set above)
    switch (this->scaling_mode_) {
      case DDP_SCALE_PACKET:
      case DDP_SCALE_STRIP:
      case DDP_SCALE_PIXEL:
        if ( multiplier != 1.0f ) {
          red   = (float)red   * multiplier;
          green = (float)green * multiplier;
          blue  = (float)blue  * multiplier;
        }
      default:
        break;
    }

    // assign pixel color
    auto output = (*it)[(i-used)/3];
    output.set_rgb(red, green, blue);
  }

  it->schedule_show();
  return (num_pixels*3);
}

float DDPAddressableLightEffect::scan_packet_and_return_multiplier_(const uint8_t *payload, uint16_t start, uint16_t end) {

  uint8_t max_val = 0;

  // look for highest value in packet or strip
  for ( uint16_t i = start; i < end; i++ ) {
    if ( payload[i] > max_val ) { max_val = payload[i]; }
  }

  // if max val is still 0, set to 255.  Either we didn't scan because (start == end) or there are no on pixels.
  // if start and end values are equal, we take that to mean mode is no scaling and brightness should be maxed so that
  // ddp values are shown raw.
  // if all pixels are off, brightness doesn't matter.
  return multiplier_from_max_val_(max_val);

}

float DDPAddressableLightEffect::multiplier_from_max_val_(uint8_t max_val) {
  if ( max_val == 0 ) { return 1.0f; }
  else { return this->state_->remote_values.get_brightness()*255.0f/(float)max_val; }
}

void DDPAddressableLightEffect::set_max_brightness_() {
  this->state_->current_values.set_brightness(1.0f);
  this->get_addressable_()->update_state(this->state_);
}

}  // namespace e131
}  // namespace esphome

#endif  // USE_ARDUINO
