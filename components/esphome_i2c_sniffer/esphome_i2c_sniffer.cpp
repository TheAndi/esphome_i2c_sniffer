#include "Arduino.h"  // attachInterruptArg, pinMode, digitalRead
#include <cstdio>
#include <cstring>
#include <string>
#include "esphome_i2c_sniffer.h"
#include "esphome/core/log.h"

namespace esphome {
namespace esphome_i2c_sniffer {

static const char *const TAG = "i2c_sniffer";

// ISR-Wrapper mit Arg-Pointer
void IRAM_ATTR scl_isr(void *arg) {
  reinterpret_cast<EsphomeI2cSniffer *>(arg)->on_scl_edge_();
}
void IRAM_ATTR sda_isr(void *arg) {
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

  // START: SDA fällt bei SCL=HIGH
  if (!sda && this->last_sda_ && scl) {
    this->in_transfer_ = true;
    this->bit_idx_ = 0;
    this->ack_phase_ = false;
    this->cur_byte_ = 0;
    this->have_addr_ = false;
    this->data_len_ = 0;
  }
  // STOP: SDA steigt bei SCL=HIGH
  else if (sda && !this->last_sda_ && scl) {
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
  // Sample auf steigender SCL-Flanke
  if (scl) {
    if (this->ack_phase_) {
      // ACK/NACK wird nicht ausgewertet
      this->ack_phase_ = false;
      this->bit_idx_ = 0;
      this->cur_byte_ = 0;
      return;
    }

    bool sda = digitalRead(this->sda_pin_);
    this->cur_byte_ = (this->cur_byte_ << 1) | (sda ? 1 : 0);
    this->bit_idx_++;

    if (this->bit_idx_ >= 8) {
      this->ack_phase_ = true;       // nächstes Bit ist ACK
      this->bit_idx_ = 0;

      if (!this->have_addr_) {
        this->addr_ = (this->cur_byte_ >> 1) & 0x7F;
        this->rw_ = (this->cur_byte_ & 0x01) != 0;
        this->have_addr_ = true;
      } else {
        if (this->data_len_ < MAX_DATA_) {
          this->data_[this->data_len_++] = this->cur_byte_;
        }
      }
      this->cur_byte_ = 0;
    }
  }

  this->last_scl_ = scl;
}

void EsphomeI2cSniffer::publish_frame_(uint8_t addr, bool rw, const uint8_t *data, uint8_t len) {
  char buf[16];
  std::string out;
  snprintf(buf, sizeof(buf), "ADDR 0x%02X %c", addr, rw ? 'R' : 'W');
  out += buf;

  if (len > 0) {
    out += " DATA:";
    for (uint8_t i = 0; i < len; i++) {
      snprintf(buf, sizeof(buf), " %02X", data[i]);
      out += buf;
    }
  }

  if (this->msg_sensor_ != nullptr) this->msg_sensor_->publish_state(out);
  if (this->last_addr_sensor_ != nullptr) this->last_addr_sensor_->publish_state(static_cast<float>(addr));
  if (this->last_data_sensor_ != nullptr) {
    uint8_t last = (len > 0) ? data[len - 1] : 0;
    this->last_data_sensor_->publish_state(static_cast<float>(last));
  }

  ESP_LOGD(TAG, "%s", out.c_str());
}

void EsphomeI2cSniffer::loop() {
  if (!this->frame_ready_)
    return;

  // Atomarer Snapshot, um Race-Conditions zu vermeiden
  uint8_t addr, len;
  bool rw;
  uint8_t data_snapshot[MAX_DATA_];

  noInterrupts();
  if (!this->frame_ready_) {  // evtl. schon von ISR zurückgesetzt
    interrupts();
    return;
  }
  this->frame_ready_ = false;
  addr = this->addr_;
  rw = this->rw_;
  len = this->data_len_;
  if (len > MAX_DATA_) len = MAX_DATA_;
  memcpy(data_snapshot, this->data_, len);
  interrupts();

  this->publish_frame_(addr, rw, data_snapshot, len);
}

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
