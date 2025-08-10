import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import text_sensor, sensor
from esphome.const import CONF_ID

ns = cg.esphome_ns.namespace('esphome_i2c_sniffer')
EsphomeI2cSniffer = ns.class_('EsphomeI2cSniffer', cg.Component)
AddressTrigger = ns.class_('AddressTrigger', automation.Trigger)

CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"
CONF_MSG_SENSOR = "msg_sensor"
CONF_LAST_ADDRESS_SENSOR = "last_address_sensor"
CONF_LAST_DATA_SENSOR = "last_data_sensor"
CONF_ON_ADDRESS = "on_address"
CONF_TRIGGER_ID = "trigger_id"
CONF_ADDR_FILTER = "address"

ON_ADDRESS_TRIGGER_SCHEMA = automation.validate_automation({
    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(AddressTrigger),
    cv.Optional(CONF_ADDR_FILTER): cv.int_,
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EsphomeI2cSniffer),
    cv.Required(CONF_SDA_PIN): cv.int_,
    cv.Required(CONF_SCL_PIN): cv.int_,
    cv.Optional(CONF_MSG_SENSOR): text_sensor.text_sensor_schema(CONF_MSG_SENSOR),
    cv.Optional(CONF_LAST_ADDRESS_SENSOR): sensor.sensor_schema(CONF_LAST_ADDRESS_SENSOR),
    cv.Optional(CONF_LAST_DATA_SENSOR): sensor.sensor_schema(CONF_LAST_DATA_SENSOR),
    cv.Optional(CONF_ON_ADDRESS): cv.ensure_list(ON_ADDRESS_TRIGGER_SCHEMA),
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

    if CONF_ON_ADDRESS in config:
        for conf in config[CONF_ON_ADDRESS]:
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            if CONF_ADDR_FILTER in conf:
                cg.add(trigger.set_address_filter(conf[CONF_ADDR_FILTER]))
            await cg.register_component(trigger, conf)
            await automation.build_trigger(
                trigger,
                [
                    (cg.uint8, "address"),
                    (cg.std_vector.template(cg.uint8), "data"),
                ],
                conf,
            )
            cg.add(var.add_on_address_trigger(trigger))
