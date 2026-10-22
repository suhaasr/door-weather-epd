#pragma once
// Fonts and text helpers shared by every screen component.
#include <Adafruit_GFX.h>

namespace door {

extern const GFXfont* const kSmall;      // 9 pt
extern const GFXfont* const kSmallBold;
extern const GFXfont* const kBody;       // 12 pt
extern const GFXfont* const kBodyBold;
extern const GFXfont* const kTitle;      // 18 pt bold
extern const GFXfont* const kHeadline;   // 24 pt bold
extern const GFXfont* const kHuge;       // 44 pt bold, for the big numbers

int textWidth(const GFXfont* f, const char* s);
void text(Adafruit_GFX& g, const GFXfont* f, int x, int baseline, const char* s, uint16_t color);
void textCentered(Adafruit_GFX& g, const GFXfont* f, int cx, int baseline, const char* s,
                  uint16_t color);
void textRight(Adafruit_GFX& g, const GFXfont* f, int right, int baseline, const char* s,
               uint16_t color);
// Greedy word wrap at spaces; '\n' forces a break. Returns lines drawn. When
// centered, x is the centre line.
int textWrapped(Adafruit_GFX& g, const GFXfont* f, int x, int baseline, int maxWidth,
                int lineHeight, const char* s, uint16_t color, bool centered = false);

// "2 PM" / "12 AM" / "midnight" for hour 24.
void formatHour(int hour, char* buf, int len);

}  // namespace door
