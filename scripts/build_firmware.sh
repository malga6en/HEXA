#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "$PROJECT_ROOT/firmware"

echo "Building HEXA firmware..."

pio run

echo "Exporting firmware.bin..."

mkdir -p "$PROJECT_ROOT/output"

cp \
    .pio/build/esp32-s2-saola-1/firmware.bin \
    "$PROJECT_ROOT/output/firmware.bin"

echo
echo "Build successful."
echo "Firmware:"
echo "$PROJECT_ROOT/output/firmware.bin"