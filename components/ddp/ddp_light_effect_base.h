#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace ddp {

class DDPComponent;

enum DDPScalingMode { DDP_NO_SCALING     = 0,
                      DDP_SCALE_PIXEL    = 1,
                      DDP_SCALE_STRIP    = 2,
                      DDP_SCALE_PACKET   = 3,
                      DDP_SCALE_MULTIPLY = 4 };

class DDPLightEffectBase {
 public:
  DDPLightEffectBase();

  virtual const std::string &get_name() = 0;

  virtual void start();
  virtual void stop();
  bool timeout_check();

  void set_ddp(DDPComponent *ddp) { this->ddp_ = ddp; }
  void set_timeout(uint32_t timeout) {this->timeout_ = timeout;}
  void set_disable_gamma(bool disable_gamma) { this->disable_gamma_ = disable_gamma;}
  void set_scaling_mode(DDPScalingMode scaling_mode) { this->scaling_mode_ = scaling_mode;}

 protected:
  DDPComponent *ddp_{nullptr};

  uint32_t timeout_{10000};
  uint32_t last_ddp_time_ms_{0};

  bool disable_gamma_{true};
  float gamma_backup_{0.0};
  bool next_packet_will_be_first_{true};

  DDPScalingMode scaling_mode_{DDP_NO_SCALING};

  virtual uint16_t process_(const uint8_t *payload, uint16_t size, uint16_t used) = 0;

  friend class DDPComponent;
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
