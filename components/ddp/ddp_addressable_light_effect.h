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
  uint16_t process_(const uint8_t *payload, uint16_t size, uint16_t used) override;

  float scan_packet_and_return_multiplier_(const uint8_t *payload, uint16_t start, uint16_t end);
  float multiplier_from_max_val_(uint8_t max_val);
  void set_max_brightness_();

};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
