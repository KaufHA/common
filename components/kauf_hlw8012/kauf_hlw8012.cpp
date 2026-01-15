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
  LOG_PIN(" SEL: ", this->sel_pin_);
  LOG_PIN("  CF: ", this->cf_pin_);
  LOG_PIN(" CF1: ", this->cf1_pin_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Voltage", this->voltage_sensor_);
  LOG_SENSOR("  ", "Current", this->current_sensor_);
  LOG_SENSOR("  ", "Power", this->power_sensor_);
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

  //////////////////////////////////
  // calculate current or voltage //
  //////////////////////////////////

  // store reading in appropriate variable if valid
  if ( this->cf1_store_.get_valid() ) {
    if ( this->current_mode_ ) {
      this->last_period_current_1_ = this->last_period_current_;
      this->last_period_current_ = cf1_store_.get_last_period();
      this->last_sensed_current_ = this->period_to_current(this->last_period_current_);
      this->current_time_out_ = false;
    }
    else {
      this->last_period_voltage_1_ = this->last_period_voltage_;
      this->last_period_voltage_ = cf1_store_.get_last_period();
      this->last_sensed_voltage_ = this->period_to_voltage(this->last_period_voltage_);
    }
    this->change_mode();

  // check for timeout
  } else if ( (micros()-cf1_store_.get_last_rise()) > this->timeout_us_ ) {
    if ( this->current_mode_ ) {
      this->last_sensed_current_ = 0.0f;
      this->current_time_out_ = true;
  //    ESP_LOGD(TAG,"Current Timed Out");
    }
    else {
      this->last_sensed_voltage_ = 0.0f;
 //     ESP_LOGD(TAG,"Voltage Timed Out");
    }
    this->change_mode();
  }


  /////////////////////
  // calculate power //
  /////////////////////

  if ( cf_store_.get_valid() ) {
    this->last_period_power_1_ = this->last_period_power_;
    this->last_period_power_ = cf_store_.get_last_period();
    this->last_sensed_power_ = this->period_to_power(this->last_period_power_);
    this->cf_store_.clr_valid();
    this->power_time_out_ = false;
  }
  // if timeout has passed, consider power to be zero.
  else if ((micros()-cf_store_.get_last_rise()) > this->timeout_us_) {
    this->last_sensed_power_ = 0.0f;
    this->power_time_out_ = true;
//    ESP_LOGD(TAG,"Power Timed Out");
  }

  /////////////////////////////
  // check for early publish //
  /////////////////////////////

  // percent
  if ( this->do_early_publish_percent_ && this->enable_early_publish_ && (this->power_sensor_->state >= this->early_publish_percent_min_power_)) {

    // if increased or decreased by configured percentage since last published
    if ( ( this->last_sensed_power_ > (this->last_published_power_ * (1.0f+this->early_publish_percent_)) ) ||
         ( this->last_sensed_power_ < (this->last_published_power_ * (1.0f-this->early_publish_percent_)) ) ) {
      ESP_LOGD(TAG, "Pub early %");
      this->update(); }
  }

  // absolute value
  if ( this->do_early_publish_absolute_ && this->enable_early_publish_ ) {

    // if increased or decreased by configured absolute value since last published
    if ( ( this->last_sensed_power_ > (this->last_published_power_ + this->early_publish_absolute_) ) ||
         ( this->last_sensed_power_ < (this->last_published_power_ - this->early_publish_absolute_) ) ) {
      ESP_LOGD(TAG, "Pub early val");
      this->update();
    }
  }

}

void Kauf_HLW8012Component::update() {

  // don't update on first run through, for whatever reason ESPHome does this.
  if (this->nth_value_ == 0) {
    this->nth_value_++;
    return;
  }

  // if early publishing is disabled, just publish and skip following code.
  if (!this->enable_early_publish_) {
    this->actually_publish();
    return;
  }

  // if both power and current are timed out, publish all three no matter what.
  if ( this->power_time_out_ && this->current_time_out_ ) {
    this->actually_publish();
    return;
  }

  // if only one of power or current timed out, suspect error and don't publish anything.
  if ( this->power_time_out_ != this->current_time_out_ ) {
    return;
  }

  // only publish value if last 2 samples of all three sensors were within 33% of each other
  // issue we are trying to avoid is missing an edge results in a 200% period change then a 50% period change.
  uint32_t last_period_plus_33 = this->last_period_power_ + (this->last_period_power_ / 3);
  uint32_t last_period_less_33 = this->last_period_power_ - (this->last_period_power_ / 3);
  if ( (this->last_period_power_1_ < last_period_less_33) || (this->last_period_power_1_ > last_period_plus_33) ) { return; }

  last_period_plus_33 = this->last_period_current_ + (this->last_period_current_ / 3);
  last_period_less_33 = this->last_period_current_ - (this->last_period_current_ / 3);
  if ( (this->last_period_current_1_ < last_period_less_33) || (this->last_period_current_1_ > last_period_plus_33) ) { return; }
  last_period_plus_33 = this->last_period_voltage_ + (this->last_period_voltage_ / 3);

  last_period_less_33 = this->last_period_voltage_ - (this->last_period_voltage_ / 3);
  if ( (this->last_period_voltage_1_ < last_period_less_33) || (this->last_period_voltage_1_ > last_period_plus_33) ) { return; }

  this->actually_publish();
}

void Kauf_HLW8012Component::actually_publish() {
  // publish power if power sensor exists
  if ( this->power_sensor_ != nullptr ) {
    this->power_sensor_->publish_state(this->last_sensed_power_);
    this->last_published_power_ = this->last_sensed_power_;
    ESP_LOGV(TAG, "Raw power value being published: %f", this->last_published_power_);
  }

  // publish current if current sensor exists
  if ( this->current_sensor_ != nullptr ) {
    this->current_sensor_->publish_state(this->last_sensed_current_);
  }

  // publish voltage if voltage sensor exists
  if ( this->voltage_sensor_ != nullptr ) {
    this->voltage_sensor_->publish_state(this->last_sensed_voltage_);
  }
}

void Kauf_HLW8012Component::change_mode() {
    this->current_mode_ = !this->current_mode_;
    ESP_LOGVV(TAG, "Changing mode to %s mode", this->current_mode_ ? "CURRENT" : "VOLTAGE");
    this->sel_pin_->digital_write(this->current_mode_);
    this->cf1_store_.reset();
}

void Kauf_HLW8012Component::set_early_publish_percent(float percent_in){
  this->do_early_publish_percent_ = true;
  this->enable_early_publish_ = true;
  this->early_publish_percent_ = percent_in/100.0f;
}

void Kauf_HLW8012Component::set_early_publish_percent_min_power(float min_power_in){
  this->early_publish_percent_min_power_ = min_power_in;
}

void Kauf_HLW8012Component::set_early_publish_absolute(float absolute_in) {
  this->do_early_publish_absolute_ = true;
  this->enable_early_publish_ = true;
  this->early_publish_absolute_ = absolute_in;
}


}  // namespace kauf_hlw8012
}  // namespace esphome
