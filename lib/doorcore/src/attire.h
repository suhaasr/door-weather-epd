#pragma once
#include <stdint.h>

namespace door {

enum class Tier : uint8_t { Freezing, Cold, Chilly, Cool, Mild, Warm, Hot };

// Tiers are chosen on the "feels like" (apparent) temperature in Fahrenheit.
Tier tierForFeelsLikeF(float feelsLikeF);
const char* tierName(Tier tier);

inline float celsiusToFahrenheit(float c) { return c * 9.0f / 5.0f + 32.0f; }

}  // namespace door
