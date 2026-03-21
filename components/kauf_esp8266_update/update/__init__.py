import esphome.codegen as cg
from esphome.components import update
import esphome.config_validation as cv
from esphome.const import CONF_SOURCE
from esphome.components.http_request import CONF_HTTP_REQUEST_ID, HttpRequestComponent

AUTO_LOAD = ["json"]
DEPENDENCIES = ["http_request"]


kauf_esp8266_update_ns = cg.esphome_ns.namespace("kauf_esp8266_update")
KaufEsp8266Update = kauf_esp8266_update_ns.class_(
    "KaufEsp8266Update", update.UpdateEntity, cg.PollingComponent
)

CONF_RELEASE_URL = "release_url"
CONF_FIRMWARE_URL = "firmware_url"

CONFIG_SCHEMA = cv.All(
    cv.only_on_esp8266,
    update.update_schema(KaufEsp8266Update)
    .extend(
        {
            cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
            cv.Required(CONF_SOURCE): cv.url,
            cv.Required(CONF_RELEASE_URL): cv.url,
            cv.Optional(CONF_FIRMWARE_URL): cv.url,
        }
    )
    .extend(cv.polling_component_schema("6h")),
)


async def to_code(config):
    var = await update.new_update(config)
    request_parent = await cg.get_variable(config[CONF_HTTP_REQUEST_ID])
    cg.add(var.set_request_parent(request_parent))

    cg.add(var.set_source_url(config[CONF_SOURCE]))
    cg.add(var.set_release_url(config[CONF_RELEASE_URL]))
    if CONF_FIRMWARE_URL in config:
        cg.add(var.set_firmware_url(config[CONF_FIRMWARE_URL]))

    await cg.register_component(var, config)
