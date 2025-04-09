from esphome import automation
import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import (
    CONF_ID,
    CONF_INITIAL_OPTION,
    CONF_LAMBDA,
    CONF_OPTIMISTIC,
    CONF_OPTIONS,
    CONF_RESTORE_VALUE,
    CONF_SET_ACTION,
)

from .. import template_ns

TemplateSelect = template_ns.class_(
    "TemplateSelect", select.Select, cg.PollingComponent
)


def validate(config):
    if CONF_LAMBDA in config:
        if config[CONF_OPTIMISTIC]:
            raise cv.Invalid("optimistic cannot be used with lambda")
        if CONF_INITIAL_OPTION in config:
            raise cv.Invalid("initial_value cannot be used with lambda")
        if CONF_RESTORE_VALUE in config:
            raise cv.Invalid("restore_value cannot be used with lambda")
    elif CONF_INITIAL_OPTION in config:
        if config[CONF_INITIAL_OPTION] not in config[CONF_OPTIONS]:
            raise cv.Invalid(
                f"initial_option '{config[CONF_INITIAL_OPTION]}' is not a valid option [{', '.join(config[CONF_OPTIONS])}]"
            )
    else:
        config[CONF_INITIAL_OPTION] = config[CONF_OPTIONS][0]

    if not config[CONF_OPTIMISTIC] and CONF_SET_ACTION not in config:
        raise cv.Invalid(
            "Either optimistic mode must be enabled, or set_action must be set, to handle the option being set."
        )

    if "forced_addr" in config and "global_addr" not in config:
        raise cv.Invalid(
            "Forced_addr requires global_addr"
        )


    return config


CONFIG_SCHEMA = cv.All(
    select.select_schema(TemplateSelect)
    .extend(
        {
            cv.Required(CONF_OPTIONS): cv.All(
                cv.ensure_list(cv.string_strict), cv.Length(min=1)
            ),
            cv.Optional(CONF_LAMBDA): cv.returning_lambda,
            cv.Optional(CONF_OPTIMISTIC, default=False): cv.boolean,
            cv.Optional(CONF_SET_ACTION): automation.validate_automation(single=True),
            cv.Optional(CONF_INITIAL_OPTION): cv.string_strict,
            cv.Optional(CONF_RESTORE_VALUE): cv.boolean,
            cv.Optional("forced_hash"): cv.int_,
            cv.Optional("forced_addr"): cv.int_,
            cv.Optional("global_addr"): cv.use_id(globals),
        }
    )
    .extend(cv.polling_component_schema("60s")),
    validate,
)

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

FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=config[CONF_OPTIONS])

    if CONF_LAMBDA in config:
        template_ = await cg.process_lambda(
            config[CONF_LAMBDA], [], return_type=cg.optional.template(cg.std_string)
        )
        cg.add(var.set_template(template_))

    else:
        cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))
        cg.add(var.set_initial_option(config[CONF_INITIAL_OPTION]))

        if CONF_RESTORE_VALUE in config:
            cg.add(var.set_restore_value(config[CONF_RESTORE_VALUE]))

    if CONF_SET_ACTION in config:
        await automation.build_automation(
            var.get_set_trigger(), [(cg.std_string, "x")], config[CONF_SET_ACTION]
        )

    if "forced_hash" in config:
        cg.add(var.set_forced_hash(config["forced_hash"]))

    if "forced_addr" in config:
        cg.add(var.set_forced_addr(config["forced_addr"]))

    if "global_addr" in config:
        ga = await cg.get_variable(config["global_addr"])
        cg.add(var.set_global_addr(ga))
