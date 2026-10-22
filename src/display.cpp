#include "display.h"

#include <GxEPD2_BW.h>
#include <SPI.h>

#include "config.h"

namespace door {

static GxEPD2_BW<DOOR_EPD_DRIVER, DOOR_EPD_DRIVER::HEIGHT> panel(
    DOOR_EPD_DRIVER(kPinEpdCs, kPinEpdDc, kPinEpdRst, kPinEpdBusy));

void EpaperDisplay::begin() {
  if (kPinEpdPower >= 0) {
    pinMode(kPinEpdPower, OUTPUT);
    digitalWrite(kPinEpdPower, HIGH);
    delay(10);
  }
  SPI.begin(kPinEpdSck, kPinEpdMiso, kPinEpdMosi, kPinEpdCs);
  // 2 ms reset pulse is required by Waveshare/DESPI boards with the
  // "clever" reset circuit.
  panel.init(0, true, 2, false);
  panel.setRotation(0);
  panel.setFullWindow();
}

void EpaperDisplay::show(const std::function<void(Adafruit_GFX&)>& draw) {
  begin();
  panel.firstPage();
  do {
    draw(panel);
  } while (panel.nextPage());
  panel.hibernate();
  if (kPinEpdPower >= 0) digitalWrite(kPinEpdPower, LOW);
}

}  // namespace door
