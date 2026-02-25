#pragma once

#ifdef USE_ESP8266

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace esp8266_pwm {

class ESP8266PWM : public output::FloatOutput, public Component {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void set_frequency(float frequency) { this->frequency_ = frequency; }
  uint8_t get_pin_num() const { return this->pin_->get_pin(); }
  /// Dynamically update frequency
  void update_frequency(float frequency) override {
    this->set_frequency(frequency);
    this->write_state(this->last_output_);
  }

#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  void set_align_pin(int8_t pin) { this->align_pin_ = pin; }
  void set_phase_offset(float offset) { this->phase_offset_ = offset; }
#endif

  /// Initialize pin
  void setup() override;
  void dump_config() override;
  /// HARDWARE setup_priority
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void write_state(float state) override;

  InternalGPIOPin *pin_;
  float frequency_{1000.0};
  /// Cache last output level for dynamic frequency updating
  float last_output_{0.0};

#ifdef KAUF_ESP8266_PHASE_LOCKED_PWM
  int8_t align_pin_{-1};
  float phase_offset_{0.5f};
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
