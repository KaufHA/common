#include "ddp.h"
#ifdef USE_NETWORK
#include "ddp_light_effect_base.h"
#include "esphome/core/log.h"
#include "esphome/components/udp/udp_component.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static const int PORT = 4048;

DDPComponent::DDPComponent() {}
DDPComponent::~DDPComponent() {

}

void DDPComponent::setup() {
  // Create the UDP object
  this->udp_ = new udp::UDPComponent();
  this->udp_->set_component_source("udp");
  this->udp_->set_listen_port(4048);
  this->udp_->set_broadcast_port(4048);
  this->udp_->add_address("255.255.255.255");

  this->udp_->setup();       // start listening
}

void DDPComponent::loop() {

}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {


}

void DDPComponent::remove_effect(DDPLightEffectBase *light_effect) {

}

bool DDPComponent::process_(const uint8_t *payload, uint16_t size) {
  return true;
}

}  // namespace ddp
}  // namespace esphome
#endif