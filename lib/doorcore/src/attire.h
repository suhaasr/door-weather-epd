#pragma once
#include <stdint.h>

namespace door {

enum class Tier : uint8_t { Freezing, Cold, Chilly, Cool, Mild, Warm, Hot };

enum class Gender : uint8_t { Man, Woman };

enum class Top : uint8_t { Tank, TShirt, LongSleeve, Sweater, Jacket, Coat, LongCoat, Sundress };
enum class Bottom : uint8_t { None, Shorts, Skirt, Pants };
enum class Shoes : uint8_t { Sandals, Sneakers, Boots };

struct Outfit {
  Top top;
  Bottom bottom;
  Shoes shoes;
  bool scarf;
  bool beanie;
  bool gloves;
  bool sunglasses;
};

// Tiers are chosen on the "feels like" (apparent) temperature in Fahrenheit.
Tier tierForFeelsLikeF(float feelsLikeF);
const char* tierName(Tier tier);
// Short description of what to wear, e.g. "Light jacket & long pants".
const char* tierAdvice(Tier tier);
Outfit outfitFor(Tier tier, Gender gender);

inline float celsiusToFahrenheit(float c) { return c * 9.0f / 5.0f + 32.0f; }

}  // namespace door
