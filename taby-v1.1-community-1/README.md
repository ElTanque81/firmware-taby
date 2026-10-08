# Taby community build for Waveshare ESP32-S3-Touch-AMOLED-1.64 V1.1

Modified Taby firmware (LCD CS on GPIO46 instead of GPIO9), based on
TRIIIS-LABS/firmware-taby v1.2.0. Community build, **not an official Taby
release**. See `LICENSE` (Apache-2.0 + Taby Artwork Terms, TRIIIS LABS).

Tested on one V1.1 board: display and touch work.
Do not flash on a V1 board.

    bash flash.sh /dev/cu.usbmodemXXX      # needs esptool, run inside this folder

Checksums: `SHA256SUMS`. Notes and source patch: branch `vieri-v1.1`.
