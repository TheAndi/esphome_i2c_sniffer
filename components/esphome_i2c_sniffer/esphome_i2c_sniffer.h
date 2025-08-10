#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/text_sensor/text_sensor.h"
#include "esphome/sensor/sensor.h"

#define MAX_BLOCKS 4

namespace esphome {
namespace esphome_i2c_sniffer {

class AddressTrigger : public esphome::Trigger<uint8_t, std::vector<uint8_t>> {
 public:
  void trigger_(uint8_t address, const std::vector<uint8_t> &data) {
    this->publish(std::make_tuple(address, data));
  }
};

class EsphomeI2cSniffer : public Component {
 public:
  void set_sda_pin(uint8_t pin) { sda_pin_ = pin; }
  void set_scl_pin(uint8_t pin) { scl_pin_ = pin; }
  void register_msg_sensor(text_sensor::TextSensor *sensor) { msg_sensor_ = sensor; }
  void register_addr_sensor(sensor::Sensor *sensor) { last_addr_sensor_ = sensor; }
  void register_data_sensor(sensor::Sensor *sensor) { last_data_sensor_ = sensor; }
  void add_on_address_trigger(AddressTrigger *trigger) { this->address_triggers_.push_back(trigger); }

  void setup() override;
  void loop() override;

 protected:
  static void IRAM_ATTR on_scl();
  static void IRAM_ATTR on_sda();
  void publish_blocks_();

  struct Block {
    uint8_t addr;
    char addr_ack;
    char mode;
    uint8_t data[32];
    char data_ack[32];
    uint8_t data_len;
  };

  static volatile Block blocks_[MAX_BLOCKS];
  static volatile uint8_t block_count_;
  static volatile bool tr_ready_;
  static volatile uint8_t bit_count_;
  static volatile uint8_t byte_count_;
  static volatile uint8_t data_byte_;
  static volatile bool in_transfer_;

  uint8_t sda_pin_;
  uint8_t scl_pin_;
  text_sensor::TextSensor *msg_sensor_{nullptr};
  sensor::Sensor *last_addr_sensor_{nullptr};
  sensor::Sensor *last_data_sensor_{nullptr};
  std::vector<AddressTrigger*> address_triggers_;
};

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
