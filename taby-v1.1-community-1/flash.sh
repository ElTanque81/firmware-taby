#!/usr/bin/env bash
# Flash modified Taby (V1.1) onto a Waveshare ESP32-S3-Touch-AMOLED-1.64 V1.1.
# usage: bash flash.sh /dev/cu.usbmodemXXX
set -e
PORT="${1:?port, e.g. /dev/cu.usbmodem101}"
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 write-flash \
  0x0 bootloader.bin 0x8000 partition-table.bin 0x19000 ota-data.bin \
  0x40000 firmware.bin 0x440000 assets.bin
