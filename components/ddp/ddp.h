#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

class UDP;

namespace esphome {
namespace ddp {

class DDPLightEffectBase;

struct DDP_Packet {
  uint8_t flags;
  uint8_t sequence_no;
  uint8_t data_type;
  uint8_t source_id;
  uint32_t data_offset;
  uint16_t data_length;
  uint8_t data_values[];
};

class DDPComponent : public esphome::Component {
 public:
  DDPComponent();
  ~DDPComponent();

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void add_effect(DDPLightEffectBase *light_effect);
  void remove_effect(DDPLightEffectBase *light_effect);

  void set_chain(bool chain=false) {
    this->chain_ = chain;
    if ( chain ) { this->tree_ = false; }
  }
  void set_tree(bool tree=false) {
    this->tree_ = tree;
    if ( tree ) { this->chain_ = false; }
  }

 protected:
  std::unique_ptr<UDP> udp_;
  std::set<DDPLightEffectBase *> light_effects_;

  bool process_(const uint8_t *payload, uint16_t size);


  bool forward_tree_(const uint8_t *payload, uint16_t size, uint16_t used) { return true; }
  bool forward_chain_(const uint8_t *payload, uint16_t size, uint16_t used) { return true; }

  bool chain_{false};
  bool tree_{false};
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO
