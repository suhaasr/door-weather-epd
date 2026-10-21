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

const char* tierAdvice(Tier t) {
  switch (t) {
    case Tier::Freezing: return "Heavy coat, hat, scarf & gloves";
    case Tier::Cold: return "Warm coat, scarf & boots";
    case Tier::Chilly: return "Light jacket & long pants";
    case Tier::Cool: return "Sweater & long pants";
    case Tier::Mild: return "T-shirt & long pants";
    case Tier::Warm: return "T-shirt & shorts or a skirt";
    case Tier::Hot: return "Light clothes & sunglasses";
  }
  return "";
}

Outfit outfitFor(Tier t, Gender g) {
  const bool man = g == Gender::Man;
  switch (t) {
    case Tier::Freezing:
      return {man ? Top::Coat : Top::LongCoat, Bottom::Pants, Shoes::Boots, true, true, true, false};
    case Tier::Cold:
      return {man ? Top::Coat : Top::LongCoat, Bottom::Pants, Shoes::Boots, true, false, false, false};
    case Tier::Chilly:
      return {Top::Jacket, Bottom::Pants, Shoes::Sneakers, false, false, false, false};
    case Tier::Cool:
      return {Top::Sweater, Bottom::Pants, Shoes::Sneakers, false, false, false, false};
    case Tier::Mild:
      return {man ? Top::TShirt : Top::LongSleeve, Bottom::Pants, Shoes::Sneakers, false, false, false, false};
    case Tier::Warm:
      return {Top::TShirt, man ? Bottom::Shorts : Bottom::Skirt, Shoes::Sneakers, false, false, false, false};
    case Tier::Hot:
      if (man) return {Top::Tank, Bottom::Shorts, Shoes::Sandals, false, false, false, true};
      return {Top::Sundress, Bottom::None, Shoes::Sandals, false, false, false, true};
  }
  return {Top::TShirt, Bottom::Pants, Shoes::Sneakers, false, false, false, false};
}

}  // namespace door
