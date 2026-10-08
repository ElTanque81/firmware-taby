#!/usr/bin/env bash
# Build + flash Taby for Waveshare ESP32-S3-Touch-AMOLED-1.64 V1.1 (LCD CS = GPIO46).
# Usage:  bash taby_v11.sh [PORT]      e.g.  bash taby_v11.sh /dev/cu.usbmodem101
set -euo pipefail
PORT="${1:-/dev/cu.usbmodem101}"
WORK="$HOME/esp"
mkdir -p "$WORK"

# 1) ESP-IDF v5.4.2 (Taby is pinned to exactly this version) - one time, ~10-20 min
if [ ! -d "$WORK/esp-idf" ]; then
  git clone -b v5.4.2 --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git "$WORK/esp-idf"
fi
cd "$WORK/esp-idf"
./install.sh esp32s3
# shellcheck disable=SC1091
. ./export.sh

# 2) Taby source
if [ ! -d "$WORK/firmware-taby" ]; then
  git clone --depth 1 https://github.com/TRIIIS-LABS/firmware-taby "$WORK/firmware-taby"
fi
cd "$WORK/firmware-taby"

# 3) V1.1 patch: the only difference to V1 is the LCD chip-select pin (GPIO9 -> GPIO46)
python3 - <<'EOF'
p = "firmware/main/board_amoled_1_64.c"
s = open(p).read()
old = "#define PIN_NUM_LCD_CS GPIO_NUM_9\n"
new = "#define PIN_NUM_LCD_CS GPIO_NUM_46  /* V1.1: CS moved from GPIO9 */\n"
if new in s:
    print("already patched")
else:
    assert s.count(old) == 1, "pattern not found - repo changed?"
    open(p, "w").write(s.replace(old, new))
    print("patched CS pin -> GPIO46")
EOF

# 4) Build (same flags as tools/build.py, without its source-digest check, which the patch would trip)
cd firmware
idf.py -B build-amoled-1.64 \
  -D SDKCONFIG=sdkconfig.amoled-1.64 \
  -D SDKCONFIG_DEFAULTS=sdkconfig.defaults \
  -D TABY_HARDWARE_TARGET=amoled-1.64 \
  build

# 5) Flash bootloader, partition table, otadata, app and assets in one go
idf.py -B build-amoled-1.64 -p "$PORT" -b 460800 flash
echo "Done. Press RST on the board."
