import logging

import esphome.codegen as cg
from esphome.components import web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.config_helpers import filter_source_files_from_platform
import esphome.config_validation as cv
from esphome.const import (
    CONF_AP,
    CONF_COMPRESSION,
    CONF_ID,
    PLATFORM_BK72XX,
    PLATFORM_ESP32,
    PLATFORM_ESP8266,
    PLATFORM_LN882X,
    PLATFORM_RTL87XX,
    PlatformFramework,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.coroutine import CoroPriority
import esphome.final_validate as fv
from esphome.types import ConfigType

_LOGGER = logging.getLogger(__name__)
CONF_PRODUCT = "product"

PRODUCT_DEFINE_MAP = {
    "plf10": "KAUF_PRODUCT_PLF10",
    "plf12": "KAUF_PRODUCT_PLF12",
    "rgbww": "KAUF_PRODUCT_RGBWW",
    "rgbsw": "KAUF_PRODUCT_RGBSW",
}


def AUTO_LOAD() -> list[str]:
    auto_load = ["json", "web_server_base", "ota.web_server"]
    if CORE.is_esp32:
        auto_load.append("socket")
    return auto_load


DEPENDENCIES = ["wifi"]
CODEOWNERS = ["@esphome/core"]

captive_portal_ns = cg.esphome_ns.namespace("captive_portal")
CaptivePortal = captive_portal_ns.class_("CaptivePortal", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CaptivePortal),
            cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(
                web_server_base.WebServerBase
            ),
            cv.Optional(CONF_COMPRESSION, default="gzip"): cv.one_of("gzip", "br"),
            cv.Optional(CONF_PRODUCT): cv.one_of(*PRODUCT_DEFINE_MAP, lower=True),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on(
        [
            PLATFORM_ESP32,
            PLATFORM_ESP8266,
            PLATFORM_BK72XX,
            PLATFORM_LN882X,
            PLATFORM_RTL87XX,
        ]
    ),
)


def _final_validate(config: ConfigType) -> ConfigType:
    full_config = fv.full_config.get()
    wifi_conf = full_config.get("wifi")
    web_server_conf = full_config.get("web_server") or {}
    ws_product = web_server_conf.get(CONF_PRODUCT)
    cp_product = config.get(CONF_PRODUCT)

    if wifi_conf is None:
        # This shouldn't happen due to DEPENDENCIES = ["wifi"], but check anyway
        raise cv.Invalid("Captive portal requires the wifi component to be configured")

    if CONF_AP not in wifi_conf:
        _LOGGER.warning(
            "Captive portal is enabled but no WiFi AP is configured. "
            "The captive portal will not be accessible. "
            "Add 'ap:' to your WiFi configuration to enable the captive portal."
        )

    # Register socket needs for DNS server and additional HTTP connections
    # - 1 UDP socket for DNS server
    # - 3 additional TCP sockets for captive portal detection probes + configuration requests
    #   OS captive portal detection makes multiple probe requests that stay in TIME_WAIT.
    #   Need headroom for actual user configuration requests.
    #   LRU purging will reclaim idle sockets to prevent exhaustion from repeated attempts.
    from esphome.components import socket

    socket.consume_sockets(4, "captive_portal")(config)

    if ws_product is not None and cp_product is not None and ws_product != cp_product:
        raise cv.Invalid(
            f"captive_portal product '{cp_product}' conflicts with web_server product '{ws_product}'."
        )

    return config


FINAL_VALIDATE_SCHEMA = _final_validate


@coroutine_with_priority(CoroPriority.CAPTIVE_PORTAL)
async def to_code(config):
    paren = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])

    var = cg.new_Pvariable(config[CONF_ID], paren)
    await cg.register_component(var, config)
    cg.add_define("USE_CAPTIVE_PORTAL")

    if config[CONF_COMPRESSION] == "gzip":
        cg.add_define("USE_CAPTIVE_PORTAL_GZIP")
    if (product := config.get(CONF_PRODUCT)) is not None:
        cg.add_define(PRODUCT_DEFINE_MAP[product])

    if CORE.using_arduino:
        if CORE.is_esp8266:
            cg.add_library("DNSServer", None)
        if CORE.is_libretiny:
            cg.add_library("DNSServer", None)


# Only compile the ESP-IDF DNS server when using ESP-IDF framework
FILTER_SOURCE_FILES = filter_source_files_from_platform(
    {
        "dns_server_esp32_idf.cpp": {
            PlatformFramework.ESP32_ARDUINO,
            PlatformFramework.ESP32_IDF,
        },
    }
)
