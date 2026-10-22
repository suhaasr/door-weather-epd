#include "storage.h"

#include <Preferences.h>
#include <math.h>

#include "forecast.h"

namespace door {

Settings SettingsStore::load() const {
  Settings s{};
  Preferences p;
  p.begin(kNamespace, true);
  p.getString("zip", s.zip, sizeof s.zip);
  p.getString("place", s.place, sizeof s.place);
  p.getString("tz", s.tz, sizeof s.tz);
  s.lat = p.getFloat("lat", NAN);
  s.lon = p.getFloat("lon", NAN);
  p.end();
  s.hasLocation = isValidZip(s.zip) && !isnan(s.lat) && !isnan(s.lon);
  return s;
}

void SettingsStore::saveLocation(const Settings& s) {
  Preferences p;
  p.begin(kNamespace, false);
  p.putString("zip", s.zip);
  p.putString("place", s.place);
  p.putFloat("lat", s.lat);
  p.putFloat("lon", s.lon);
  p.end();
}

void SettingsStore::saveTz(const char* tz) {
  Preferences p;
  p.begin(kNamespace, false);
  p.putString("tz", tz);
  p.end();
}

void SettingsStore::loadReminders(ReminderList& list) const {
  Preferences p;
  p.begin(kNamespace, true);
  String data = p.getString("rem", "");
  p.end();
  parseReminders(data.c_str(), list);
}

void SettingsStore::saveReminders(const ReminderList& list) {
  Preferences p;
  p.begin(kNamespace, false);
  p.putString("rem", serializeReminders(list).c_str());
  p.end();
}

BatteryTracker SettingsStore::loadBatteryTracker() const {
  Preferences p;
  p.begin(kNamespace, true);
  BatteryTracker t(p.getLong64("batT", 0), p.getInt("batP", -1));
  p.end();
  return t;
}

void SettingsStore::saveBatteryTracker(const BatteryTracker& t) {
  Preferences p;
  p.begin(kNamespace, false);
  p.putLong64("batT", t.anchorTime());
  p.putInt("batP", t.anchorPercent());
  p.end();
}

}  // namespace door
