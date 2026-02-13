#include "total_daily_energy.h"
#include "esphome/core/log.h"
#include "esphome/components/esp8266/preferences.h"  // KAUF: included for set_next_forced_addr

namespace esphome {
namespace total_daily_energy {

static const char *const TAG = "total_daily_energy";

void TotalDailyEnergy::setup() {
  float initial_value = 0;

  if (this->restore_) {
    // KAUF: implement forced addr/hash
    if (this->forced_addr != 12345) esp8266::set_next_forced_addr(this->forced_addr);
    if (this->forced_hash != 0)
      this->pref_ = global_preferences->make_preference<float>(this->forced_hash);
    else
      this->pref_ = this->make_entity_preference<float>();

    this->pref_.load(&initial_value);
  }
  this->publish_state_and_save(initial_value);

  this->last_update_ = millis();

  this->parent_->add_on_state_callback([this](float state) { this->process_new_state_(state); });
}

void TotalDailyEnergy::dump_config() { LOG_SENSOR("", "Total Daily Energy", this); }

void TotalDailyEnergy::loop() {

  // KAUF: allow manually zeroing, which disables automatic zeroing.
  if (this->manual_zero) {
    this->manual_zero = false;
    this->total_energy_ = 0;
    this->publish_state_and_save(0);
  }

  if (this->manual_control)
    return;

  auto t = this->time_->now();
  if (!t.is_valid())
    return;

  if (this->last_day_of_year_ == 0) {
    this->last_day_of_year_ = t.day_of_year;
    return;
  }

  if (t.day_of_year != this->last_day_of_year_) {
    this->last_day_of_year_ = t.day_of_year;
    this->total_energy_ = 0;
    this->publish_state_and_save(0);
  }
}

void TotalDailyEnergy::publish_state_and_save(float state) {
  this->total_energy_ = state;
  this->publish_state(state);
  if (this->restore_) {
    this->pref_.save(&state);
  }
}

void TotalDailyEnergy::process_new_state_(float state) {
  if (std::isnan(state))
    return;
  const uint32_t now = millis();
  const float old_state = this->last_power_state_;
  const float new_state = state;
  float delta_hours = (now - this->last_update_) / 1000.0f / 60.0f / 60.0f;
  float delta_energy = 0.0f;
  switch (this->method_) {
    case TOTAL_DAILY_ENERGY_METHOD_TRAPEZOID:
      delta_energy = delta_hours * (old_state + new_state) / 2.0;
      break;
    case TOTAL_DAILY_ENERGY_METHOD_LEFT:
      delta_energy = delta_hours * old_state;
      break;
    case TOTAL_DAILY_ENERGY_METHOD_RIGHT:
      delta_energy = delta_hours * new_state;
      break;
  }
  this->last_power_state_ = new_state;
  this->last_update_ = now;
  this->publish_state_and_save(this->total_energy_ + delta_energy);
}

// KAUF: zero out total energy manually
void TotalDailyEnergy::zero_total_energy() {
  this->manual_zero = true;
  this->manual_control = true;
}

}  // namespace total_daily_energy
}  // namespace esphome
