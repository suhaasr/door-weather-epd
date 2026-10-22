#include "attire.h"

namespace door {

Tier tierForFeelsLikeF(float f) {
  if (f < 32) return Tier::Freezing;
  if (f < 45) return Tier::Cold;
  if (f < 55) return Tier::Chilly;
  if (f < 65) return Tier::Cool;
  if (f < 75) return Tier::Mild;
  if (f < 85) return Tier::Warm;
  return Tier::Hot;
}

const char* tierName(Tier t) {
  switch (t) {
    case Tier::Freezing: return "Freezing";
    case Tier::Cold: return "Cold";
    case Tier::Chilly: return "Chilly";
    case Tier::Cool: return "Cool";
    case Tier::Mild: return "Mild";
    case Tier::Warm: return "Warm";
    case Tier::Hot: return "Hot";
  }
  return "";
}

}  // namespace door
