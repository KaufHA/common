#include "template_number.h"
#include "esphome/core/log.h"
#ifdef USE_ESP8266
#include "esphome/components/esp8266/preferences.h"  // KAUF: included for set_next_forced_addr
#endif

namespace esphome::template_ {

static const char *const TAG = "template.number";

void TemplateNumber::setup() {
  if (this->f_.has_value())
    return;

  float value;
  if (!this->restore_value_) {
    value = this->initial_value_;
  } else {

    // KAUF: forced addr/hash support
#ifdef USE_ESP8266
    if (this->forced_addr != 12345) esp8266::set_next_forced_addr(this->forced_addr);
#endif
    if (this->forced_hash != 0)
      this->pref_ = global_preferences->make_preference<float>(this->forced_hash);
    else
      this->pref_ = this->make_entity_preference<float>();

    if (!this->pref_.load(&value)) {
      if (!std::isnan(this->initial_value_)) {
        value = this->initial_value_;
      } else {
        value = this->traits.get_min_value();
      }
    }
  }
  this->publish_state(value);
}

void TemplateNumber::update() {
  if (!this->f_.has_value())
    return;

  auto val = this->f_();
  if (val.has_value()) {
    this->publish_state(*val);
  }
}

void TemplateNumber::control(float value) {
  this->set_trigger_.trigger(value);

  if (this->optimistic_)
    this->publish_state(value);

  if (this->restore_value_)
    this->pref_.save(&value);
}
void TemplateNumber::dump_config() {
  LOG_NUMBER("", "Template Number", this);
  ESP_LOGCONFIG(TAG, "  Optimistic: %s", YESNO(this->optimistic_));
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::template_
