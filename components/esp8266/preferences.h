#pragma once

#ifdef USE_ESP8266

#include "esphome/components/globals/globals_component.h"

namespace esphome::esp8266 {

void setup_preferences(uint32_t start_free);
void preferences_prevent_write(bool prevent);

void set_global_addr(globals::GlobalsComponent<int> *ga_in);

}  // namespace esphome::esp8266

#endif  // USE_ESP8266
