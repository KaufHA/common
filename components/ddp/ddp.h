#pragma once
#include "esphome/core/defines.h"
#ifdef USE_NETWORK
#include "esphome/core/component.h"
#include "esphome/components/udp/udp_component.h"


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
  std::set<DDPLightEffectBase *> light_effects_;
  udp::UDPComponent *udp_;

  bool process_(const uint8_t *payload, uint16_t size);
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
