#pragma once
// Minimal test harness: TEST(name) { CHECK(...); } registers a suite; run all
// with `build/tests`, or only suites whose name contains a filter with
// `build/tests <filter>` (`make test T=<filter>`).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <Adafruit_GFX.h>

namespace test {

extern int checks;
extern int failures;

struct Case {
  const char* name;
  void (*fn)();
};
std::vector<Case>& registry();
struct Register {
  Register(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline std::string readFile(const char* path) {
  std::ifstream in(path);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

inline void useTz(const char* tz) {
  setenv("TZ", tz, 1);
  tzset();
}

inline time_t localTime(int y, int mo, int d, int h, int mi, int s = 0) {
  struct tm t = {};
  t.tm_year = y - 1900;
  t.tm_mon = mo - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  t.tm_isdst = -1;
  return mktime(&t);
}

inline struct tm local(time_t t) {
  struct tm r;
  localtime_r(&t, &r);
  return r;
}

inline int blackPixels(GFXcanvas1& c, int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x) n += c.getPixel(x, y) ? 0 : 1;
  return n;
}

}  // namespace test

#define CHECK(cond)                                                              \
  do {                                                                           \
    ++test::checks;                                                              \
    if (!(cond)) {                                                               \
      ++test::failures;                                                          \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);   \
    }                                                                            \
  } while (0)

#define TEST(name)                                              \
  static void test_##name();                                    \
  static test::Register register_##name(#name, test_##name);    \
  static void test_##name()
