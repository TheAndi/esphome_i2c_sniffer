#include "Arduino.h"  // stellt attachInterruptArg & pinMode bereit
#include "esphome_i2c_sniffer.h"
#include "esphome/core/log.h"
#include <cstdio>

namespace esphome {
namespace esphome_i2c_sniffer {

static const char *const TAG = "i2c_sniffer";

static void IRAM_ATTR scl_isr(void *arg) {
  reinterpret_cast<EsphomeI2cSniffer *>(arg)->on_scl_edge_();
}
static void IRAM_ATTR sda_isr(void *arg) {
  reinterpret_cast<EsphomeI2cSniffer *>(arg)->on_sda_edge_();
}

void EsphomeI2cSniffer::setup() {
  pinMode(this->scl_pin_, INPUT_PULLUP);
  pinMode(this->sda_pin_, INPUT_PULLUP);

  this->last_scl_ = digitalRead(this->scl_pin_);
  this->last_sda_ = digitalRead(this->sda_pin_);

  attachInterruptArg(this->scl_pin_, scl_isr, this, CHANGE);
  attachInterruptArg(this->sda_pin_, sda_isr, this, CHANGE);

  ESP_LOGI(TAG, "I2C sniffer on SDA=%u, SCL=%u", this->sda_pin_, this->scl_pin_);
}

void EsphomeI2cSniffer::dump_config() {
  ESP_LOGCONFIG(TAG, "I2C Sniffer:");
  ESP_LOGCONFIG(TAG, "  SDA Pin: %u", this->sda_pin_);
  ESP_LOGCONFIG(TAG, "  SCL Pin: %u", this->scl_pin_);
}

void IRAM_ATTR EsphomeI2cSniffer::on_sda_edge_() {
  bool sda = digitalRead(this->sda_pin_);
  bool scl = digitalRead(this->scl_pin_);

  if (!sda && this->last_sda_ && scl) {  // START
    this->in_transfer_ = true;
    this->bit_idx_ = 0;
    this->ack_phase_ = false;
    this->cur_byte_ = 0;
    this->have_addr_ = false;
    this->data_len_ = 0;
  } else if (sda && !this->last_sda_ && scl) {  // STOP
    if (this->in_transfer_) {
      this->in_transfer_ = false;
      this->frame_ready_ = true;
    }
  }

  this->last_sda_ = sda;
}

void IRAM_ATTR EsphomeI2cSniffer::on_scl_edge_() {
  if (!this->in_transfer_) {
    this->last_scl_ = digitalRead(this->scl_pin_);
    return;
  }

  bool scl = digitalRead(this->scl_pin_);
  if (scl) {  // Sample auf steigender SCL-Flanke
    if (this->ack_phase_) {
      this->ack_phase_ = false;
      this->bit_idx_ = 0;
      this->cur_byte_ = 0;
      return;
    }

    bool sda = digitalRead(this->sda_pin_);
    this->cur_byte_ = (this->cur_byte_ << 1) | (sda ? 1 : 0);
    this->bit_idx_++;

    if (this->bit_idx_ >= 8) {
      this->ack_phase_ = true;
      this->bit_idx_ = 0;

      if (!this->have_addr_) {
        this->addr_ = (this->cur_byte_ >> 1) & 0x7F;
        this->rw_ = (this->cur_byte_ & 0x01) != 0;
        this->have_addr_ = true;
      } else {
        if (this->data_len_ < sizeof(this->data_)) {
          this->data_[this->data_len_++] = this->cur_byte_;
        }
      }
      this->cur_byte_ = 0;
    }
  }

  this->last_scl_ = scl;
}

void EsphomeI2cSniffer::publish_frame_() {
  char buf[16];
  std::string out;
  snprintf(buf, sizeof(buf), "ADDR 0x%02X %c", this->addr_, this->rw_ ? 'R' : 'W');
  out += buf;

  if (this->data_len_ > 0) {
    out += " DATA:";
    for (uint8_t i = 0; i < this->data_len_; i++) {
      snprintf(buf, sizeof(buf), " %02X", this->data_[i]);
      out += buf;
    }
  }

  if (this->msg_sensor_ != nullptr)
    this->msg_sensor_->publish_state(out);

  if (this->last_addr_sensor_ != nullptr)
    this->last_addr_sensor_->publish_state(static_cast<float>(this->addr_));

  if (this->last_data_sensor_ != nullptr) {
    uint8_t last = (this->data_len_ > 0) ? this->data_[this->data_len_ - 1] : 0;
    this->last_data_sensor_->publish_state(static_cast<float>(last));
  }

  ESP_LOGD(TAG, "%s", out.c_str());
}

void EsphomeI2cSniffer::loop() {
  if (!this->frame_ready_)
    return;

  noInterrupts();
  bool ready = this->frame_ready_;
  this->frame_ready_ = false;
  interrupts();

  if (ready)
    this->publish_frame_();
}

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
