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
  block_count_ = 0; bit_count_ = 0; byte_count_ = 0; data_byte_ = 0;
  in_transfer_ = false; tr_ready_ = false;
  attachInterrupt(digitalPinToInterrupt(scl_pin_), on_scl, RISING);
  attachInterrupt(digitalPinToInterrupt(sda_pin_), on_sda, CHANGE);
}

void EsphomeI2cSniffer::loop() {
  if (tr_ready_) {
    publish_blocks_();
  }
}

void EsphomeI2cSniffer::publish_blocks_() {
  Block local[MAX_BLOCKS];
  uint8_t count;
  noInterrupts();
  count = block_count_;
  for (uint8_t i = 0; i <= count && i < MAX_BLOCKS; i++) {
    local[i] = blocks_[i];
  }
  tr_ready_ = false;
  interrupts();

  if (local[0].addr == 0) return;

  // Build printable string
  String out = "[" + String(millis()) + "ms] ";
  for (uint8_t b = 0; b <= count; b++) {
    const Block &blk = local[b];
    if (b) out += " | ";
    out += "[0x";
    if (blk.addr < 0x10) out += "0";
    String hexA = String(blk.addr, HEX); hexA.toUpperCase();
    out += hexA + "]" + blk.mode + blk.addr_ack;
    for (uint8_t i = 0; i < blk.data_len; i++) {
      out += "(";
      if (blk.data[i] < 0x10) out += "0";
      String hexD = String(blk.data[i], HEX); hexD.toUpperCase();
      out += hexD + ")" + blk.data_ack[i];
    }
  }
  ESP_LOGD("i2c_sniffer", "%s", out.c_str());
  if (msg_sensor_) msg_sensor_->publish_state(out.c_str());
  if (last_addr_sensor_) last_addr_sensor_->publish_state(local[count].addr);
  if (last_data_sensor_) {
    float v = local[count].data_len ? local[count].data[local[count].data_len - 1] : 0.0f;
    last_data_sensor_->publish_state(v);
  }

  // Trigger on_address callbacks
  for (auto *trig : address_triggers_) {
    std::vector<uint8_t> d(local[count].data, local[count].data + local[count].data_len);
    trig->trigger_(local[count].addr, d);
  }
}

void IRAM_ATTR EsphomeI2cSniffer::on_scl() {
  if (!in_transfer_) return;
  bool sda = digitalRead(GPIO_NUM_2);  // replace with sda_pin_ if constant allowed
  if (bit_count_ < 8) {
    data_byte_ = (data_byte_ << 1) | sda;
    bit_count_++;
  }
  if (bit_count_ == 8) {
    char ack = sda ? '-' : '+';
    Block &blk = blocks_[block_count_];
    if (byte_count_ == 0) {
      blk.addr = (data_byte_ >> 1) & 0x7F;
      blk.mode = (data_byte_ & 1) ? 'R' : 'W';
      blk.addr_ack = ack;
      blk.data_len = 0;
    } else if (blk.data_len < 32) {
      blk.data[blk.data_len] = data_byte_;
      blk.data_ack[blk.data_len++] = ack;
    }
    byte_count_++; bit_count_ = 0; data_byte_ = 0;
  }
}

void IRAM_ATTR EsphomeI2cSniffer::on_sda() {
  bool sda = digitalRead(sda_pin_), scl = digitalRead(scl_pin_);
  if (!sda && scl) {
    // START or repeated START
    if (in_transfer_ && block_count_ < MAX_BLOCKS - 1) block_count_++;
    else block_count_ = 0;
    blocks_[block_count_].addr = 0; blocks_[block_count_].addr_ack = '+';
    blocks_[block_count_].mode = 'W'; blocks_[block_count_].data_len = 0;
    bit_count_ = byte_count_ = data_byte_ = 0;
    in_transfer_ = true;
  } else if (sda && scl && in_transfer_) {
    // STOP
    in_transfer_ = false;
    tr_ready_ = true;
  }
}

}  // namespace esphome_i2c_sniffer
}  // namespace esphome
