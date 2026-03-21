#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "esphome/components/http_request/http_request.h"
#include "esphome/components/update/update_entity.h"

namespace esphome::kauf_esp8266_update {

class KaufEsp8266Update final : public update::UpdateEntity, public PollingComponent {
 public:
  void setup() override;
  void update() override;

  void perform(bool force) override;
  void check() override { this->update(); }

  void set_source_url(const char *source_url) { this->source_url_ = source_url; }
  void set_release_url(const char *release_url) { this->release_url_ = release_url; }
  void set_firmware_url(const char *firmware_url) { this->firmware_url_ = firmware_url; }

  void set_request_parent(http_request::HttpRequestComponent *request_parent) { this->request_parent_ = request_parent; }

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  http_request::HttpRequestComponent *request_parent_;
  const char *source_url_{nullptr};
  const char *release_url_{nullptr};   // GitHub releases page — used by HA as "release notes" link
  const char *firmware_url_{nullptr};  // Direct firmware download — used by web UI

  static void update_task(void *params);
  uint8_t initial_check_remaining_{0};
  uint8_t retry_count_{0};
};

}  // namespace esphome::kauf_esp8266_update
