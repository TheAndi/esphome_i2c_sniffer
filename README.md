# esphome_i2c_sniffer
Passive I²C sniffer for ESPHome. Decodes I²C traffic on any pins, outputs each transaction (with timestamp) as text sensor and logs. Supports multi-block, register-read, last address/data sensors, and on_address callback. Purely passive, does not interfere with bus.
