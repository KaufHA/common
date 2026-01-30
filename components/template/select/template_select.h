#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/template_lambda.h"

namespace esphome::template_ {

class TemplateSelect final : public select::Select, public PollingComponent {
 public:
  template<typename F> void set_template(F &&f) { this->f_.set(std::forward<F>(f)); }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE + 1.0f; }

  Trigger<std::string> *get_set_trigger() const { return this->set_trigger_; }
  void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }
  void set_initial_option_index(size_t initial_option_index) { this->initial_option_index_ = initial_option_index; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

  // KAUF: forced addr/hash stuff (sentinel: 0 = use default hash, 12345 = don't force addr)
  uint32_t forced_hash = 0;
  uint32_t forced_addr = 12345;
  void set_forced_hash(uint32_t hash_value) { this->forced_hash = hash_value; }
  void set_forced_addr(uint32_t addr_value) { this->forced_addr = addr_value; }

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
