#pragma once

#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/helpers.h"
#include "number_call.h"
#include "number_traits.h"

#include "esphome/components/globals/globals_component.h"
#include "esphome/core/version.h"

#if ESPHOME_VERSION_CODE < VERSION_CODE(2022, 6, 0)
  #error "Please Update ESPHome to the latest version.  Expecting 2022.6.X"
#endif

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2022, 7, 0)
  #error "KAUF external components have not been updated for this version of ESPHome yet, or you are not using the latest KAUF external components version.  Expecting 2022.6.X.  You can try deleting the .esphome/packages and .esphome/external_components subdirectories within the ESPHome config directory to resolve this."
#endif

namespace esphome {
namespace number {

#define LOG_NUMBER(prefix, type, obj) \
  if ((obj) != nullptr) { \
    ESP_LOGCONFIG(TAG, "%s%s '%s'", prefix, LOG_STR_LITERAL(type), (obj)->get_name().c_str()); \
    if (!(obj)->get_icon().empty()) { \
      ESP_LOGCONFIG(TAG, "%s  Icon: '%s'", prefix, (obj)->get_icon().c_str()); \
    } \
    if (!(obj)->traits.get_unit_of_measurement().empty()) { \
      ESP_LOGCONFIG(TAG, "%s  Unit of Measurement: '%s'", prefix, (obj)->traits.get_unit_of_measurement().c_str()); \
    } \
  }

class Number;

/** Base-class for all numbers.
 *
 * A number can use publish_state to send out a new value.
 */
class Number : public EntityBase {
 public:
  float state;

  void publish_state(float state);

  NumberCall make_call() { return NumberCall(this); }

  void add_on_state_callback(std::function<void(float)> &&callback);

  NumberTraits traits;

  /// Return whether this number has gotten a full state yet.
  bool has_state() const { return has_state_; }


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
  friend class NumberCall;

  /** Set the value of the number, this is a virtual method that each number integration must implement.
   *
   * This method is called by the NumberCall.
   *
   * @param value The value as validated by the NumberCall.
   */
  virtual void control(float value) = 0;

  CallbackManager<void(float)> state_callback_;
  bool has_state_{false};
};

}  // namespace number
}  // namespace esphome
