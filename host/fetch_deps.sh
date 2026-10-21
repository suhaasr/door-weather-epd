#!/usr/bin/env bash
# Fetches the pinned host-side copies of the Arduino libraries the core uses.
# Versions match platformio.ini.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p .deps
clone() {  # url ref dir
  [ -d ".deps/$3" ] || git -c advice.detachedHead=false clone -q --depth 1 -b "$2" "$1" ".deps/$3"
}
clone https://github.com/adafruit/Adafruit-GFX-Library 1.12.6 gfx
clone https://github.com/bblanchon/ArduinoJson v7.4.3 ArduinoJson
