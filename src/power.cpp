#include "power.h"

#include <Arduino.h>
#include <esp_sleep.h>
#include <string.h>

#include "battery.h"
#include "config.h"
#include "drift.h"
#include "network.h"

namespace door {

static RTC_DATA_ATTR RtcState rtcState;

RtcState& Power::rtc() {
  if (rtcState.magic != kRtcMagic) {
    memset(&rtcState, 0, sizeof rtcState);
    rtcState.magic = kRtcMagic;
    rtcState.batteryPercent = -1;
    rtcState.sleepCorrection = 1.0f;
  }
  return rtcState;
}

bool Power::wokeFromTimer() { return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER; }

int Power::readBatteryPercent() {
  pinMode(kPinBatteryAdc, INPUT);
  uint32_t sum = 0;
  constexpr int kSamples = 16;
  for (int i = 0; i < kSamples; ++i) sum += analogReadMilliVolts(kPinBatteryAdc);
  int mv = static_cast<int>(sum / kSamples * kBatteryDividerRatio) + kBatteryMillivoltOffset;
  return batteryPercentFromMillivolts(mv);
}

void Power::sleepFor(double seconds) {
  Network().off();
  RtcState& r = rtc();
  esp_sleep_enable_timer_wakeup(correctedSleepMicros(seconds, r.sleepCorrection));
  r.programmedSleepS = seconds * r.sleepCorrection;
  esp_deep_sleep_start();
}

void Power::sleepUntilReset() {
  Network().off();
  rtc().sleepStartedAt = 0;
  esp_deep_sleep_start();  // no wake source: only RESET/power wakes it
}

}  // namespace door
