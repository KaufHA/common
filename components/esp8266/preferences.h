#pragma once

#ifdef USE_ESP8266

namespace esphome::esp8266 {

// KAUF: set start_free in setup_preferences
void setup_preferences(uint32_t start_free);
void preferences_prevent_write(bool prevent);

// KAUF: set forced address for next make_preference call
void set_next_forced_addr(uint32_t addr);

}  // namespace esphome::esp8266

#endif  // USE_ESP8266
