#include "test.h"

#include <math.h>

#include <map>

#include "attire.h"

using namespace door;
using namespace test;

TEST(tiers) {
  CHECK(tierForFeelsLikeF(10) == Tier::Freezing);
  CHECK(tierForFeelsLikeF(31.9f) == Tier::Freezing);
  CHECK(tierForFeelsLikeF(32) == Tier::Cold);
  CHECK(tierForFeelsLikeF(50) == Tier::Chilly);
  CHECK(tierForFeelsLikeF(60) == Tier::Cool);
  CHECK(tierForFeelsLikeF(70) == Tier::Mild);
  CHECK(tierForFeelsLikeF(80) == Tier::Warm);
  CHECK(tierForFeelsLikeF(85) == Tier::Hot);
  CHECK(celsiusToFahrenheit(100) == 212);
}
