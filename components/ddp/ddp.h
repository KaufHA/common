#pragma once

#if defined(USE_ARDUINO) || defined(USE_ESP_IDF)

#include "esphome/core/component.h"

#ifdef USE_ARDUINO
#ifdef USE_ESP32
#include <WiFi.h>
#endif

#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#endif

#ifdef USE_LIBRETINY
#include <WiFi.h>
#include <WiFiUdp.h>
#endif
#endif  // USE_ARDUINO

#ifdef USE_ESP_IDF
#include "esphome/components/socket/socket.h"
#endif

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace esphome {
namespace ddp {

class DDPLightEffectBase;

class DDPComponent : public esphome::Component {
 public:
  DDPComponent();
  ~DDPComponent();

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void add_effect(DDPLightEffectBase *light_effect);
  void remove_effect(DDPLightEffectBase *light_effect);

 protected:
#ifdef USE_ARDUINO
  std::unique_ptr<WiFiUDP> udp_;
#endif
#ifdef USE_ESP_IDF
  std::unique_ptr<socket::Socket> socket_;
#endif
  std::set<DDPLightEffectBase *> light_effects_;

  bool process_(const uint8_t *payload, uint16_t size);
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP_IDF
