#pragma once
// The 7.5" e-paper panel. Each show() powers it up, draws a full refresh and
// puts it back into hibernation.
#include <Adafruit_GFX.h>

#include <functional>

namespace door {

class EpaperDisplay {
 public:
  void show(const std::function<void(Adafruit_GFX&)>& draw);

 private:
  void begin();
};

}  // namespace door
