#include "test.h"

#include <math.h>

#include <map>

#include "drift.h"

using namespace door;
using namespace test;

TEST(drift) {
  // RTC runs 5% fast: we programmed 10000 s but only 9524 s really passed.
  float c = updateSleepCorrection(1.0f, 10000, 10000 / 1.05);
  CHECK(c > 1.02f && c < 1.03f);
  for (int i = 0; i < 10; ++i) {
    double desired = 10000;
    double programmed = desired * c;
    c = updateSleepCorrection(c, programmed, programmed / 1.05);
  }
  CHECK(c > 1.045f && c < 1.055f);
  CHECK(updateSleepCorrection(1.0f, 600, 500) == 1.0f);       // too short to learn from
  CHECK(updateSleepCorrection(1.0f, 10000, 2000) == 1.0f);    // implausible
  CHECK(updateSleepCorrection(NAN, 10000, 10000) == 1.0f);    // garbage state
  CHECK(correctedSleepMicros(3600, 1.0f) == 3600000000ULL);
  CHECK(llabs((long long)correctedSleepMicros(100, 5.0f) - 115000000LL) < 10);  // clamped
}
