#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/template_lambda.h"

#include "esphome/components/globals/globals_component.h"

namespace esphome {
namespace template_ {

class TemplateNumber final : public number::Number, public PollingComponent {
 public:
  template<typename F> void set_template(F &&f) { this->f_.set(std::forward<F>(f)); }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  Trigger<float> *get_set_trigger() const { return set_trigger_; }
  void set_optimistic(bool optimistic) { optimistic_ = optimistic; }
  void set_initial_value(float initial_value) { initial_value_ = initial_value; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

  bool has_forced_hash = false;
  uint32_t forced_hash = 0;
  void set_forced_hash(uint32_t hash_value) {
    forced_hash = hash_value;
    has_forced_hash = true;
  }

  uint32_t forced_addr = 12345;
  void set_forced_addr(uint32_t addr_value) {
    forced_addr = addr_value;
  }

  bool has_global_forced_addr = false;
  globals::GlobalsComponent<int> *global_forced_addr;
  void set_global_addr(globals::GlobalsComponent<int> *ga_in) {
    has_global_forced_addr = true;
    global_forced_addr = ga_in;
  }

 protected:
  void control(float value) override;
  bool optimistic_{false};
  float initial_value_{NAN};
  bool restore_value_{false};
  Trigger<float> *set_trigger_ = new Trigger<float>();
  TemplateLambda<float> f_;

  ESPPreferenceObject pref_;
};

}  // namespace template_
}  // namespace esphome
