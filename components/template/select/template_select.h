#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/template_lambda.h"

// KAUF: import globals component for forced hash/address
#include "esphome/components/globals/globals_component.h"

namespace esphome::template_ {

class TemplateSelect final : public select::Select, public PollingComponent {
 public:
  template<typename F> void set_template(F &&f) { this->f_.set(std::forward<F>(f)); }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  Trigger<std::string> *get_set_trigger() const { return this->set_trigger_; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void set_initial_option_index(size_t initial_option_index) { this->initial_option_index_ = initial_option_index; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

  // KAUF: forced addr/hash stuff
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
  void control(size_t index) override;
  bool optimistic_ = false;
  size_t initial_option_index_{0};
  bool restore_value_ = false;
  Trigger<std::string> *set_trigger_ = new Trigger<std::string>();
  TemplateLambda<std::string> f_;

  ESPPreferenceObject pref_;
};

}  // namespace esphome::template_
