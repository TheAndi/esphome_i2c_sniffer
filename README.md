esphome_i2c_sniffer — Passive I²C Bus Sniffer for ESPHome

    Passively captures and decodes I²C transactions on any two GPIO pins (SDA/SCL), no ACK/NACK interference.

    Works as an ESPHome external component; can be installed via external_components from a Git repository.

    Publishes each decoded I²C transaction as a text sensor (block format, e.g. [40563ms] [0x54]W+(10)+ | [0x54]R+(00)+(00)-).

    Also exposes optional sensors for the last seen address and last data byte.

    Offers a callback interface (on_address) for automations or advanced triggers.

    Logs every I²C message as ESPHome DEBUG log line.

Features

    Multi-block, combined transactions (Register-Read with repeated start) supported and merged into a single line.

    Timestamped output for bus analysis.

    Strictly passive: does not drive or disturb the bus.

    All code and comments are in English only; only code-relevant comments.

Typical use

    Sniff DDC/CI, EDID, EEPROM, or any generic I²C communication (e.g., between HDMI source and display, or microcontrollers).

How to use

    Add as a Git external component in ESPHome.

    Configure the custom text sensor and (optionally) the two numeric sensors in your ESPHome YAML.

    Flash, then watch decoded I²C activity in Home Assistant and ESPHome logs.
