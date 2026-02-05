#include "captive_portal.h"
#ifdef USE_CAPTIVE_PORTAL
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/components/json/json_util.h"
#include "captive_index.h"
#include "esphome/core/version.h"

namespace esphome {
namespace captive_portal {

static const char *const TAG = "captive_portal";

struct ProductUiMetadata {
  const char *display_name;
  const char *product_url;
  const char *update_url;
  const char *ota_warning;
};

static ProductUiMetadata get_product_ui_metadata() {
#if defined(KAUF_PRODUCT_PLF12)
  return ProductUiMetadata{"Plug (PLF12)", "https://kaufha.com/plf12", "https://github.com/KaufHA/PLF12/releases", ""};
#elif defined(KAUF_PRODUCT_PLF10)
  return ProductUiMetadata{"Plug (PLF10)", "https://kaufha.com/plf10", "https://github.com/KaufHA/PLF10/releases", ""};
#elif defined(KAUF_PRODUCT_RGBWW)
  return ProductUiMetadata{
      "RGBWW Bulb",
      "https://kaufha.com/blf10",
      "https://github.com/KaufHA/kauf-rgbww-bulbs/releases",
      "DO NOT USE ANY WLED BIN FILE. WLED is not going to work properly on this bulb. "
      "Use the included DDP functionality to control this bulb from another WLED instance "
      "or xLights."};
#elif defined(KAUF_PRODUCT_RGBSW)
  return ProductUiMetadata{
      "RGB Switch", "https://kaufha.com/srf10", "https://github.com/KaufHA/kauf-rgb-switch/releases", ""};
#else
  return ProductUiMetadata{"Unknown Product", "https://kaufha.com", "https://github.com/KaufHA", ""};
#endif
}

void CaptivePortal::handle_config(AsyncWebServerRequest *request) {
  json::JsonBuilder builder;
  JsonObject root = builder.root();
#ifdef ESPHOME_PROJECT_NAME
  const char *project_name = ESPHOME_PROJECT_NAME;
#else
  const char *project_name = "Kauf.unknown";
#endif
  ProductUiMetadata product_ui = get_product_ui_metadata();

  char mac_s[18];
  const char *mac_str = get_mac_address_pretty_into_buffer(mac_s);

  root[ESPHOME_F("mac")] = mac_str;
  root[ESPHOME_F("name")] = App.get_name();
  root[ESPHOME_F("esph_v")] = ESPHOME_VERSION;
  root[ESPHOME_F("soft_ssid")] = wifi::global_wifi_component->soft_ssid;
  root[ESPHOME_F("hard_ssid")] = wifi::global_wifi_component->hard_ssid;
  root[ESPHOME_F("free_sp")] = ESP.getFreeSketchSpace();
  char build_time_buffer[Application::BUILD_TIME_STR_SIZE];
  App.get_build_time_string(build_time_buffer);
  root[ESPHOME_F("build_ts")] = build_time_buffer;
  root[ESPHOME_F("proj_n")] = project_name;
#ifdef ESPHOME_PROJECT_VERSION
  root[ESPHOME_F("proj_v")] = ESPHOME_PROJECT_VERSION;
#else
  root[ESPHOME_F("proj_v")] = "unknown";
#endif

  JsonObject kauf_ui = root[ESPHOME_F("kauf_ui")].to<JsonObject>();
  kauf_ui[ESPHOME_F("display_name")] = product_ui.display_name;
  kauf_ui[ESPHOME_F("product_url")] = product_ui.product_url;
  kauf_ui[ESPHOME_F("update_url")] = product_ui.update_url;
  kauf_ui[ESPHOME_F("ota_warning")] = product_ui.ota_warning;

  JsonArray aps = root[ESPHOME_F("aps")].to<JsonArray>();
  for (auto &scan : wifi::global_wifi_component->get_scan_result()) {
    if (scan.get_is_hidden()) {
      continue;
    }

    JsonObject ap = aps.add<JsonObject>();
    ap[ESPHOME_F("ssid")] = scan.get_ssid();
    ap[ESPHOME_F("rssi")] = scan.get_rssi();
    ap[ESPHOME_F("lock")] = scan.get_with_auth();
  }

  std::string payload = builder.serialize();
  AsyncResponseStream *stream = request->beginResponseStream(ESPHOME_F("application/json"));
  stream->addHeader(ESPHOME_F("cache-control"), ESPHOME_F("public, max-age=0, must-revalidate"));
  stream->print(payload.c_str());
  request->send(stream);
}
void CaptivePortal::handle_wifisave(AsyncWebServerRequest *request) {
  std::string ssid = request->arg("ssid").c_str();  // NOLINT(readability-redundant-string-cstr)
  std::string psk = request->arg("psk").c_str();    // NOLINT(readability-redundant-string-cstr)
  ESP_LOGI(TAG,
           "Requested WiFi Settings Change:\n"
           "  SSID='%s'\n"
           "  Password=" LOG_SECRET("'%s'"),
           ssid.c_str(), psk.c_str());
  request->redirect(ESPHOME_F("/?save"));
#ifdef USE_ESP8266
  // Delay reboot briefly so HTTP response/redirect can flush to the client.
  this->set_timeout(250, [ssid, psk]() { wifi::global_wifi_component->save_wifi_sta(ssid, psk); });
#else
  // Defer save to main loop thread to avoid NVS operations from HTTP thread
  this->defer([ssid, psk]() { wifi::global_wifi_component->save_wifi_sta(ssid, psk); });
#endif
}

void CaptivePortal::setup() {
  // Disable loop by default - will be enabled when captive portal starts
  this->disable_loop();
}
void CaptivePortal::start() {
  this->base_->init();
  if (!this->initialized_) {
    this->base_->add_handler(this);
  }

  network::IPAddress ip = wifi::global_wifi_component->wifi_soft_ap_ip();

#if defined(USE_ESP32)
  // Create DNS server instance for ESP-IDF
  this->dns_server_ = make_unique<DNSServer>();
  this->dns_server_->start(ip);
#elif defined(USE_ARDUINO)
  this->dns_server_ = make_unique<DNSServer>();
  this->dns_server_->setErrorReplyCode(DNSReplyCode::NoError);
  this->dns_server_->start(53, ESPHOME_F("*"), ip);
#endif

  this->initialized_ = true;
  this->active_ = true;

  // Enable loop() now that captive portal is active
  this->enable_loop();

  ESP_LOGV(TAG, "Captive portal started");
}

void CaptivePortal::handleRequest(AsyncWebServerRequest *req) {
  if (req->url() == ESPHOME_F("/config.json")) {
    this->handle_config(req);
    return;
  } else if (req->url() == ESPHOME_F("/wifisave")) {
    this->handle_wifisave(req);
    return;
  }

  // All other requests get the captive portal page
  // This includes OS captive portal detection endpoints which will trigger
  // the captive portal when they don't receive their expected responses
#ifndef USE_ESP8266
  auto *response = req->beginResponse(200, ESPHOME_F("text/html"), INDEX_GZ, sizeof(INDEX_GZ));
#else
  auto *response = req->beginResponse_P(200, ESPHOME_F("text/html"), INDEX_GZ, sizeof(INDEX_GZ));
#endif
#ifdef USE_CAPTIVE_PORTAL_GZIP
  response->addHeader(ESPHOME_F("Content-Encoding"), ESPHOME_F("gzip"));
#else
  response->addHeader(ESPHOME_F("Content-Encoding"), ESPHOME_F("br"));
#endif
  req->send(response);
}

CaptivePortal::CaptivePortal(web_server_base::WebServerBase *base) : base_(base) { global_captive_portal = this; }
float CaptivePortal::get_setup_priority() const {
  // Before WiFi
  return setup_priority::WIFI + 1.0f;
}
void CaptivePortal::dump_config() { ESP_LOGCONFIG(TAG, "Captive Portal:"); }

CaptivePortal *global_captive_portal = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace captive_portal
}  // namespace esphome
#endif
