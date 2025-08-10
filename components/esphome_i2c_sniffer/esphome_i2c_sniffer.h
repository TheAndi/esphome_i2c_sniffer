#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"

#define MAX_BLOCKS 4

namespace esphome {
namespace esphome_i2c_sniffer {

class EsphomeI2cSniffer : public Component {
 public:
  void set_sda_pin(uint8_t pin) { sda_pin_ = pin; }
  void set_scl_pin(uint8_t pin) { scl_pin_ = pin; }
  void register_msg_sensor(text_sensor::TextSensor *sensor) { msg_sensor_ = sensor; }
  void register_addr_sensor(sensor::Sensor *sensor) { last_addr_sensor_ = sensor; }
  void register_data_sensor(sensor::Sensor *sensor) { last_data_sensor_ = sensor; }
  void set_on_address_callback(std::function<void(uint8_t, const std::vector<uint8_t>&)> cb) { cb_ = cb; }

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
  std::function<void(uint8_t, const std::vector<uint8_t>&)> cb_;
};

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
