#include "hlw8012.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hlw8012 {

static const char *const TAG = "hlw8012";

// valid for HLW8012 and CSE7759
static const uint32_t HLW8012_CLOCK_FREQUENCY = 3579000;

void HLW8012Component::setup() {
  float reference_voltage = 0;
  ESP_LOGCONFIG(TAG, "Setting up HLW8012...");
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
void HLW8012Component::dump_config() {
  ESP_LOGCONFIG(TAG, "HLW8012:");
  LOG_PIN("  SEL Pin: ", this->sel_pin_)
  LOG_PIN("  CF Pin: ", this->cf_pin_)
  LOG_PIN("  CF1 Pin: ", this->cf1_pin_)
  ESP_LOGCONFIG(TAG, "  Change measurement mode every %u", this->change_mode_every_);
  ESP_LOGCONFIG(TAG, "  Current resistor: %.1f mΩ", this->current_resistor_ * 1000.0f);
  ESP_LOGCONFIG(TAG, "  Voltage Divider: %.1f", this->voltage_divider_);
  LOG_UPDATE_INTERVAL(this)
  LOG_SENSOR("  ", "Voltage", this->voltage_sensor_)
  LOG_SENSOR("  ", "Current", this->current_sensor_)
  LOG_SENSOR("  ", "Power", this->power_sensor_)
  LOG_SENSOR("  ", "Energy", this->energy_sensor_)
}
float HLW8012Component::get_setup_priority() const { return setup_priority::DATA; }

float HLW8012Component::period_to_power(float period_in) {
    float hz;

    if ( period_in == 0.0f ) { hz = 0.0f;}
    else                     { hz = 1 / (period_in / 1000000.0f); }

    return hz * this->power_multiplier_;
    // float power = hz * this->power_multiplier_;
    // ESP_LOGD("KAUF HLW 8012 UPDATE", "CF Valid: %d -- CF LAST: %d -- HZ: %f -- Power: %f", cf_store_.get_valid(), cf_store_.get_last_period(), hz, power);
    // return (power);
}

float HLW8012Component::period_to_current(float period_in) {
    float hz;

    if ( period_in == 0.0f ) { hz = 0.0f;}
    else                     { hz = 1 / (period_in / 1000000.0f); }

    // return hz * this->current_multiplier_;
    float current = hz * this->current_multiplier_;
    ESP_LOGD("KAUF HLW 8012 UPDATE", "CF1 Valid: %d -- CF1 LAST: %d -- HZ: %f -- Current: %f", cf1_store_.get_valid(), cf1_store_.get_last_period(), hz, current);
    return (current);
}


void HLW8012Component::update() {

  if (this->nth_value_ < 2) {
    this->nth_value_++
    return;
  }

  if (this->power_sensor_ != nullptr) {

    // if more than 10 seconds since last rising edge, consider power to be zero
    if ((micros()-cf_store_.get_last_rise()) > 10000000)  {
  //    ESP_LOGD("KAUF HLW 8012 UPDATE", "============ hard setting power to zero");
      this->power_sensor_->publish_state(0.0f);
    }

    // if more than update interval, publish as if edge occured now (if already zero, stay there)
    else if ( ((micros()-cf_store_.get_last_rise()) > (this->get_update_interval()*1000)) &&
              ((micros()-cf_store_.get_last_rise()) > cf_store_.get_last_period()) &&
              (this->power_sensor_->state != 0.0f) ) {
    //  ESP_LOGD("KAUF HLW 8012 UPDATE", "============ fading power down -- micros: %d -- last_rise: %d -- interval: %d ", micros(), cf_store_.get_last_rise(), this->get_update_interval());
      this->power_sensor_->publish_state(this->period_to_power(micros()-cf_store_.get_last_rise()));
    }

    // otherwise, publish actual value based on last period
    else {
      //ESP_LOGD("KAUF HLW 8012 UPDATE", "============ doing normal publish");
      this->power_sensor_->publish_state(this->period_to_power(cf_store_.get_last_period()));
    }

  }

  // do current if current sensor exists and in current mode
  if ( (this->current_sensor_ != nullptr) && this->current_mode_) {

    // if we don't have a valid period value, either stay at zero or scale down as if edge occurred now.
    if ( !cf1_store_.get_valid() ) {

      if ( ((micros()-cf1_store_.get_last_rise()) > (this->get_update_interval()*1000)) &&
           ((micros()-cf1_store_.get_last_rise()) > cf1_store_.get_last_period()) &&
           (this->current_sensor_->state != 0.0f) ) {
        this->power_sensor_->publish_state(this->period_to_power(micros()-cf1_store_.get_last_rise()));
      }

    }

  }

////  this->cf1_store_.reset();

  // HLW8012 has 50% duty cycle
  // pulse_counter::pulse_counter_t raw_cf = 0 //this->cf_store_.read_raw_value();
  // pulse_counter::pulse_counter_t raw_cf1 = 0 //this->cf1_store_.read_raw_value();
  // float cf_hz = raw_cf / (this->get_update_interval() / 1000.0f);
  // if (raw_cf <= 1) {
  //   // don't count single pulse as power
  //   cf_hz = 0.0f;
  // }
  // float cf1_hz = raw_cf1 / (this->get_update_interval() / 1000.0f);
  // if (raw_cf1 <= 1) {
  //   // don't count single pulse as anything
  //   cf1_hz = 0.0f;
  // }

//   if (this->nth_value_++ < 2) {
//     return;
//   }

// //  float power = cf_hz * this->power_multiplier_;

//   if (this->change_mode_at_ != 0) {
//     // Only read cf1 after one cycle. Apparently it's quite unstable after being changed.
//     if (this->current_mode_) {
//       float current = cf1_hz * this->current_multiplier_;
//       ESP_LOGD(TAG, "Got power=%.1fW, current=%.1fA", power, current);
//       if (this->current_sensor_ != nullptr) {
//         this->current_sensor_->publish_state(current);
//       }
//     } else {
//       float voltage = cf1_hz * this->voltage_multiplier_;
//       ESP_LOGD(TAG, "Got power=%.1fW, voltage=%.1fV", power, voltage);
//       if (this->voltage_sensor_ != nullptr) {
//         this->voltage_sensor_->publish_state(voltage);
//       }
//     }
//   }

//   if (this->power_sensor_ != nullptr) {
//     this->power_sensor_->publish_state(power);
//   }

//   if (this->energy_sensor_ != nullptr) {
//     cf_total_pulses_ += raw_cf;
//     float energy = cf_total_pulses_ * this->power_multiplier_ / 3600;
//     this->energy_sensor_->publish_state(energy);
//   }

//   if (this->change_mode_at_++ == this->change_mode_every_) {
//     this->current_mode_ = !this->current_mode_;
//     ESP_LOGV(TAG, "Changing mode to %s mode", this->current_mode_ ? "CURRENT" : "VOLTAGE");
//     this->change_mode_at_ = 0;
//     this->sel_pin_->digital_write(this->current_mode_);
//   }
}

}  // namespace hlw8012
}  // namespace esphome
