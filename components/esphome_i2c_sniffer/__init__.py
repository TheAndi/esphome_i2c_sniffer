import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, sensor
from esphome.const import CONF_ID

# Stellt sicher, dass die zugehörigen C++-Teile eingebunden werden
AUTO_LOAD = ["sensor", "text_sensor"]

ns = cg.esphome_ns.namespace("esphome_i2c_sniffer")
EsphomeI2cSniffer = ns.class_("EsphomeI2cSniffer", cg.Component)

CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"
CONF_MSG_SENSOR = "msg_sensor"
CONF_LAST_ADDRESS_SENSOR = "last_address_sensor"
CONF_LAST_DATA_SENSOR = "last_data_sensor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(EsphomeI2cSniffer),
        cv.Required(CONF_SDA_PIN): cv.uint8_t,
        cv.Required(CONF_SCL_PIN): cv.uint8_t,
        cv.Optional(CONF_MSG_SENSOR): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_LAST_ADDRESS_SENSOR): sensor.sensor_schema(accuracy_decimals=0),
        cv.Optional(CONF_LAST_DATA_SENSOR): sensor.sensor_schema(accuracy_decimals=0),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_sda_pin(config[CONF_SDA_PIN]))
    cg.add(var.set_scl_pin(config[CONF_SCL_PIN]))

    if CONF_MSG_SENSOR in config:
        t = await text_sensor.new_text_sensor(config[CONF_MSG_SENSOR])
        cg.add(var.set_msg_sensor(t))

    if CONF_LAST_ADDRESS_SENSOR in config:
        a = await sensor.new_sensor(config[CONF_LAST_ADDRESS_SENSOR])
        cg.add(var.set_last_addr_sensor(a))

    if CONF_LAST_DATA_SENSOR in config:
        d = await sensor.new_sensor(config[CONF_LAST_DATA_SENSOR])
        cg.add(var.set_last_data_sensor(d))

    await cg.register_component(var, config)
