#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace ddp {

class DDPComponent;

class DDPLightEffectBase {
 public:
  DDPLightEffectBase();

  virtual const std::string &get_name() = 0;

  virtual void start();
  virtual void stop();

  void set_ddp(DDPComponent *ddp) { this->ddp_ = ddp; }

 protected:
  DDPComponent *ddp_{nullptr};

  virtual uint16_t process(const uint8_t *payload, uint16_t size, uint16_t used) = 0;

  friend class DDPComponent;
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
