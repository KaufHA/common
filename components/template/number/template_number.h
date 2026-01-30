#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/template_lambda.h"

namespace esphome::template_ {

class TemplateNumber final : public number::Number, public PollingComponent {
 public:
  template<typename F> void set_template(F &&f) { this->f_.set(std::forward<F>(f)); }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE + 1.0f; }

  Trigger<float> *get_set_trigger() const { return set_trigger_; }
  void set_optimistic(bool optimistic) { optimistic_ = optimistic; }
  void set_initial_value(float initial_value) { initial_value_ = initial_value; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

  // KAUF: forced addr/hash stuff
  uint32_t forced_hash = 0;
  uint32_t forced_addr = 12345;
  void set_forced_hash(uint32_t hash_value) { this->forced_hash = hash_value; }
  void set_forced_addr(uint32_t addr_value) { this->forced_addr = addr_value; }

 protected:
  void control(float value) override;
  bool optimistic_{false};
  float initial_value_{NAN};
  bool restore_value_{false};
  Trigger<float> *set_trigger_ = new Trigger<float>();
  TemplateLambda<float> f_;

  ESPPreferenceObject pref_;
};

}  // namespace esphome::template_
