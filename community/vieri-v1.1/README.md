# Waveshare ESP32-S3-Touch-AMOLED-1.64 V1.1 notes

Community notes, not an official Taby release. Taby firmware, artwork and
license belong to TRIIIS LABS (see the top-level `LICENSE`, Apache-2.0 plus the
Taby Artwork Terms).

## What is different on V1.1

Same SH8601/CO5300-style init sequence, X offset (`0x14`), QSPI data pins and
reset pin as V1. The only difference found is the **LCD chip select**:

| Board | LCD_CS |
|-------|--------|
| V1    | GPIO 9 |
| V1.1  | GPIO 46 |

V1.1 pins (from Waveshare's `ESP32-S3-Touch-AMOLED-1.64-v2` demo): CS 46,
CLK 10, D0 11, D1 12, D2 13, D3 14, RST 21, touch I2C SDA 47 / SCL 48.

With the stock V1 Taby image the screen stays black on V1.1.

## Contents

- `taby_v11.sh` - installs ESP-IDF v5.4.2, builds Taby with `PIN_NUM_LCD_CS`
  set to GPIO 46 and flashes it (`bash taby_v11.sh /dev/cu.usbmodemXXX`).
- `Vieri/` - small Arduino sketch (own animated face with touch) using the
  Waveshare SH8601 driver. Display commands use QSPI opcode `0x02`, pixel data
  opcode `0x32`, x offset `0x14`, even-aligned draw windows.

## Status

Tested on one V1.1 board: the patched Taby build runs and the display works
(touch was not tested separately). The Vieri sketch shows its face on the same
board. The build uses `idf.py` directly, not `tools/build.py`, because
that script's source-digest check is not meant for modified sources.
