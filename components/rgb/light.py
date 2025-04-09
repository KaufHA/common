import esphome.codegen as cg
from esphome.components import light, output
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import CONF_BLUE, CONF_GREEN, CONF_OUTPUT_ID, CONF_RED

rgb_ns = cg.esphome_ns.namespace("rgb")
RGBLightOutput = rgb_ns.class_("RGBLightOutput", light.LightOutput)

CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(RGBLightOutput),
        cv.Required(CONF_RED): cv.use_id(output.FloatOutput),
        cv.Required(CONF_GREEN): cv.use_id(output.FloatOutput),
        cv.Required(CONF_BLUE): cv.use_id(output.FloatOutput),
    }
)


def final_validate(config):
    if ("esp8266" in fv.full_config.get()):
        esp8266_config = fv.full_config.get()["esp8266"]
        if ( ("start_free" in esp8266_config) and ("forced_addr" in config)):
            if ( (esp8266_config["start_free"] <= config["forced_addr"] + 11) ):
                start_free_num = esp8266_config["start_free"]
                forced_addr_num = config["forced_addr"]
                raise cv.Invalid(
                    f"Forced address ({forced_addr_num}) conflicts with esp8266: start_free ({start_free_num})"
                )
    else:
        if ("forced_addr" in config):
            raise cv.Invalid(
                "Forced_addr is only compatible with esp8266 platform"
            )

    if "forced_addr" in config and "global_addr" not in config:
        raise cv.Invalid(
            "Forced_addr requires global_addr"
        )

FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    red = await cg.get_variable(config[CONF_RED])
    cg.add(var.set_red(red))
    green = await cg.get_variable(config[CONF_GREEN])
    cg.add(var.set_green(green))
    blue = await cg.get_variable(config[CONF_BLUE])
    cg.add(var.set_blue(blue))
