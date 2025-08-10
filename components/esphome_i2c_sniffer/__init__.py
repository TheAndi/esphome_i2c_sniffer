import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor, sensor
from esphome.const import CONF_ID

esphome_i2c_sniffer_ns = cg.esphome_ns.namespace('esphome_i2c_sniffer')
EsphomeI2cSniffer = esphome_i2c_sniffer_ns.class_('EsphomeI2cSniffer', cg.Component)

CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"
CONF_MSG_SENSOR = "msg_sensor"
CONF_LAST_ADDRESS_SENSOR = "last_address_sensor"
CONF_LAST_DATA_SENSOR = "last_data_sensor"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EsphomeI2cSniffer),
    cv.Required(CONF_SDA_PIN): cv.int_,
    cv.Required(CONF_SCL_PIN): cv.int_,
    cv.Optional(CONF_MSG_SENSOR): text_sensor.text_sensor_schema,
    cv.Optional(CONF_LAST_ADDRESS_SENSOR): sensor.sensor_schema,
    cv.Optional(CONF_LAST_DATA_SENSOR): sensor.sensor_schema,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_sda_pin(config[CONF_SDA_PIN]))
    cg.add(var.set_scl_pin(config[CONF_SCL_PIN]))
    await cg.register_component(var, config)
    if CONF_MSG_SENSOR in config:
        sens = await text_sensor.new_text_sensor(config[CONF_MSG_SENSOR])
        cg.add(var.register_msg_sensor(sens))
    if CONF_LAST_ADDRESS_SENSOR in config:
        addr = await sensor.new_sensor(config[CONF_LAST_ADDRESS_SENSOR])
        cg.add(var.register_addr_sensor(addr))
    if CONF_LAST_DATA_SENSOR in config:
        data = await sensor.new_sensor(config[CONF_LAST_DATA_SENSOR])
        cg.add(var.register_data_sensor(data))
