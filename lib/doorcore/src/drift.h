#pragma once
#include <stdint.h>

namespace door {

// The ESP32's internal RTC oscillator can drift by several percent, which
// adds up over the ~17 hour overnight sleep. After each NTP sync we compare
// how long we actually slept with what we programmed and learn a correction
// factor that is applied to the next sleep.
//
// programmedSeconds: duration handed to the sleep timer.
// actualSeconds:     real (NTP) time that elapsed during that sleep.
float updateSleepCorrection(float current, double programmedSeconds,
                            double actualSeconds);

uint64_t correctedSleepMicros(double desiredSeconds, float correction);

}  // namespace door
