#pragma once
// Battery reading, state kept in RTC memory across deep sleep, and sleeping.
#include <time.h>

#include "forecast.h"

namespace door {

// Survives deep sleep (lost on power loss or RESET).
struct RtcState {
  uint32_t magic;
  int batteryPercent;
  float sleepCorrection;
  time_t sleepStartedAt;    // NTP-accurate time we went to sleep, 0 if unknown
  double programmedSleepS;  // value handed to the sleep timer
  bool criticalShown;
  time_t forecastFetchedAt;
  Forecast forecast;
};

class Power {
 public:
  // Clears the RTC state after power loss (or a layout change). Returns it.
  static RtcState& rtc();
  static bool wokeFromTimer();
  static int readBatteryPercent();
  [[noreturn]] static void sleepFor(double seconds);
  [[noreturn]] static void sleepUntilReset();

 private:
  static constexpr uint32_t kRtcMagic = 0xD00D0004;
};

}  // namespace door
