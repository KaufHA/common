#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace ddp {

class DDPAddressableLightEffect : public DDPLightEffectBase, public light::AddressableLightEffect {
 public:
  DDPAddressableLightEffect(const std::string &name);

  const std::string &get_name() override;

  void start() override;
  void stop() override;

  void apply(light::AddressableLight &it, const Color &current_color) override;

 protected:
  uint16_t process(const uint8_t *payload, uint16_t size, uint16_t used) override;
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
