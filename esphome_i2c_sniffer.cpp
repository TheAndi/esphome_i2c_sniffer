#pragma once
#include "esphome.h"

#define MAX_BLOCKS 4

class EsphomeI2cSniffer : public Component, public TextSensor {
 public:
  EsphomeI2cSniffer(uint8_t pin_sda, uint8_t pin_scl);

  void setup() override;
  void loop() override;

  void set_last_address_sensor(Sensor *sensor);
  void set_last_data_sensor(Sensor *sensor);
  void set_on_address_callback(std::function<void(uint8_t address, const std::vector<uint8_t> &data)> cb);

 protected:
  static void IRAM_ATTR on_scl();
  static void IRAM_ATTR on_sda();

  struct Block {
    uint8_t addr;
    char addr_ack;
    char mode;
    uint8_t data[32];
    char data_ack[32];
    uint8_t data_len;
  };

  static volatile Block blocks[MAX_BLOCKS];
  static volatile uint8_t block_count;
  static volatile bool tr_ready;
  static volatile uint8_t bit_count;
  static volatile uint8_t byte_count;
  static volatile uint8_t data_byte;
  static volatile bool in_transfer;
  uint8_t pin_sda_;
  uint8_t pin_scl_;

  Sensor *last_address_sensor_;
  Sensor *last_data_sensor_;
  std::function<void(uint8_t address, const std::vector<uint8_t> &data)> cb_;

  void print_blocks();
};
