#include "drift.h"

namespace door {

namespace {
constexpr float kMin = 0.85f;
constexpr float kMax = 1.15f;
constexpr double kMinSampleSeconds = 1200;

float clampCorrection(float c) { return c < kMin ? kMin : (c > kMax ? kMax : c); }
}  // namespace

float updateSleepCorrection(float current, double programmedSeconds,
                            double actualSeconds) {
  if (!(current >= kMin && current <= kMax)) current = 1.0f;
  if (programmedSeconds < kMinSampleSeconds || actualSeconds <= 0) return current;
  double observed = programmedSeconds / actualSeconds;  // timer ticks per real second
  if (observed < kMin || observed > kMax) return current;  // implausible sample
  return clampCorrection(static_cast<float>(0.5 * current + 0.5 * observed));
}

uint64_t correctedSleepMicros(double desiredSeconds, float correction) {
  if (desiredSeconds < 1) desiredSeconds = 1;
  return static_cast<uint64_t>(desiredSeconds * clampCorrection(correction) * 1e6);
}

}  // namespace door
