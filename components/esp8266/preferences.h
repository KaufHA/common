#pragma once

#ifdef USE_ESP8266

// KAUF: include global component so that a global variable can be used to set address
#include "esphome/components/globals/globals_component.h"

namespace esphome::esp8266 {

// KAUF: set start_free in setup_preferences
void setup_preferences(uint32_t start_free);
void preferences_prevent_write(bool prevent);

// KAUF: allow setting of global address variable
void set_global_addr(globals::GlobalsComponent<int> *ga_in);

}  // namespace esphome::esp8266

#endif  // USE_ESP8266
