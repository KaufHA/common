#ifdef USE_ARDUINO

#include "ddp.h"
#include "ddp_addressable_light_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp_addressable_light_effect";

DDPAddressableLightEffect::DDPAddressableLightEffect(const std::string &name) : AddressableLightEffect(name) {}

const std::string &DDPAddressableLightEffect::get_name() { return AddressableLightEffect::get_name(); }

void DDPAddressableLightEffect::start() {
  AddressableLightEffect::start();
  DDPLightEffectBase::start();
}

void DDPAddressableLightEffect::stop() {
  DDPLightEffectBase::stop();
  AddressableLightEffect::stop();
}


void DDPAddressableLightEffect::apply(light::AddressableLight &it, const Color &current_color) { }

uint16_t DDPAddressableLightEffect::process(const uint8_t *payload, uint16_t size, uint16_t used) {
  auto *it = get_addressable_();

  ESP_LOGV(TAG, "Applying DDP data for '%s' (size: %d - used: %d)", get_name().c_str(), size, used);

  int num_pixels = std::min(it->size(), ((size-used)/3));

  for (uint16_t i = used+1; i < used+1+(num_pixels*3); i+=3) {
    auto output = (*it)[i];
    output.set(Color(payload[i], payload[i+1], payload[i+2], (payload[i] + payload[i+1] + payload[i+2]) / 3));
  }

  it->schedule_show();

  return (num_pixels*3);
}

}  // namespace e131
}  // namespace esphome

#endif  // USE_ARDUINO
