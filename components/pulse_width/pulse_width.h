#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace pulse_width {

/// Store data in a class that doesn't use multiple-inheritance (vtables in flash)
class PulseWidthSensorStore {
 public:
  void setup(InternalGPIOPin *pin) {
    pin->setup();
    this->pin_ = pin->to_isr();
    this->last_rise_ = micros();
    pin->attach_interrupt(&PulseWidthSensorStore::gpio_intr, this, gpio::INTERRUPT_ANY_EDGE);
  }
  static void gpio_intr(PulseWidthSensorStore *arg);
  uint32_t get_pulse_width_us() const { return this->last_width_; }
  float get_pulse_width_s() const { return this->last_width_ / 1e6f; }
  uint32_t get_last_rise() const { return last_rise_; }

  uint32_t get_last_period() const { return this->last_period_; }
  uint32_t get_avg() const {
    if ( this->avg_num_ != 0 ) { return (this->avg_sum_/this->avg_num_); }
    else                       { return 0; }
  }

  void reset_avg() {
    this->avg_num_ = 0;
    this->avg_sum_ = 0;
    this->avg_skip_ = 0;
  }

 protected:
  ISRInternalGPIOPin pin_;
  volatile uint32_t last_width_{0};
  volatile uint32_t last_period_{0};
  volatile uint32_t last_rise_{0};

  volatile uint32_t avg_num_{0};
  volatile uint32_t avg_sum_{0};
  volatile uint32_t avg_skip_{0};
};

class PulseWidthSensor : public sensor::Sensor, public PollingComponent {
 public:
  void set_pin(InternalGPIOPin *pin) { pin_ = pin; }
  void setup() override { this->store_.setup(this->pin_); }
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void update() override;

 protected:
  PulseWidthSensorStore store_;
  InternalGPIOPin *pin_;
};

}  // namespace pulse_width
}  // namespace esphome
