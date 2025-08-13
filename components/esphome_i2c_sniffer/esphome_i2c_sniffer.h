#pragma once

#include <vector>
#include <string>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace esphome_i2c_sniffer {

class EsphomeI2cSniffer : public Component {
 public:
  void set_sda_pin(uint8_t pin) { this->sda_pin_ = pin; }
  void set_scl_pin(uint8_t pin) { this->scl_pin_ = pin; }

  void set_msg_sensor(text_sensor::TextSensor *s) { this->msg_sensor_ = s; }
  void set_last_addr_sensor(sensor::Sensor *s) { this->last_addr_sensor_ = s; }
  void set_last_data_sensor(sensor::Sensor *s) { this->last_data_sensor_ = s; }

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  void IRAM_ATTR on_scl_edge_();
  void IRAM_ATTR on_sda_edge_();
  void publish_frame_();

  uint8_t sda_pin_{255};
  uint8_t scl_pin_{255};

  text_sensor::TextSensor *msg_sensor_{nullptr};
  sensor::Sensor *last_addr_sensor_{nullptr};
  sensor::Sensor *last_data_sensor_{nullptr};

  volatile bool in_transfer_{false};
  volatile bool last_sda_{true};
  volatile bool last_scl_{true};

  volatile uint8_t bit_idx_{0};
  volatile bool ack_phase_{false};
  volatile uint8_t cur_byte_{0};

  volatile bool have_addr_{false};
  volatile uint8_t addr_{0};
  volatile bool rw_{false};
  uint8_t data_[64];
  volatile uint8_t data_len_{0};

  volatile bool frame_ready_{false};
};

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
