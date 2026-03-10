#ifdef USE_ESP8266

#include "esp8266_pwm.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/macros.h"

#include <core_esp8266_waveform.h>
#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
#include <core_esp8266_features.h>
#endif

namespace esphome {
namespace esp8266_pwm {

static const char *const TAG = "esp8266_pwm";

void ESP8266PWM::setup() {
#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  enablePhaseLockedWaveform();
#endif
  this->pin_->setup();
  this->turn_off();
}
void ESP8266PWM::dump_config() {
  ESP_LOGCONFIG(TAG,
                "ESP8266 PWM:\n"
                "  Frequency: %.1f Hz",
                this->frequency_);
  LOG_PIN("  Pin: ", this->pin_);
#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  if (this->align_pin_ >= 0) {
    ESP_LOGCONFIG(TAG, "  Align pin: GPIO%d  Phase offset: %.3f", this->align_pin_, this->phase_current_);
  }
#endif
  LOG_FLOAT_OUTPUT(this);
}
void ESP8266PWM::loop() {
#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  if (this->align_output_ == nullptr || this->fixed_phase_) return;
  float cw = this->align_output_->last_output_;
  // Only track when both channels are actively PWMing (not off, not fully on).
  if (cw <= 0.0f || cw >= 1.0f || this->last_output_ <= 0.0f || this->last_output_ >= 1.0f) {
    this->phase_pending_ = -1.0f;  // reset on any inactive frame
    return;
  }
  if (cw != this->phase_pending_) {
    this->phase_pending_ = cw;
    this->phase_stable_ms_ = millis();
  } else if (millis() - this->phase_stable_ms_ >= this->adapt_delay_ms_) {
    float ww = this->last_output_;
    float sum = cw + ww;
    float new_phase = (sum > 0.0f) ? (cw / sum) : 0.5f;
    if (new_phase != this->phase_current_) {
      this->phase_current_ = new_phase;
      // Actual hardware overlap: CW=[0,cw], WW=[phase_hardware_, phase_hardware_+ww] mod 1
      float P = this->phase_hardware_;
      float actual_overlap;
      if (P + ww > 1.0f) {
        actual_overlap = fmaxf(0.0f, cw - P) + fminf(cw, P + ww - 1.0f);
      } else {
        actual_overlap = (P < cw) ? fminf(cw - P, ww) : 0.0f;
      }
      ESP_LOGD(TAG, "Phase saved GPIO%d: CW=%.3f WW=%.3f  hw_phase=%.3f  saving=%.3f  overlap=%.1f%%",
               this->pin_->get_pin(), cw, ww, this->phase_hardware_, this->phase_current_, actual_overlap * 100.0f);
    }
  }
#endif
}

void HOT ESP8266PWM::write_state(float state) {
  float prev_output = this->last_output_;
  this->last_output_ = state;

  // Also check pin inversion
  if (this->pin_->is_inverted()) {
    state = 1.0f - state;
  }

  auto total_time_us = static_cast<uint32_t>(roundf(1e6f / this->frequency_));
  auto duty_on = static_cast<uint32_t>(roundf(total_time_us * state));
  uint32_t duty_off = total_time_us - duty_on;

  if (duty_on == 0) {
    // Optional servo compatibility workaround (disabled by default for bulbs).
    // This path performs GPIO read + delay to avoid pulse truncation in some servo cases.
#ifdef KAUF_ESP8266_PWM_SERVO_COMPAT
    if (this->pin_->digital_read() && 50 <= this->frequency_ && this->frequency_ <= 300) {
      delay(3);
    }
#endif
    stopWaveform(this->pin_->get_pin());
    this->pin_->digital_write(this->pin_->is_inverted());
  } else if (duty_off == 0) {
    stopWaveform(this->pin_->get_pin());
    this->pin_->digital_write(!this->pin_->is_inverted());
  } else {
#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
    if (this->align_pin_ >= 0 && (prev_output <= 0.0f || prev_output >= 1.0f)) {
      if (!this->fixed_phase_ && this->phase_hint_ >= 0.0f) {
        this->phase_current_ = this->phase_hint_;
        this->phase_hint_ = -1.0f;
      }
      this->phase_hardware_ = this->phase_current_;
      auto offset_us = static_cast<uint32_t>(roundf(this->phase_current_ * total_time_us));
      ESP_LOGI(TAG, "WW startup GPIO%d: phase=%.3f offset=%uus (period=%uus)",
               this->pin_->get_pin(), this->phase_current_, offset_us, total_time_us);
      startWaveform(this->pin_->get_pin(), duty_on, duty_off, 0,
                    this->align_pin_, offset_us, false);
    } else
#endif
    startWaveform(this->pin_->get_pin(), duty_on, duty_off, 0);
  }
}

}  // namespace esp8266_pwm
}  // namespace esphome

#endif
