#pragma once

#if defined(USE_ARDUINO) || defined(USE_ESP_IDF)

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace binary_sensor {
class BinarySensor;
}  // namespace binary_sensor
}  // namespace esphome

namespace esphome {
namespace ddp {

class DDPAddressableLightEffect : public DDPLightEffectBase, public light::AddressableLightEffect {
 public:
  DDPAddressableLightEffect(const char *name);

  virtual esphome::StringRef get_name() const;  

  void start() override;
  void stop() override;

  void apply(light::AddressableLight &it, const Color &current_color) override;

#ifdef USE_BINARY_SENSOR
  void set_effect_active_sensor(binary_sensor::BinarySensor *sensor);
#endif

 protected:
  uint16_t process_(const uint8_t *payload, uint16_t size, uint16_t used) override;

  float scan_packet_and_return_multiplier_(const uint8_t *payload, uint16_t start, uint16_t end);
  float multiplier_from_max_val_(uint8_t max_val);
  void set_max_brightness_();
  void set_effect_active_(light::AddressableLight *it, bool active);

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *effect_active_sensor_{nullptr};
#endif
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP_IDF
