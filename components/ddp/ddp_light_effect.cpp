#if defined(USE_ARDUINO) || defined(USE_ESP32)

#include "ddp.h"
#include "ddp_light_effect.h"
#include "esphome/core/log.h"

#include <algorithm>

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_light_effect";

DDPLightEffect::DDPLightEffect(const char *name) : LightEffect(name) {}

esphome::StringRef DDPLightEffect::get_name() const { return LightEffect::get_name(); }

#ifdef USE_DDP_BACKGROUND_LISTEN
void DDPLightEffect::init() {
  if (this->listen_mode_ == DDP_LISTEN_YIELD_ON_CONTROL) {
    this->state_->add_remote_values_listener(this);
  }
#ifdef USE_LIGHT_GAMMA_LUT
  // Capture the normal gamma table once the parent light state exists. This is
  // needed even when background listening keeps the DDP receiver active without the
  // effect itself being selected.
  // upstream light only exposes the LUT pointer; lut is GammaTable's first member, so it converts back to the table.
  this->gamma_table_backup_ = reinterpret_cast<const light::GammaTable *>(this->state_->get_gamma_table());
#endif
}
#endif

void DDPLightEffect::start() {
#ifdef USE_DDP_BACKGROUND_LISTEN
  if (this->listen_mode_ == DDP_LISTEN_YIELD_ON_CONTROL) {
    // Explicitly selecting DDP hands control back to it.
    this->suspended_ = false;
  }
#endif
#ifndef USE_DDP_BACKGROUND_LISTEN
#ifdef USE_LIGHT_GAMMA_LUT
  // Without always-listening support, preserve the original lifecycle and
  // capture gamma state only when the effect is selected.
  // upstream light only exposes the LUT pointer; lut is GammaTable's first member, so it converts back to the table.
  this->gamma_table_backup_ = reinterpret_cast<const light::GammaTable *>(this->state_->get_gamma_table());
#endif
#endif
  this->next_packet_will_be_first_ = true;

  LightEffect::start();
  DDPLightEffectBase::start();
}

void DDPLightEffect::stop() {
#ifdef USE_LIGHT_GAMMA_LUT
  this->state_->set_gamma_table(this->gamma_table_backup_);
#endif
#ifdef USE_DDP_BACKGROUND_LISTEN
  // Turning the light off or choosing another effect while DDP is streaming is
  // a control command. stop() runs before that command is published, so the
  // remote-values listener would see DDP as idle; yield here instead.
  if (this->listen_mode_ == DDP_LISTEN_YIELD_ON_CONTROL && !this->next_packet_will_be_first_) {
    ESP_LOGD(TAG, "DDP effect '%s' stopped while streaming; yielding DDP background control.",
             this->get_name().c_str());
    this->suspended_ = true;
  }
#endif
  this->next_packet_will_be_first_ = true;

#ifdef USE_DDP_BACKGROUND_LISTEN
  // Background modes are reconciled in the DDP component loop after the new
  // requested light state/effect has been applied.
  if (this->listen_mode_ == DDP_LISTEN_EFFECT_ONLY) {
    DDPLightEffectBase::stop();
  }
#else
  DDPLightEffectBase::stop();
#endif
  LightEffect::stop();
}

void DDPLightEffect::restore_remote_state_() {
  this->next_packet_will_be_first_ = true;

#ifdef USE_LIGHT_GAMMA_LUT
  this->state_->set_gamma_table(this->gamma_table_backup_);
#endif

  // DDP only overrides current_values, never remote_values. Restoring therefore
  // returns to the latest Home Assistant / IR / Device Group state without any
  // state publication or preference write.
  this->state_->current_values = this->state_->remote_values;
  auto *output = this->state_->get_output();
  output->update_state(this->state_);
  output->write_state(this->state_);
}

void DDPLightEffect::apply() {
  if (this->timeout_check()) {
    ESP_LOGD(TAG, "DDP stream for '%s->%s' timed out.", this->state_->get_name().c_str(), this->get_name().c_str());
    this->restore_remote_state_();
  }
}

#ifdef USE_DDP_BACKGROUND_LISTEN
bool DDPLightEffect::background_listen_required_() const {
  if (this->listen_mode_ == DDP_LISTEN_YIELD_ON_CONTROL) {
    return !this->suspended_;
  }
  if (this->is_active()) {
    return true;
  }
  if (this->listen_mode_ == DDP_LISTEN_ALWAYS) {
    return true;
  }
  return this->listen_mode_ == DDP_LISTEN_WHEN_OFF && this->state_ != nullptr &&
         !this->state_->remote_values.is_on();
}

