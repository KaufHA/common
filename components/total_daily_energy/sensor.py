import esphome.codegen as cg
from esphome.components import sensor, time
import esphome.config_validation as cv

# KAUF: add final validation for forced address
import esphome.final_validate as fv
from esphome.const import (
    CONF_ACCURACY_DECIMALS,
    CONF_ICON,
    CONF_ID,
    CONF_METHOD,
    CONF_RESTORE,
    CONF_TIME_ID,
    CONF_UNIT_OF_MEASUREMENT,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_TOTAL_INCREASING,
)
from esphome.core.entity_helpers import inherit_property_from

DEPENDENCIES = ["time"]

CONF_POWER_ID = "power_id"
total_daily_energy_ns = cg.esphome_ns.namespace("total_daily_energy")
TotalDailyEnergyMethod = total_daily_energy_ns.enum("TotalDailyEnergyMethod")
TOTAL_DAILY_ENERGY_METHODS = {
    "trapezoid": TotalDailyEnergyMethod.TOTAL_DAILY_ENERGY_METHOD_TRAPEZOID,
    "left": TotalDailyEnergyMethod.TOTAL_DAILY_ENERGY_METHOD_LEFT,
    "right": TotalDailyEnergyMethod.TOTAL_DAILY_ENERGY_METHOD_RIGHT,
}
TotalDailyEnergy = total_daily_energy_ns.class_(
    "TotalDailyEnergy", sensor.Sensor, cg.Component
)


def inherit_unit_of_measurement(uom, config):
    return uom + "h"


def inherit_accuracy_decimals(decimals, config):
    return decimals + 2


CONFIG_SCHEMA = (
    sensor.sensor_schema(
        TotalDailyEnergy,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    )
    .extend(
        {
            cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Required(CONF_POWER_ID): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_RESTORE, default=True): cv.boolean,
            cv.Optional("min_save_interval"): cv.invalid(
                "`min_save_interval` was removed in 2022.6.0. Please use the `preferences` -> `flash_write_interval` to adjust."
            ),
            cv.Optional(CONF_METHOD, default="right"): cv.enum(
                TOTAL_DAILY_ENERGY_METHODS, lower=True
            ),

            # KAUF: options for forced address / hash
            cv.Optional("forced_hash"): cv.int_,
            cv.Optional("forced_addr"): cv.int_,
            cv.Optional("global_addr"): cv.use_id(globals),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

# KAUF: validation for forced addr
def final_validate(config):
    if ("esp8266" in fv.full_config.get()):
        esp8266_config = fv.full_config.get()["esp8266"]
        if ( ("start_free" in esp8266_config) and ("forced_addr" in config)):
            if ( (esp8266_config["start_free"] <= config["forced_addr"] + 1) ):
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


FINAL_VALIDATE_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(TotalDailyEnergy),
            cv.Optional(CONF_ICON): cv.icon,
            cv.Optional(CONF_UNIT_OF_MEASUREMENT): sensor.validate_unit_of_measurement,
            cv.Optional(CONF_ACCURACY_DECIMALS): sensor.validate_accuracy_decimals,
            cv.Required(CONF_POWER_ID): cv.use_id(sensor.Sensor),
        },
        extra=cv.ALLOW_EXTRA,
    ),
    inherit_property_from(CONF_ICON, CONF_POWER_ID),
    inherit_property_from(
        CONF_UNIT_OF_MEASUREMENT, CONF_POWER_ID, transform=inherit_unit_of_measurement
    ),
    inherit_property_from(
        CONF_ACCURACY_DECIMALS, CONF_POWER_ID, transform=inherit_accuracy_decimals
    ),
    # KAUF: final validate
    final_validate,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    sens = await cg.get_variable(config[CONF_POWER_ID])
    cg.add(var.set_parent(sens))
    time_ = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_))
    cg.add(var.set_restore(config[CONF_RESTORE]))
    cg.add(var.set_method(config[CONF_METHOD]))

    # KAUF: set up forced addr/hash
    if "forced_hash" in config:
        cg.add(var.set_forced_hash(config["forced_hash"]))

    if "forced_addr" in config:
        cg.add(var.set_forced_addr(config["forced_addr"]))

    if "global_addr" in config:
        ga = await cg.get_variable(config["global_addr"])
        cg.add(var.set_global_addr(ga))
