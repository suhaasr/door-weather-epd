#pragma once
#include "icons.h"
#include "screen.h"

namespace door {

// One of the three equal bottom panels: label, big number, caption. A warning
// level > 0 adds a triangle (outlined, then solid); 3 also inverts the panel.
class StatPanel {
 public:
  StatPanel(const char* label, const char* value, char unit, const char* caption,
            int warningLevel = 0)
      : label_(label), value_(value), unit_(unit), caption_(caption), warningLevel_(warningLevel) {}
  void draw(Adafruit_GFX& g, int x0, int w, int top, int bottom) const;

  // 0 = Good, 1 = Moderate, 2 = Unhealthy for some, 3 = Unhealthy or worse.
  static int aqiWarningLevel(int aqi);

 private:
  static void drawBigNumber(Adafruit_GFX& g, int x, int baseline, const char* num, char unit,
                            bool small, uint16_t fg = kBlack, uint16_t bg = kWhite);
  static void drawWarningIcon(Adafruit_GFX& g, int cx, int cy, int size, bool solid,
                              uint16_t fg, uint16_t bg);
  const char* label_;
  const char* value_;
  char unit_;
  const char* caption_;
  int warningLevel_;
};

}  // namespace door
