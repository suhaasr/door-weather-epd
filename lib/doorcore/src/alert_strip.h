#pragma once
#include "screen.h"

namespace door {

// Black strip listing every NWS alert in effect: the most severe in large type,
// the rest in small text, wrapping onto extra lines as needed.
class AlertStrip {
 public:
  explicit AlertStrip(const WeatherScreen& s) : s_(s), layout_(computeLayout(s)) {}
  int height() const { return layout_.height; }  // 0 when there are no alerts
  int lines() const { return layout_.lines; }
  void draw(Adafruit_GFX& g, int top) const;

 private:
  struct Layout {
    const GFXfont* mainFont;
    int line[kMaxAlerts];  // line (0-based) each alert is drawn on
    int lines;
    int height;
  };
  static constexpr int kAlertX0 = 78, kAlertRight = kScreenW - 30, kAlertSep = 30;
  static constexpr int kAlertLineH = 24;
  static Layout computeLayout(const WeatherScreen& s);
  static int extraWidth(const ScreenAlert& a);
  static void upperCopy(const char* in, char* out, size_t len);
  const WeatherScreen& s_;
  Layout layout_;
};

}  // namespace door
