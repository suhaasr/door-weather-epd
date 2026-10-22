#include "alert_strip.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "battery.h"
#include "extras.h"
#include "icons.h"
#include "text.h"

namespace door {

int AlertStrip::extraWidth(const ScreenAlert& a) {
  return textWidth(kSmallBold, a.event) + 6 + textWidth(kSmall, a.until);
}

void AlertStrip::upperCopy(const char* in, char* out, size_t len) {
  size_t i = 0;
  for (; in[i] && i + 1 < len; ++i) out[i] = static_cast<char>(toupper(in[i]));
  out[i] = 0;
}

AlertStrip::Layout AlertStrip::computeLayout(const WeatherScreen& s) {
  Layout l = {kTitle, {}, 0, 0};
  if (s.alertCount <= 0) return l;
  char event[40];
  upperCopy(s.alerts[0].event, event, sizeof event);
  auto mainWidth = [&](const GFXfont* f) {
    return textWidth(f, event) + 14 + textWidth(kBody, s.alerts[0].until);
  };
  if (kAlertX0 + mainWidth(kTitle) > kAlertRight) l.mainFont = kBodyBold;
  int x = kAlertX0 + mainWidth(l.mainFont);
  int line = 0;
  for (int i = 1; i < s.alertCount; ++i) {
    const int w = extraWidth(s.alerts[i]);
    if (x + kAlertSep + w > kAlertRight && x > kAlertX0) {
      ++line;
      x = kAlertX0 - kAlertSep;
    }
    l.line[i] = line;
    x += kAlertSep + w;
  }
  l.lines = line + 1;
  l.height = 44 + line * kAlertLineH + (line ? 4 : 0);
  return l;
}

void AlertStrip::draw(Adafruit_GFX& g, int top) const {
  const WeatherScreen& s = s_;
  const Layout& l = layout_;
  g.fillRoundRect(12, top, kScreenW - 24, l.height, 10, kBlack);
  const int cy = top + 22;
  g.fillTriangle(44, cy - 16, 25, cy + 15, 63, cy + 15, kWhite);
  g.fillRect(42, cy - 7, 5, 13, kBlack);
  g.fillRect(42, cy + 9, 5, 4, kBlack);

  char event[40];
  upperCopy(s.alerts[0].event, event, sizeof event);
  text(g, l.mainFont, kAlertX0, cy + (l.mainFont == kTitle ? 12 : 9), event, kWhite);
  int x = kAlertX0 + textWidth(l.mainFont, event) + 14;
  text(g, kBody, x, cy + 9, s.alerts[0].until, kWhite);
  x += textWidth(kBody, s.alerts[0].until);

  int line = 0;
  for (int i = 1; i < s.alertCount; ++i) {
    if (l.line[i] != line) {
      line = l.line[i];
      x = kAlertX0 - kAlertSep;
    }
    const int baseline = line == 0 ? cy + 8 : top + 34 + line * kAlertLineH;
    const ScreenAlert& a = s.alerts[i];
    g.fillCircle(x + kAlertSep / 2, baseline - 5, 3, kWhite);
    x += kAlertSep;
    text(g, kSmallBold, x, baseline, a.event, kWhite);
    x += textWidth(kSmallBold, a.event) + 6;
    text(g, kSmall, x, baseline, a.until, kWhite);
    x += textWidth(kSmall, a.until);
  }
}

}  // namespace door
