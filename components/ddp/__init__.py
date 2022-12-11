import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.light.types import AddressableLightEffect, LightEffect
from esphome.components.light.effects import (
    register_addressable_effect,
    register_rgb_effect,
)
from esphome.const import CONF_ID, CONF_NAME #, CONF_CHANNELS

DEPENDENCIES = ["network"]

ddp_ns = cg.esphome_ns.namespace("ddp")
DDPLightEffect = ddp_ns.class_("DDPLightEffect", LightEffect)
DDPAddressableLightEffect = ddp_ns.class_(
    "DDPAddressableLightEffect", AddressableLightEffect
)
DDPComponent = ddp_ns.class_("DDPComponent", cg.Component)

CONF_DDP_ID = "ddp_id"
CONF_DDP_CHAIN = "forward_chain"
CONF_DDP_TREE = "forward_tree"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DDPComponent),
            cv.Optional(CONF_DDP_CHAIN, default=False): cv.boolean,
            cv.Optional(CONF_DDP_TREE, default=False): cv.boolean,
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
    },
)
@register_addressable_effect(
    "addressable_ddp",
    DDPAddressableLightEffect,
    "Addressable DDP",
    {
        cv.GenerateID(CONF_DDP_ID): cv.use_id(DDPComponent),
    },
)
async def ddp_light_effect_to_code(config, effect_id):
    parent = await cg.get_variable(config[CONF_DDP_ID])

    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])

    # to be implemented
    # cg.add(effect.set_forward_tree(config[CONF_DDP_TREE]))
    # cg.add(effect.set_forward_chain(config[CONF_DDP_CHAIN]))

    cg.add(effect.set_ddp(parent))
    return effect
