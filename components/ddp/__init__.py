import esphome.codegen as cg
from esphome.config_helpers import filter_source_files_from_platform
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.components.light.types import AddressableLightEffect, LightEffect
from esphome.components.light.effects import (
    register_addressable_effect,
    register_rgb_effect,
)
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    PLATFORM_BK72XX,
    PLATFORM_ESP32,
    PLATFORM_ESP8266,
    PLATFORM_LN882X,
    PLATFORM_RTL87XX,
    PlatformFramework,
)
from esphome.core import CORE


def AUTO_LOAD() -> list[str]:
    auto_load = []
    if CORE.is_esp32 and not CORE.using_arduino:
        auto_load.append("socket")
    return auto_load


DEPENDENCIES = ["network"]


def _consume_ddp_sockets(config):
    if CORE.is_esp32 and not CORE.using_arduino:
        from esphome.components import socket

        socket.consume_sockets(1, "ddp")(config)
    return config


def _normalize_active_sensor(config):
    active_sensor = config.get(CONF_ACTIVE_SENSOR, False)
    if active_sensor is False:
        return config
    if active_sensor is True:
        active_sensor = {}
    if CONF_NAME not in active_sensor:
        active_sensor = {**active_sensor, CONF_NAME: f"{config[CONF_NAME]} Active"}
    active_sensor = binary_sensor.binary_sensor_schema(binary_sensor.BinarySensor)(active_sensor)
    config[CONF_ACTIVE_SENSOR] = active_sensor
    return config

ddp_ns = cg.esphome_ns.namespace("ddp")
DDPLightEffect = ddp_ns.class_("DDPLightEffect", LightEffect)
DDPAddressableLightEffect = ddp_ns.class_(
    "DDPAddressableLightEffect", AddressableLightEffect
)
DDPComponent = ddp_ns.class_("DDPComponent", cg.Component)

DDP_SCALING = {
    "PIXEL":    ddp_ns.DDP_SCALE_PIXEL,
    "STRIP":    ddp_ns.DDP_SCALE_STRIP,
    "PACKET":   ddp_ns.DDP_SCALE_PACKET,
    "MULTIPLY": ddp_ns.DDP_SCALE_MULTIPLY,
    "NONE":     ddp_ns.DDP_NO_SCALING}

CONF_DDP_ID = "ddp_id"
CONF_DDP_TIMEOUT = "timeout"
CONF_DDP_DIS_GAMMA = "disable_gamma"
CONF_DDP_SCALING = "brightness_scaling"
CONF_ACTIVE_SENSOR = "active_sensor"
CONF_STATS_INTERVAL = "stats_interval"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DDPComponent),
            cv.Optional(
                CONF_STATS_INTERVAL, default="0s"
            ): cv.positive_time_period_milliseconds,
        }
    ),
    cv.only_on(
        [
            PLATFORM_ESP32,
            PLATFORM_ESP8266,
            PLATFORM_BK72XX,
            PLATFORM_LN882X,
            PLATFORM_RTL87XX,
        ]
    ),
    _consume_ddp_sockets,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_stats_interval(config[CONF_STATS_INTERVAL]))


@register_rgb_effect(
    "ddp",
    DDPLightEffect,
    "DDP",
    {
        cv.GenerateID(CONF_DDP_ID): cv.use_id(DDPComponent),
        cv.Optional(CONF_DDP_TIMEOUT): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_DDP_DIS_GAMMA): cv.boolean,
        cv.Optional(CONF_DDP_SCALING): cv.one_of(*DDP_SCALING, upper=True),
    },
)
@register_addressable_effect(
    "addressable_ddp",
    DDPAddressableLightEffect,
    "Addressable DDP",
    {
        cv.GenerateID(CONF_DDP_ID): cv.use_id(DDPComponent),
        cv.Optional(CONF_DDP_TIMEOUT): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_DDP_DIS_GAMMA): cv.boolean,
        cv.Optional(CONF_DDP_SCALING): cv.one_of(*DDP_SCALING, upper=True),
        cv.Optional(CONF_ACTIVE_SENSOR, default=False): cv.Any(
            cv.boolean,
            cv.Schema({}, extra=cv.ALLOW_EXTRA),
        ),
    },
    _normalize_active_sensor,
)
async def ddp_light_effect_to_code(config, effect_id):
    parent = await cg.get_variable(config[CONF_DDP_ID])

    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])

    cg.add(effect.set_ddp(parent))

    if CONF_DDP_TIMEOUT in config:
        cg.add(effect.set_timeout(config[CONF_DDP_TIMEOUT]))

    if CONF_DDP_DIS_GAMMA in config:
        cg.add(effect.set_disable_gamma(config[CONF_DDP_DIS_GAMMA]))

    if CONF_DDP_SCALING in config:
        cg.add(effect.set_scaling_mode(DDP_SCALING[config[CONF_DDP_SCALING]]))

    if config.get(CONF_ACTIVE_SENSOR):
        sensor = await binary_sensor.new_binary_sensor(config[CONF_ACTIVE_SENSOR])
        cg.add(effect.set_effect_active_sensor(sensor))

    return effect


FILTER_SOURCE_FILES = filter_source_files_from_platform(
    {
        "ddp_arduino.cpp": {
            PlatformFramework.ESP32_ARDUINO,
            PlatformFramework.ESP8266_ARDUINO,
            PlatformFramework.BK72XX_ARDUINO,
            PlatformFramework.RTL87XX_ARDUINO,
            PlatformFramework.LN882X_ARDUINO,
        },
        "ddp_esp32_idf.cpp": {PlatformFramework.ESP32_IDF},
    }
)
