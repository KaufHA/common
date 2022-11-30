#include "kauf_hlw8012.h"
#include "esphome/core/log.h"

namespace esphome {
namespace kauf_hlw8012 {

static const char *const TAG = "kauf_hlw8012";


void IRAM_ATTR Kauf_HLWSensorStore::gpio_intr(Kauf_HLWSensorStore *arg) {
  const bool new_level = arg->pin_.digital_read();
  const uint32_t now = micros();
  if (new_level) {
    arg->last_period_ = (now - arg->last_rise_);
    arg->last_rise_ = now;

    if ( arg->skip_ ) { arg->skip_ = false; }
    else              { arg->valid_ = true; }
  }
}

void Kauf_HLWSensorStore::reset() {
  this->skip_        = true;      // skip first received edge
  this->valid_       = false;     // data no longer valid
  this->last_rise_   = micros();  // consider this the new previous rise time for new analysis
  this->last_period_ = 0;
}


// valid for HLW8012 and CSE7759
static const uint32_t HLW8012_CLOCK_FREQUENCY = 3579000;

void Kauf_HLW8012Component::setup() {
  float reference_voltage = 0;
  ESP_LOGCONFIG(TAG, "Setting up Kauf HLW8012...");
  this->sel_pin_->setup();
  this->sel_pin_->digital_write(this->current_mode_);
  this->cf_store_.setup(this->cf_pin_);
  this->cf1_store_.setup(this->cf1_pin_);

  // Initialize multipliers
  if (this->sensor_model_ == HLW8012_SENSOR_MODEL_BL0937) {
    reference_voltage = 1.218f;
    this->power_multiplier_ =
        reference_voltage * reference_voltage * this->voltage_divider_ / this->current_resistor_ / 1721506.0f;
    this->current_multiplier_ = reference_voltage / this->current_resistor_ / 94638.0f;
    this->voltage_multiplier_ = reference_voltage * this->voltage_divider_ / 15397.0f;
  } else {
    // HLW8012 and CSE7759 have same reference specs
    reference_voltage = 2.43f;
    this->power_multiplier_ = reference_voltage * reference_voltage * this->voltage_divider_ / this->current_resistor_ *
                              64.0f / 24.0f / HLW8012_CLOCK_FREQUENCY;
    this->current_multiplier_ = reference_voltage / this->current_resistor_ * 512.0f / 24.0f / HLW8012_CLOCK_FREQUENCY;
    this->voltage_multiplier_ = reference_voltage * this->voltage_divider_ * 256.0f / HLW8012_CLOCK_FREQUENCY;
  }
}
void Kauf_HLW8012Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Kauf HLW8012:");
  LOG_PIN("  SEL Pin: ", this->sel_pin_)
  LOG_PIN("  CF Pin: ", this->cf_pin_)
  LOG_PIN("  CF1 Pin: ", this->cf1_pin_)
  ESP_LOGCONFIG(TAG, "  Current resistor: %.1f mΩ", this->current_resistor_ * 1000.0f);
  ESP_LOGCONFIG(TAG, "  Voltage Divider: %.1f", this->voltage_divider_);
  LOG_UPDATE_INTERVAL(this)
  LOG_SENSOR("  ", "Voltage", this->voltage_sensor_)
  LOG_SENSOR("  ", "Current", this->current_sensor_)
  LOG_SENSOR("  ", "Power", this->power_sensor_)
}
float Kauf_HLW8012Component::get_setup_priority() const { return setup_priority::DATA; }

float Kauf_HLW8012Component::period_to_power(float period_in) {
    return period_to_hz(period_in) * this->power_multiplier_;
}

float Kauf_HLW8012Component::period_to_current(float period_in) {
    return period_to_hz(period_in) * this->current_multiplier_;
}

float Kauf_HLW8012Component::period_to_voltage(float period_in) {
    return period_to_hz(period_in) * this->voltage_multiplier_;
}

float Kauf_HLW8012Component::period_to_hz(float period_in) {
    if ( period_in == 0.0f ) { return( 0.0f );}
    else                     { return( 1 / (period_in / 1000000.0f) ); }
}

void Kauf_HLW8012Component::loop() {

  // store valid reading in appropriate variable
  if ( this->cf1_store_.get_valid() ) {
    if ( this->current_mode_ ) { this->last_current_ = this->period_to_current(cf1_store_.get_last_period()); }
    else                       { this->last_voltage_ = this->period_to_voltage(cf1_store_.get_last_period()); }
    this->change_mode();

  // check for timeout
  } else if ( (micros()-cf1_store_.get_last_rise()) > this->timeout_us_ ) {
    if ( this->current_mode_ ) { this->last_current_ = 0.0f; }
    else                       { this->last_voltage_ = 0.0f; }
    this->change_mode();
  }

  // fade down toward timeout
  else if ( this->current_mode_ ) {
    float new_current = this->period_to_current(micros()-cf1_store_.get_last_rise());
    if ( new_current < this->last_current_ ) { this->last_current_ = new_current; }
  }
  else {
    float new_voltage = this->period_to_voltage(micros()-cf1_store_.get_last_rise());
    if ( new_voltage < this->last_voltage_ ) { this->last_voltage_ = new_voltage; }
  }

}

void Kauf_HLW8012Component::update() {

  if (this->nth_value_ == 0) {
    this->nth_value_++;
    return;
  }

  // execute power block if power sensor exists
  if (this->power_sensor_ != nullptr) {

    // if more than 9 seconds since last rising edge, consider power to be zero.
    if ((micros()-cf_store_.get_last_rise()) > this->timeout_us_) {
      this->power_sensor_->publish_state(0.0f);
    }

    // if more than update interval, publish as if edge occured now (if already zero, stay there)
    // has to also be greater than last period.  If pulse was already taking greater than update interval, need to at least wait that long
    // before trying to fade down.
    else if ( ((micros()-cf_store_.get_last_rise()) > (this->get_update_interval()*1000)) &&
              ((micros()-cf_store_.get_last_rise()) > cf_store_.get_last_period()) &&
              (this->power_sensor_->state != 0.0f)) {
      this->power_sensor_->publish_state(this->period_to_power(micros()-cf_store_.get_last_rise()));
    }

    // otherwise, publish actual value based on last period
    else {
      this->power_sensor_->publish_state(this->period_to_power(cf_store_.get_last_period()));
    }

  }

  // execute current block if current sensor exists and we are in current mode
  if ( this->current_sensor_ != nullptr ) {
    this->current_sensor_->publish_state(this->last_current_);
  }

  if ( this->voltage_sensor_ != nullptr ) {
    this->voltage_sensor_->publish_state(this->last_voltage_);
  }


}

void Kauf_HLW8012Component::change_mode() {
    this->current_mode_ = !this->current_mode_;
    ESP_LOGV(TAG, "Changing mode to %s mode", this->current_mode_ ? "CURRENT" : "VOLTAGE");
    this->sel_pin_->digital_write(this->current_mode_);
    this->cf1_store_.reset();
}

}  // namespace kauf_hlw8012
}  // namespace esphome
