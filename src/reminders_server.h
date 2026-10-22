#pragma once
// Serves the reminders page (ReminderPortal) on the home network until the
// user taps Done or it's idle for kRemindersIdleMinutes.
#include "display.h"
#include "network.h"
#include "storage.h"

namespace door {

class RemindersServer {
 public:
  enum class Result { Done, WifiSetup, NoWifi };
  RemindersServer(SettingsStore& store, EpaperDisplay& display, Network& network)
      : store_(store), display_(display), network_(network) {}
  Result run(const Settings& s);

 private:
  SettingsStore& store_;
  EpaperDisplay& display_;
  Network& network_;
};

}  // namespace door
