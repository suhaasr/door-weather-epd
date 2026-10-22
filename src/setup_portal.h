#pragma once
// Wi-Fi + ZIP code setup: a WPA2 hotspot with a captive portal (WiFiManager).
#include "display.h"
#include "network.h"
#include "storage.h"

namespace door {

class SetupPortal {
 public:
  SetupPortal(SettingsStore& store, EpaperDisplay& display, Network& network)
      : store_(store), display_(display), network_(network) {}
  // Returns true once Wi-Fi works and the ZIP code resolves to a location.
  bool run(Settings& s, int batteryPct);
  // True if BOOT is held within kSetupButtonHoldMs of waking.
  static bool bootButtonHeld();

 private:
  SettingsStore& store_;
  EpaperDisplay& display_;
  Network& network_;
};

}  // namespace door
