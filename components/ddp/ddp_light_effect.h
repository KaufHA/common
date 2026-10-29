#pragma once

#if defined(USE_ARDUINO) || defined(USE_ESP32)

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "ddp_light_effect_base.h"

namespace esphome {
namespace ddp {

enum DDPListenMode {
  DDP_LISTEN_EFFECT_ONLY = 0,
  DDP_LISTEN_WHEN_OFF = 1,
  DDP_LISTEN_ALWAYS = 2,
  DDP_LISTEN_YIELD_ON_CONTROL = 3,
};

class DDPLightEffect : public DDPLightEffectBase,
                       public light::LightEffect
#ifdef USE_DDP_BACKGROUND_LISTEN
                       , public light::LightRemoteValuesListener
#endif
{
 public:
  DDPLightEffect(const char *name);

  virtual esphome::StringRef get_name() const;

#ifdef USE_DDP_BACKGROUND_LISTEN
  void init() override;
#endif
  void start() override;
  void stop() override;
  void apply() override;
#ifdef USE_DDP_BACKGROUND_LISTEN
  void set_listen_mode(DDPListenMode listen_mode) { this->listen_mode_ = listen_mode; }
  void on_light_remote_values_update() override;
#endif

 protected:
  uint16_t process_(const uint8_t *payload, uint16_t size, uint16_t used) override;
#ifdef USE_DDP_BACKGROUND_LISTEN
  bool background_listen_required_() const override;
  void on_background_listen_stopped_() override;
  void poll_() override;
#endif
  void restore_remote_state_();
#ifdef USE_DDP_BACKGROUND_LISTEN
  DDPListenMode listen_mode_{DDP_LISTEN_EFFECT_ONLY};
  bool suspended_{false};
#endif
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP32
