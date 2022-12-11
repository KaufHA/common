#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"

#ifdef USE_ESP32
#include <WiFi.h>
#endif

#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
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

  void set_forward_chain(bool forward_chain) {
    this->forward_chain_ = forward_chain;
    if ( forward_chain ) { this->forward_tree_ = false; }
  }
  void set_forward_tree(bool forward_tree) {
    this->forward_tree_ = forward_tree;
    if ( forward_tree ) { this->forward_chain_ = false; }
  }

 protected:
  std::unique_ptr<WiFiUDP> udp_;
  std::set<DDPLightEffectBase *> light_effects_;

  bool process_(const uint8_t *payload, uint16_t size);

  // to be implemented
  bool do_forward_tree_(const uint8_t *payload, uint16_t size, uint16_t used) { return true; }
  bool do_forward_chain_(const uint8_t *payload, uint16_t size, uint16_t used) { return true; }

  bool forward_chain_{false};
  bool forward_tree_{false};
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
