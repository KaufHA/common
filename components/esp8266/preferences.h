#pragma once
#ifdef USE_ESP8266

#include "esphome/core/preference_backend.h"

namespace esphome::esp8266 {

class ESP8266Preferences final : public PreferencesMixin<ESP8266Preferences> {
 public:
  using PreferencesMixin<ESP8266Preferences>::make_preference;
  void setup();

  // KAUF: Primary implementation — forced_addr=12345 means "no forced address, allocate sequentially"
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash, uint32_t forced_addr = 12345);
  ESPPreferenceObject make_preference(size_t length, uint32_t type) {
#ifdef USE_ESP8266_PREFERENCES_FLASH
    return this->make_preference(length, type, true);
#else
    return this->make_preference(length, type, false);
#endif
  }

  // KAUF: Typed template — decides in_flash automatically, with forced_addr
  template<typename T, enable_if_t<is_trivially_copyable<T>::value, bool> = true>
  ESPPreferenceObject make_preference(uint32_t type, uint32_t forced_addr) {
#ifdef USE_ESP8266_PREFERENCES_FLASH
    return this->make_preference(sizeof(T), type, true, forced_addr);
#else
    return this->make_preference(sizeof(T), type, false, forced_addr);
#endif
  }

  // KAUF: Typed template — explicit in_flash, with forced_addr
  template<typename T, enable_if_t<is_trivially_copyable<T>::value, bool> = true>
  ESPPreferenceObject make_preference(uint32_t type, bool in_flash, uint32_t forced_addr) {
    return this->make_preference(sizeof(T), type, in_flash, forced_addr);
  }

  bool sync();
  bool reset();

  uint32_t current_offset = 0;
  uint32_t current_flash_offset = 0;  // in words

  // KAUF: used as a bookmark for where free space starts
  uint32_t init_flash_offset;

};

// KAUF: set start_free in setup_preferences
void setup_preferences(uint32_t start_free);
void preferences_prevent_write(bool prevent);

}  // namespace esphome::esp8266

DECLARE_PREFERENCE_ALIASES(esphome::esp8266::ESP8266Preferences)

#endif  // USE_ESP8266
