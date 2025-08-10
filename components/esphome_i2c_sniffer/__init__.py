import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import text_sensor, sensor
from esphome.const import CONF_ID

esphome_i2c_sniffer_ns = cg.esphome_ns.namespace('esphome_i2c_sniffer')
EsphomeI2cSniffer = esphome_i2c_sniffer_ns.class_('EsphomeI2cSniffer', cg.Component)

CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EsphomeI2cSniffer),
    cv.Required(CONF_SDA_PIN): cv.int_,
    cv.Required(CONF_SCL_PIN): cv.int_,
    cv.Optional("msg_sensor"): text_sensor.text_sensor_schema,
    cv.Optional("last_address_sensor"): sensor.sensor_schema,
    cv.Optional("last_data_sensor"): sensor.sensor_schema,
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_sda_pin(config[CONF_SDA_PIN]))
    cg.add(var.set_scl_pin(config[CONF_SCL_PIN]))
    await cg.register_component(var, config)
    if "msg_sensor" in config:
        sens = await text_sensor.new_text_sensor(config["msg_sensor"])
        cg.add(var.register_msg_sensor(sens))
    if "last_address_sensor" in config:
        addr = await sensor.new_sensor(config["last_address_sensor"])
        cg.add(var.register_addr_sensor(addr))
    if "last_data_sensor" in config:
        data = await sensor.new_sensor(config["last_data_sensor"])
        cg.add(var.register_data_sensor(data))
