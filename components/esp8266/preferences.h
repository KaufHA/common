#pragma once
#ifdef USE_ESP8266

#include "esphome/core/preference_backend.h"

namespace esphome::esp8266 {

class ESP8266Preferences final : public PreferencesMixin<ESP8266Preferences> {
 public:
  using PreferencesMixin<ESP8266Preferences>::make_preference;
  void setup();
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash);
  ESPPreferenceObject make_preference(size_t length, uint32_t type) {
#ifdef USE_ESP8266_PREFERENCES_FLASH
    return this->make_preference(length, type, true);
#else
    return this->make_preference(length, type, false);
#endif
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

// KAUF: set forced address for next make_preference call
void set_next_forced_addr(uint32_t addr);

}  // namespace esphome::esp8266

DECLARE_PREFERENCE_ALIASES(esphome::esp8266::ESP8266Preferences)

#endif  // USE_ESP8266
