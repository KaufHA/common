#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace ddp {

class DDPLightEffect : public DDPLightEffectBase, public light::LightEffect {
 public:
  DDPLightEffect(const std::string &name);

  const std::string &get_name() override;

  void start() override;
  void stop() override;
  void apply() override;

 protected:
  uint16_t process_(const uint8_t *payload, uint16_t size, uint16_t used) override;
};

}  // namespace ddp
}  // namespace esphome

#endif  // ESPHOME_DDP_LIGHT_EFFECT_H
