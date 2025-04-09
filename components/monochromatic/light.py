import esphome.codegen as cg
from esphome.components import light, output
import esphome.config_validation as cv
from esphome.const import CONF_OUTPUT, CONF_OUTPUT_ID

monochromatic_ns = cg.esphome_ns.namespace("monochromatic")
MonochromaticLightOutput = monochromatic_ns.class_(
    "MonochromaticLightOutput", light.LightOutput
)

CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MonochromaticLightOutput),
        cv.Required(CONF_OUTPUT): cv.use_id(output.FloatOutput),
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

    out = await cg.get_variable(config[CONF_OUTPUT])
    cg.add(var.set_output(out))
