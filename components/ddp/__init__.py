import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.light.types import AddressableLightEffect, LightEffect
from esphome.components.light.effects import (
    register_addressable_effect,
    register_rgb_effect,
)
from esphome.const import CONF_ID, CONF_NAME

DEPENDENCIES = ["network"]

ddp_ns = cg.esphome_ns.namespace("ddp")
DDPLightEffect = ddp_ns.class_("DDPLightEffect", LightEffect)
DDPAddressableLightEffect = ddp_ns.class_(
    "DDPAddressableLightEffect", AddressableLightEffect
)
DDPComponent = ddp_ns.class_("DDPComponent", cg.Component)

CONF_DDP_ID = "ddp_id"
CONF_DDP_TIMEOUT = "timeout"
CONF_DDP_DIS_GAMMA = "disable_gamma"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DDPComponent),
        }
    ),
    cv.only_with_arduino,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)


@register_rgb_effect(
    "ddp",
    DDPLightEffect,
    "DDP",
    {
        cv.GenerateID(CONF_DDP_ID): cv.use_id(DDPComponent),
        cv.Optional(CONF_DDP_TIMEOUT): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_DDP_DIS_GAMMA): cv.boolean,
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
    },
)
async def ddp_light_effect_to_code(config, effect_id):
    parent = await cg.get_variable(config[CONF_DDP_ID])

    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])

    cg.add(effect.set_ddp(parent))

    if CONF_DDP_TIMEOUT in config:
        cg.add(effect.set_timeout(config[CONF_DDP_TIMEOUT]))

    if CONF_DDP_DIS_GAMMA in config:
        cg.add(effect.set_disable_gamma(config[CONF_DDP_DIS_GAMMA]))

    return effect
