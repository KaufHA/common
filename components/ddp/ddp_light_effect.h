#pragma once

#if defined(USE_ARDUINO) || defined(USE_ESP32)

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace ddp {

class DDPLightEffect : public DDPLightEffectBase, public light::LightEffect, public light::LightRemoteValuesListener {
 public:
  DDPLightEffect(const char *name);

  virtual esphome::StringRef get_name() const;

  void init() override;
  void start() override;
  void stop() override;
  void suspend() override;
  void apply() override;
  void on_light_remote_values_update() override;
  void set_listen_when_off(bool listen_when_off) { this->listen_when_off_ = listen_when_off; }

 protected:
  uint16_t process_(const uint8_t *payload, uint16_t size, uint16_t used) override;
  void poll_() override;
  void restore_remote_state_();
  bool listen_when_off_{false};
  bool ignore_next_ddp_selection_update_{false};
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP32