void DDPLightEffect::on_light_remote_values_update() {
  // An explicitly selected DDP effect owns the light; brightness and other
  // changes made while it is selected must not suspend it.
  if (this->listen_mode_ != DDP_LISTEN_YIELD_ON_CONTROL || this->suspended_ ||
      this->next_packet_will_be_first_ || this->is_active()) {
    return;
  }

  // A published HA/API/IR/Device Group/automation update arrived while DDP was
  // actively driving the output. Yield immediately and remain suspended until
  // the DDP effect is explicitly selected again or the device reboots.
  ESP_LOGD(TAG, "Control update received for '%s'; yielding DDP background control.",
           this->state_->get_name().c_str());
  this->restore_remote_state_();
  this->suspended_ = true;
  DDPLightEffectBase::stop();
}

void DDPLightEffect::on_background_listen_stopped_() {
  // If a WHEN_OFF listener is disabled by an HA/local ON request while DDP is
  // driving the physical output, immediately apply the requested state.
  if (!this->next_packet_will_be_first_) {
    this->restore_remote_state_();
  }
}

void DDPLightEffect::poll_() {
  // In a background mode, the effect may not be the currently
  // selected ESPHome effect, so apply() will not be called by LightState.
  if (this->listen_mode_ != DDP_LISTEN_EFFECT_ONLY && this->timeout_check()) {
    ESP_LOGD(TAG, "DDP stream for '%s->%s' timed out.", this->state_->get_name().c_str(), this->get_name().c_str());
    this->restore_remote_state_();
  }
}
#endif

