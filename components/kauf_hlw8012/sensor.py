import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor
from esphome.const import (
    CONF_CURRENT,
    CONF_CURRENT_RESISTOR,
    CONF_ID,
    CONF_POWER,
    CONF_SEL_PIN,
    CONF_MODEL,
    CONF_VOLTAGE,
    CONF_VOLTAGE_DIVIDER,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_WATT,
    UNIT_WATT_HOURS,
)

kauf_hlw8012_ns = cg.esphome_ns.namespace("kauf_hlw8012")
Kauf_HLW8012Component = kauf_hlw8012_ns.class_("Kauf_HLW8012Component", cg.PollingComponent)
HLW8012SensorModels = kauf_hlw8012_ns.enum("HLW8012SensorModels")

MODELS = {
    "HLW8012": HLW8012SensorModels.HLW8012_SENSOR_MODEL_HLW8012,
    "CSE7759": HLW8012SensorModels.HLW8012_SENSOR_MODEL_CSE7759,
    "BL0937": HLW8012SensorModels.HLW8012_SENSOR_MODEL_BL0937,
}

CONF_CF1_PIN = "cf1_pin"
CONF_CF_PIN = "cf_pin"
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Kauf_HLW8012Component),
        cv.Required(CONF_SEL_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_CF_PIN): cv.All(pins.internal_gpio_input_pullup_pin_schema),
        cv.Required(CONF_CF1_PIN): cv.All(pins.internal_gpio_input_pullup_pin_schema),
        cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT_RESISTOR, default=0.001): cv.resistance,
        cv.Optional(CONF_VOLTAGE_DIVIDER, default=2351): cv.positive_float,
        cv.Optional(CONF_MODEL, default="HLW8012"): cv.enum(MODELS, upper=True),
        cv.Optional("timeout", default="9s"): cv.positive_time_period_milliseconds,
        cv.Optional("early_publish_percent"): cv.positive_float,
        cv.Optional("early_publish_percent_min_power"): cv.positive_float,
        cv.Optional("early_publish_absolute"): cv.positive_float,
    }
).extend(cv.polling_component_schema("60s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    sel = await cg.gpio_pin_expression(config[CONF_SEL_PIN])
    cg.add(var.set_sel_pin(sel))
    cf = await cg.gpio_pin_expression(config[CONF_CF_PIN])
    cg.add(var.set_cf_pin(cf))
    cf1 = await cg.gpio_pin_expression(config[CONF_CF1_PIN])
    cg.add(var.set_cf1_pin(cf1))

    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_voltage_sensor(sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))

    if "early_publish_percent" in config:
        cg.add(var.set_early_publish_percent(config["early_publish_percent"]))
    if "early_publish_percent_min_power" in config:
        cg.add(var.set_early_publish_percent_min_power(config["early_publish_percent_min_power"]))
    if "early_publish_absolute" in config:
        cg.add(var.set_early_publish_absolute(config["early_publish_absolute"]))

    cg.add(var.set_current_resistor(config[CONF_CURRENT_RESISTOR]))
    cg.add(var.set_voltage_divider(config[CONF_VOLTAGE_DIVIDER]))
    cg.add(var.set_sensor_model(config[CONF_MODEL]))

    cg.add(var.set_timeout(config["timeout"]))
