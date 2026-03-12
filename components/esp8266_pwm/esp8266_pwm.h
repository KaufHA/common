#pragma once

#ifdef USE_ESP8266

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/output/float_output.h"

#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
extern "C" void enablePhaseLockedWaveform();
#endif

namespace esphome {
namespace esp8266_pwm {

enum QuantizeMode : uint8_t {
  QUANTIZE_NONE = 0,  // Keep default behavior (round to nearest PWM tick)
  QUANTIZE_UP = 1,    // Always round up to next PWM tick
  QUANTIZE_DOWN = 2,  // Always round down to previous PWM tick
};

class ESP8266PWM : public output::FloatOutput, public Component {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void set_frequency(float frequency) { this->frequency_ = frequency; }
  void set_quantize_mode(QuantizeMode mode) { this->quantize_mode_ = mode; }
  uint8_t get_pin_num() const { return this->pin_->get_pin(); }
  /// Dynamically update frequency
  void update_frequency(float frequency) override {
    this->set_frequency(frequency);
    this->write_state(this->last_output_);
  }

#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  void set_align_pin(int8_t pin) { this->align_pin_ = pin; }
  void set_align_output(ESP8266PWM *output) { this->align_output_ = output; }
  void set_phase_offset(float offset) { this->phase_current_ = offset; this->fixed_phase_ = true; }
  void set_adapt_delay(uint32_t ms) { this->adapt_delay_ms_ = ms; }
  void prepare_startup_phase(float hint) { this->phase_hint_ = hint; }
#endif
  float get_last_output() const { return this->last_output_; }

  /// Initialize pin
  void setup() override;
  void loop() override;
  void dump_config() override;
  /// HARDWARE setup_priority
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void write_state(float state) override;

  InternalGPIOPin *pin_;
  float frequency_{1000.0};
  /// Cache last output level for dynamic frequency updating
  float last_output_{0.0};
  QuantizeMode quantize_mode_{QUANTIZE_NONE};

#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  int8_t align_pin_{-1};
  bool fixed_phase_{false};       // true = use phase_current_ as fixed offset, no adaptive logic
  /// Phase offset (fraction 0-1) used at next INIT. Updated by loop() after 2s stability.
  float phase_current_{0.5f};
  float phase_pending_{-1.0f};      // candidate CW duty being watched for stability; -1 = none
  uint32_t phase_stable_ms_{0};     // millis() when phase_pending_ last changed
  uint32_t adapt_delay_ms_{2000};   // stability window before committing phase_current_ (not really used except to print status)
  float phase_hint_{-1.0f};         // set by light component; -1 = none pending
  float phase_hardware_{0.5f};      // phase actually sent to hardware at last INIT; never changes while on
  ESP8266PWM *align_output_{nullptr};
#endif
};

template<typename... Ts> class SetFrequencyAction : public Action<Ts...> {
 public:
  SetFrequencyAction(ESP8266PWM *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(float, frequency);

  void play(const Ts &...x) {
    float freq = this->frequency_.value(x...);
    this->parent_->update_frequency(freq);
  }

  ESP8266PWM *parent_;
};

}  // namespace esp8266_pwm
}  // namespace esphome

#endif