uint16_t DDPLightEffect::process_(const uint8_t *payload, uint16_t size, uint16_t used) {
  const uint8_t channels = ddp_channels_per_pixel(payload, size);
  const bool is_rgbw = channels == 4;

  // One complete pixel is required for a non-addressable light.
  if (size < (used + channels)) {
    return 0;
  }

  // Disable gamma only while a DDP stream is actually active. DDP values are
  // already explicit channel values and should be written without ESPHome's
  // normal light gamma curve.
  if (this->next_packet_will_be_first_ && this->disable_gamma_) {
#ifdef USE_LIGHT_GAMMA_LUT
    this->state_->set_gamma_table(nullptr);
#endif
  }

  this->next_packet_will_be_first_ = false;
  this->last_ddp_time_ms_ = millis();

  ESP_LOGV(TAG, "Applying DDP %s data for '%s->%s': R=%02x G=%02x B=%02x W=%02x size=%d used=%d",
           is_rgbw ? "RGBW" : "RGB", this->state_->get_name().c_str(), this->get_name().c_str(), payload[used],
           payload[used + 1], payload[used + 2], is_rgbw ? payload[used + 3] : 0, size, used);

  float red = static_cast<float>(payload[used]) / 255.0f;
  float green = static_cast<float>(payload[used + 1]) / 255.0f;
  float blue = static_cast<float>(payload[used + 2]) / 255.0f;
  float white = is_rgbw ? static_cast<float>(payload[used + 3]) / 255.0f : 0.0f;

  float multiplier = this->state_->remote_values.get_brightness();
  float max_val = 0.0f;

  if (this->scaling_mode_ == DDP_SCALE_PACKET) {
    uint8_t packet_max = 0;
    for (int i = 10; i < size; i++) {
      packet_max = std::max(packet_max, payload[i]);
    }
    max_val = static_cast<float>(packet_max) / 255.0f;
  }

  // A non-addressable light is one pixel, so PIXEL and STRIP are equivalent.
  if (this->scaling_mode_ == DDP_SCALE_STRIP || this->scaling_mode_ == DDP_SCALE_PIXEL) {
    max_val = std::max(std::max(red, green), std::max(blue, white));
  }

  if (max_val != 0.0f) {
    multiplier /= max_val;
  }

  if (this->scaling_mode_ != DDP_NO_SCALING) {
    red *= multiplier;
    green *= multiplier;
    blue *= multiplier;
    white *= multiplier;
  }

  // Clamp after optional scaling; LightCall normally does this for us, but DDP
  // deliberately bypasses LightCall so remote_values and preferences remain untouched.
  red = std::min(1.0f, std::max(0.0f, red));
  green = std::min(1.0f, std::max(0.0f, green));
  blue = std::min(1.0f, std::max(0.0f, blue));
  white = std::min(1.0f, std::max(0.0f, white));

  const float rgb_max = std::max(red, std::max(green, blue));
  const float master_brightness = std::max(rgb_max, white);

  auto values = this->state_->remote_values;
  values.set_state(true);
  values.set_brightness(master_brightness);
  values.set_white(0.0f);
  values.set_cold_white(0.0f);
  values.set_warm_white(0.0f);

  // RGB color values are stored normalized in LightColorValues. Use
  // color_brightness to retain the RGB intensity independently of W.
  if (rgb_max > 0.0f) {
    values.set_red(red / rgb_max);
    values.set_green(green / rgb_max);
    values.set_blue(blue / rgb_max);
    values.set_color_brightness(master_brightness > 0.0f ? rgb_max / master_brightness : 0.0f);
  } else {
    values.set_red(1.0f);
    values.set_green(1.0f);
    values.set_blue(1.0f);
    values.set_color_brightness(0.0f);
  }

  const auto traits = this->state_->get_traits();
  const bool has_rgb = traits.supports_color_capability(light::ColorCapability::RGB);
  const bool has_rgbct = has_rgb && traits.supports_color_capability(light::ColorCapability::COLOR_TEMPERATURE);
  const bool has_rgbww = has_rgb && traits.supports_color_capability(light::ColorCapability::COLD_WARM_WHITE);
  const bool has_rgbw = has_rgb && traits.supports_color_capability(light::ColorCapability::WHITE);

  // ESPHome exposes RGBCT and RGBWW as different capabilities even when
  // color_interlock is enabled:
  //   RGBCT -> RGB + COLOR_TEMPERATURE
  //   RGBWW -> RGB + COLD_WARM_WHITE
  // Keep the mappings separate so the same DDP receiver works with either
  // physical topology.
  if (is_rgbw && has_rgbct) {
    // RGBCT: W is the white-brightness channel. Preserve the current CCT and
    // use DDP W only to control the white intensity. Force the combined mode
    // in current_values so a realtime DDP packet may contain RGB + W even when
    // the normal light has color_interlock:true.
    values.set_color_mode(light::ColorMode::RGB_COLOR_TEMPERATURE);

    const float min_mireds = traits.get_min_mireds();
    const float max_mireds = traits.get_max_mireds();
    float color_temperature = this->state_->remote_values.get_color_temperature();

    if (min_mireds > 0.0f && max_mireds > min_mireds &&
        (color_temperature < min_mireds || color_temperature > max_mireds)) {
      color_temperature = (min_mireds + max_mireds) * 0.5f;
    }

    values.set_color_temperature(color_temperature);
    values.set_white(master_brightness > 0.0f ? white / master_brightness : 0.0f);
  } else if (is_rgbw && has_rgbww) {
    // RGBWW: W is split across the physical cold- and warm-white channels
    // according to the currently selected CCT. This remains distinct from the
    // RGBCT path above, where the hardware has a separate CCT control PWM.
    values.set_color_mode(light::ColorMode::RGB_COLD_WARM_WHITE);

    const float min_mireds = traits.get_min_mireds();
    const float max_mireds = traits.get_max_mireds();
    float color_temperature = this->state_->remote_values.get_color_temperature();

    if (min_mireds > 0.0f && max_mireds > min_mireds) {
      if (color_temperature < min_mireds || color_temperature > max_mireds) {
        color_temperature = (min_mireds + max_mireds) * 0.5f;
      }

      const float ww_fraction = (color_temperature - min_mireds) / (max_mireds - min_mireds);
      const float cw_fraction = 1.0f - ww_fraction;
      const float mix_max = std::max(cw_fraction, ww_fraction);
      const float white_scale = master_brightness > 0.0f ? white / master_brightness : 0.0f;

      values.set_color_temperature(color_temperature);
      values.set_cold_white(white_scale * cw_fraction / mix_max);
      values.set_warm_white(white_scale * ww_fraction / mix_max);
    } else {
      const float white_scale = master_brightness > 0.0f ? white / master_brightness : 0.0f;
      values.set_cold_white(white_scale);
      values.set_warm_white(white_scale);
    }
  } else if (is_rgbw && has_rgbw) {
    // Native RGBW output, including a color-interlocked RGBW light. As above,
    // force the combined current mode only for the realtime DDP output.
    values.set_color_mode(light::ColorMode::RGB_WHITE);
    values.set_white(master_brightness > 0.0f ? white / master_brightness : 0.0f);
  } else if (has_rgb) {
    // Existing RGB behavior. An RGBW W byte is ignored when the target has no
    // white-capable output.
    values.set_color_mode(light::ColorMode::RGB);
    values.set_brightness(rgb_max);
    values.set_color_brightness(rgb_max > 0.0f ? 1.0f : 0.0f);
  } else {
    ESP_LOGV(TAG, "DDP target '%s' has no RGB-capable color mode", this->state_->get_name().c_str());
    return channels;
  }

  // DDP is a realtime override: write current_values directly. This deliberately
  // does not modify/publish remote_values and cannot schedule a light preference
  // save, regardless of packet rate.
  this->state_->current_values = values;
  auto *output = this->state_->get_output();
  output->update_state(this->state_);
  output->write_state(this->state_);

  return channels;
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP32
