#include "text.h"

#include <stdio.h>
#include <string.h>

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "fonts/DejaVuSansBold44pt7b.h"

namespace door {

const GFXfont* const kSmall = &FreeSans9pt7b;
const GFXfont* const kSmallBold = &FreeSansBold9pt7b;
const GFXfont* const kBody = &FreeSans12pt7b;
const GFXfont* const kBodyBold = &FreeSansBold12pt7b;
const GFXfont* const kTitle = &FreeSansBold18pt7b;
const GFXfont* const kHeadline = &FreeSansBold24pt7b;
const GFXfont* const kHuge = &DejaVuSans_Bold44pt7b;

int textWidth(const GFXfont* f, const char* s) {
  int adv = 0;
  for (const char* p = s; *p; ++p) {
    uint8_t c = static_cast<uint8_t>(*p);
    if (c >= f->first && c <= f->last) adv += f->glyph[c - f->first].xAdvance;
  }
  return adv;
}

void text(Adafruit_GFX& g, const GFXfont* f, int x, int baseline, const char* s,
          uint16_t color) {
  g.setFont(f);
  g.setTextSize(1);
  g.setTextColor(color);
  g.setTextWrap(false);
  g.setCursor(x, baseline);
  g.print(s);
}

void textCentered(Adafruit_GFX& g, const GFXfont* f, int cx, int baseline,
                  const char* s, uint16_t color) {
  text(g, f, cx - textWidth(f, s) / 2, baseline, s, color);
}

void textRight(Adafruit_GFX& g, const GFXfont* f, int right, int baseline,
               const char* s, uint16_t color) {
  text(g, f, right - textWidth(f, s), baseline, s, color);
}

int textWrapped(Adafruit_GFX& g, const GFXfont* f, int x, int baseline,
                int maxWidth, int lineHeight, const char* s, uint16_t color,
                bool centered) {
  char line[128] = "";
  char candidate[128];
  int lines = 0;
  auto flush = [&]() {
    if (!*line) return;
    if (centered) {
      textCentered(g, f, x, baseline + lines * lineHeight, line, color);
    } else {
      text(g, f, x, baseline + lines * lineHeight, line, color);
    }
    ++lines;
    line[0] = 0;
  };
  const char* p = s;
  while (*p) {
    if (*p == ' ') {
      ++p;
      continue;
    }
    if (*p == '\n') {
      flush();
      ++p;
      continue;
    }
    const char* end = p;
    while (*end && *end != ' ' && *end != '\n') ++end;
    snprintf(candidate, sizeof candidate, "%s%s%.*s", line, *line ? " " : "",
             static_cast<int>(end - p), p);
    if (*line && textWidth(f, candidate) > maxWidth) {
      flush();
      snprintf(line, sizeof line, "%.*s", static_cast<int>(end - p), p);
    } else {
      snprintf(line, sizeof line, "%s", candidate);
    }
    p = end;
  }
  flush();
  return lines;
}

void formatHour(int hour, char* buf, int len) {
  if (hour == 24) {
    snprintf(buf, len, "midnight");
    return;
  }
  int h = hour % 12;
  if (h == 0) h = 12;
  snprintf(buf, len, "%d %s", h, (hour % 24) < 12 ? "AM" : "PM");
}

}  // namespace door
