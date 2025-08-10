#include "esphome_i2c_sniffer.h"
#include "esphome/core/log.h"

namespace esphome {
namespace esphome_i2c_sniffer {

volatile EsphomeI2cSniffer::Block EsphomeI2cSniffer::blocks_[MAX_BLOCKS];
volatile uint8_t EsphomeI2cSniffer::block_count_ = 0;
volatile bool EsphomeI2cSniffer::tr_ready_ = false;
volatile uint8_t EsphomeI2cSniffer::bit_count_ = 0;
volatile uint8_t EsphomeI2cSniffer::byte_count_ = 0;
volatile uint8_t EsphomeI2cSniffer::data_byte_ = 0;
volatile bool EsphomeI2cSniffer::in_transfer_ = false;

void EsphomeI2cSniffer::setup() {
  pinMode(scl_pin_, INPUT_PULLUP);
  pinMode(sda_pin_, INPUT_PULLUP);
  block_count_ = 0;
  bit_count_ = 0;
  byte_count_ = 0;
  data_byte_ = 0;
  in_transfer_ = false;
  tr_ready_ = false;
  attachInterrupt(digitalPinToInterrupt(scl_pin_), on_scl, RISING);
  attachInterrupt(digitalPinToInterrupt(sda_pin_), on_sda, CHANGE);
}

void EsphomeI2cSniffer::loop() {
  if (tr_ready_) {
    publish_blocks_();
  }
}

void EsphomeI2cSniffer::publish_blocks_() {
  Block blocks_local[MAX_BLOCKS];
  uint8_t block_count_local;
  noInterrupts();
  block_count_local = block_count_;
  for (uint8_t i = 0; i <= block_count_local && i < MAX_BLOCKS; i++) {
    blocks_local[i].addr = blocks_[i].addr;
    blocks_local[i].addr_ack = blocks_[i].addr_ack;
    blocks_local[i].mode = blocks_[i].mode;
    blocks_local[i].data_len = blocks_[i].data_len;
    for (uint8_t j = 0; j < blocks_[i].data_len; j++) {
      blocks_local[i].data[j] = blocks_[i].data[j];
      blocks_local[i].data_ack[j] = blocks_[i].data_ack[j];
    }
  }
  tr_ready_ = false;
  interrupts();

  if (blocks_local[0].addr == 0) return;

  String out;
  unsigned long ts = millis();
  out += "[";
  out += ts;
  out += "ms] ";
  for (uint8_t b = 0; b <= block_count_local; b++) {
    const Block &blk = blocks_local[b];
    if (b > 0) out += " | ";
    out += "[0x";
    if (blk.addr < 0x10) out += "0";
    String temp = String(blk.addr, HEX);
    temp.toUpperCase();
    out += temp;
    out += "]";
    out += blk.mode;
    out += blk.addr_ack;
    for (uint8_t i = 0; i < blk.data_len; i++) {
      out += "(";
      if (blk.data[i] < 0x10) out += "0";
      String temp2 = String(blk.data[i], HEX);
      temp2.toUpperCase();
      out += temp2;
      out += ")";
      out += blk.data_ack[i];
    }
  }
  ESP_LOGD("esphome_i2c_sniffer", "%s", out.c_str());
  if (msg_sensor_ != nullptr) msg_sensor_->publish_state(out.c_str());
  if (last_addr_sensor_ != nullptr) last_addr_sensor_->publish_state(blocks_local[block_count_local].addr);
  if (last_data_sensor_ != nullptr) {
    float v = 0.0f;
    if (blocks_local[block_count_local].data_len > 0)
      v = blocks_local[block_count_local].data[blocks_local[block_count_local].data_len - 1];
    last_data_sensor_->publish_state(v);
  }
  if (cb_) {
    std::vector<uint8_t> data;
    for (uint8_t i = 0; i < blocks_local[block_count_local].data_len; i++) {
      data.push_back(blocks_local[block_count_local].data[i]);
    }
    cb_(blocks_local[block_count_local].addr, data);
  }
}

void IRAM_ATTR EsphomeI2cSniffer::on_scl() {
  if (!in_transfer_) return;
  bool sda = digitalRead(PIN_SDA);
  if (bit_count_ < 7) {
    data_byte_ = (data_byte_ << 1) | sda;
    bit_count_++;
  } else if (bit_count_ == 7) {
    data_byte_ = (data_byte_ << 1) | sda;
    bit_count_++;
  } else if (bit_count_ == 8) {
    char ack = sda ? '-' : '+';
    if (block_count_ < MAX_BLOCKS) {
      Block &blk = const_cast<Block &>(blocks_[block_count_]);
      if (byte_count_ == 0) {
        blk.addr = (data_byte_ >> 1) & 0x7F;
        blk.mode = (data_byte_ & 0x01) ? 'R' : 'W';
        blk.addr_ack = ack;
        blk.data_len = 0;
      } else {
        if (blk.data_len < 32) {
          blk.data[blk.data_len] = data_byte_;
          blk.data_ack[blk.data_len] = ack;
          blk.data_len++;
        }
      }
    }
    byte_count_++;
    bit_count_ = 0;
    data_byte_ = 0;
  }
}

void IRAM_ATTR EsphomeI2cSniffer::on_sda() {
  bool sda = digitalRead(PIN_SDA);
  bool scl = digitalRead(PIN_SCL);
  if (!sda && scl) {
    if (in_transfer_ && block_count_ < MAX_BLOCKS - 1) {
      block_count_++;
      if (block_count_ < MAX_BLOCKS) {
        blocks_[block_count_].addr = 0;
        blocks_[block_count_].addr_ack = '+';
        blocks_[block_count_].mode = 'W';
        blocks_[block_count_].data_len = 0;
      }
    } else {
      block_count_ = 0;
      if (block_count_ < MAX_BLOCKS) {
        blocks_[block_count_].addr = 0;
        blocks_[block_count_].addr_ack = '+';
        blocks_[block_count_].mode = 'W';
        blocks_[block_count_].data_len = 0;
      }
    }
    bit_count_ = 0;
    byte_count_ = 0;
    data_byte_ = 0;
    in_transfer_ = true;
  } else if (sda && scl) {
    if (in_transfer_) {
      in_transfer_ = false;
      tr_ready_ = true;
    }
  }
}

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
