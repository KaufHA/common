#include "web_server_base.h"
#ifdef USE_NETWORK
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"

#ifdef USE_ARDUINO
#include <StreamString.h>

#include <string>

#if defined(USE_ESP32) || defined(USE_LIBRETINY)
#include <Update.h>
#endif
#ifdef USE_ESP8266
#include <Updater.h>
#endif
#endif

namespace esphome {
namespace web_server_base {

static const char *const TAG = "web_server_base";

void WebServerBase::add_handler(AsyncWebHandler *handler) {
  // remove all handlers

  if (!credentials_.username.empty()) {
    handler = new internal::AuthMiddlewareHandler(handler, &credentials_);
  }
  this->handlers_.push_back(handler);
  if (this->server_ != nullptr) {
    this->server_->addHandler(handler);
  }
}

void OTARequestHandler::report_ota_error() {
#ifdef USE_ARDUINO
  StreamString ss;
  Update.printError(ss);
  ESP_LOGW(TAG, "OTA Update failed! Error: %s", ss.c_str());
#endif
}

void OTARequestHandler::handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index,
                                     uint8_t *data, size_t len, bool final) {
#ifdef USE_ARDUINO
  bool success;

  std::string str = filename.c_str();

  // kill process if already errored out
  if ( this->kauf_ota_error_code != 0 ) {
    ESP_LOGD(TAG, "Last OTA try errored out; reboot firmware to try again.");
    return;
    }

  // kill process if "minimal" is found in string
  std::size_t found = str.find("minimal");
  if (found!=std::string::npos) {
     this->kauf_ota_error_code = 1;
     ESP_LOGD(TAG, "*****  DO NOT TRY TO FLASH TASMOTA-MINIMAL *****");
     return;
  }

  // kill process if "WLED" is found in string
  found = str.find("WLED");
  if (found!=std::string::npos) {
     this->kauf_ota_error_code = 2;
     ESP_LOGD(TAG, "*****  DO NOT TRY TO FLASH WLED *****");
     return;
  }

  // kill process if "wled" is found in string
  found = str.find("wled");
  if (found!=std::string::npos) {
     this->kauf_ota_error_code = 2;
     ESP_LOGD(TAG, "*****  DO NOT TRY TO FLASH WLED *****");
     return;
  }

  // remember whether a gzip file was used
  found = str.find(".gz");
  if (found!=std::string::npos) {
    last_gzip = true;
  }

  // if used, confirm filename does not conflict with sensor value
#ifdef SENSOR_4M
  found = str.find("-1m");
  if ( SENSOR_4M && (found!=std::string::npos) ) {
     ESP_LOGD(TAG, "*****  Apparently trying to flash 1M firmware over 4M version *****");
     this->kauf_ota_error_code = 3;
     return;
  }

  found = str.find("-4m");
  if ( !SENSOR_4M && (found!=std::string::npos) ) {
     ESP_LOGD(TAG, "*****  Apparently trying to flash 4M firmware over 1M version *****");
     this->kauf_ota_error_code = 3;
     return;
  }
#endif


  if (index == 0) {
    ESP_LOGI(TAG, "OTA Update Start: %s", filename.c_str());
    this->ota_read_length_ = 0;
#ifdef USE_ESP8266
    Update.runAsync(true);
    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    success = Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#endif
#if defined(USE_ESP32_FRAMEWORK_ARDUINO) || defined(USE_LIBRETINY)
    if (Update.isRunning()) {
      Update.abort();
    }
    success = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
#endif
    if (!success) {
      report_ota_error();
      return;
    }
  } else if (Update.hasError()) {
    // don't spam logs with errors if something failed at start
    return;
  }

  success = Update.write(data, len) == len;
  if (!success) {
    report_ota_error();
    return;
  }
  this->ota_read_length_ += len;

  const uint32_t now = millis();
  if (now - this->last_ota_progress_ > 1000) {
    if (request->contentLength() != 0) {
      float percentage = (this->ota_read_length_ * 100.0f) / request->contentLength();
      ESP_LOGD(TAG, "OTA in progress: %0.1f%%", percentage);
    } else {
      ESP_LOGD(TAG, "OTA in progress: %u bytes read", this->ota_read_length_);
    }
    this->last_ota_progress_ = now;
  }

  if (final) {
    if (Update.end(true)) {
      ESP_LOGI(TAG, "OTA update successful!");
      this->parent_->set_timeout(100, []() { App.safe_reboot(); });
    } else {
      report_ota_error();
    }
  }
#endif
}
void OTARequestHandler::handleRequest(AsyncWebServerRequest *request) {
#ifdef USE_ARDUINO
  if ( this->kauf_ota_error_code != 0 ) {
    // create response
    AsyncResponseStream *stream = request->beginResponseStream("text/html");
    stream->addHeader("Access-Control-Allow-Origin", "*");
    stream->print(F("<!DOCTYPE html><html><head><meta charset=UTF-8><link rel=icon href=data:></head><body>"));

    stream->print(F("Update Failed. -- "));

    if ( this->kauf_ota_error_code == 1) {
      stream->print(F("You appear to be trying to flash tasmota-minimal, which could brick the device.  Rename firmware file to not include the word <b>minimal</b> to override."));
    }
    if ( this->kauf_ota_error_code == 2) {
      stream->print(F("You appear to be trying to flash a WLED bin file, which could brick the device.  Rename firmware file to not include the word <b>wled</b> or <b>WLED</b> to override."));
    }
    if ( this->kauf_ota_error_code == 3) {
      stream->print(F("You appear to be trying to flash a mismatched update file, either -1m over -4m or -4m over -1m.  Download the proper update file or remove <b>-1m</b> or <b>-4m</b> from filename to override."));
    }

    stream->print(F("</body></html>"));
    request->send(stream);

    this->kauf_ota_error_code = 0;
  }
  else if (!Update.hasError()) {
    AsyncWebServerResponse *response;
    response = request->beginResponse(200, "text/plain", "Firmware file uploaded successfully.  The device will now process the firmware file and reboot itself.  You can try to connect to the device over Wi-Fi immediately, but don't power cycle for at least five minutes if the device is not reachable.");
    response->addHeader("Connection", "close");
    request->send(response);
  } else {

    // Get error string
    StreamString ss;
    Update.printError(ss);

    // create response
    AsyncResponseStream *stream = request->beginResponseStream("text/html");
    stream->addHeader("Access-Control-Allow-Origin", "*");
    stream->print(F("<!DOCTYPE html><html><head><meta charset=UTF-8><link rel=icon href=data:></head><body>"));

    stream->print(F("Update Failed.  This device is now restarting itself automatically, which could resolve the error in some cases.<br><br>"));
    stream->print(ss);
    if ( ss.find("Not Enough Space") && !last_gzip ) {
      stream->print(F(" - Using a .bin.gz file instead of a .bin file might help."));
    }

    stream->print(F("</body></html>"));
    request->send(stream);

    // reboot
    this->parent_->set_timeout(100, []() { App.safe_reboot(); });
  }
#endif
}

void WebServerBase::add_ota_handler() {
#ifdef USE_ARDUINO
  this->add_handler(new OTARequestHandler(this));  // NOLINT
#endif
}
float WebServerBase::get_setup_priority() const {
  // Before WiFi (captive portal)
  return setup_priority::WIFI + 2.0f;
}

}  // namespace web_server_base
}  // namespace esphome
#endif
