#pragma once

#if defined(USE_ARDUINO) || defined(USE_ESP32)

#include "esphome/core/component.h"

#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#endif

#ifdef USE_LIBRETINY
#include <WiFi.h>
#include <WiFiUdp.h>
#endif

#ifdef USE_ESP32
#include "esphome/components/socket/socket.h"
#endif

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace esphome {
namespace ddp {

class DDPLightEffectBase;

class DDPComponent : public esphome::Component {
 public:
  DDPComponent();
  ~DDPComponent();

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void add_effect(DDPLightEffectBase *light_effect);
  void remove_effect(DDPLightEffectBase *light_effect);
#ifdef USE_DDP_BACKGROUND_LISTEN
  void add_background_effect(DDPLightEffectBase *light_effect) { this->background_effects_.insert(light_effect); }
#endif
#ifdef USE_DDP_STATS
  void set_stats_interval(uint32_t interval_ms) {
    this->stats_interval_ms_ = interval_ms;
  }
#endif

 protected:

#ifdef USE_ESP32
  std::unique_ptr<socket::Socket> socket_;
#else
  std::unique_ptr<WiFiUDP> udp_;
#endif

  std::set<DDPLightEffectBase *> light_effects_;
#ifdef USE_DDP_BACKGROUND_LISTEN
  std::set<DDPLightEffectBase *> background_effects_;
#endif

  bool process_(const uint8_t *payload, uint16_t size);
  bool ensure_listening_();
  bool start_listening_();
  void stop_listening_();
#ifdef USE_ESP32
  bool is_listening_() const { return this->socket_ != nullptr; }
#else
  bool is_listening_() const { return this->udp_ != nullptr; }
#endif
#ifdef USE_DDP_BACKGROUND_LISTEN
  void update_background_effects_();
  void poll_effects_();
#endif
#ifdef USE_DDP_STATS
  void note_packet_(const char *source, uint16_t size);
  uint32_t stats_interval_ms_;
  uint32_t stats_last_ms_{0};
  uint32_t stats_packets_{0};
  uint16_t last_packet_size_{0};
  char last_source_[64]{};
  bool have_source_{false};
#endif
};

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ARDUINO || USE_ESP32
