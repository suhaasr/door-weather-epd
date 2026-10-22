#pragma once
// Everything kept in flash across RESET and power loss.
#include "battery.h"
#include "reminders.h"

namespace door {

struct Settings {
  char zip[6];
  float lat;
  float lon;
  char place[40];
  char tz[48];  // POSIX TZ rule
  bool hasLocation;
};

class SettingsStore {
 public:
  Settings load() const;
  void saveLocation(const Settings& s);
  void saveTz(const char* tz);
  void loadReminders(ReminderList& list) const;
  void saveReminders(const ReminderList& list);
  BatteryTracker loadBatteryTracker() const;
  void saveBatteryTracker(const BatteryTracker& t);

 private:
  static constexpr const char* kNamespace = "door";
};

}  // namespace door
