#include "tz.h"

#include <stdio.h>
#include <string.h>

namespace door {

namespace {
struct Zone {
  const char* prefix;
  const char* posix;
};

const char* const kEastern = "EST5EDT,M3.2.0,M11.1.0";
const char* const kCentral = "CST6CDT,M3.2.0,M11.1.0";
const char* const kMountain = "MST7MDT,M3.2.0,M11.1.0";
const char* const kPacific = "PST8PDT,M3.2.0,M11.1.0";
const char* const kAlaska = "AKST9AKDT,M3.2.0,M11.1.0";

// Longest/most specific prefixes first.
const Zone kZones[] = {
    {"America/Indiana/Knox", kCentral},
    {"America/Indiana/Tell_City", kCentral},
    {"America/Indiana/", kEastern},
    {"America/Kentucky/", kEastern},
    {"America/North_Dakota/", kCentral},
    {"America/New_York", kEastern},
    {"America/Detroit", kEastern},
    {"America/Chicago", kCentral},
    {"America/Menominee", kCentral},
    {"America/Denver", kMountain},
    {"America/Boise", kMountain},
    {"America/Phoenix", "MST7"},
    {"America/Los_Angeles", kPacific},
    {"America/Anchorage", kAlaska},
    {"America/Juneau", kAlaska},
    {"America/Sitka", kAlaska},
    {"America/Metlakatla", kAlaska},
    {"America/Yakutat", kAlaska},
    {"America/Nome", kAlaska},
    {"America/Adak", "HST10HDT,M3.2.0,M11.1.0"},
    {"Pacific/Honolulu", "HST10"},
    {"America/Puerto_Rico", "AST4"},
    {"America/St_Thomas", "AST4"},
    {"Pacific/Guam", "ChST-10"},
    {"Pacific/Saipan", "ChST-10"},
    {"Pacific/Pago_Pago", "SST11"},
};
}  // namespace

void posixTzForIana(const char* iana, long offset, char* buf, size_t len) {
  if (iana) {
    for (const Zone& z : kZones) {
      if (strncmp(iana, z.prefix, strlen(z.prefix)) == 0) {
        snprintf(buf, len, "%s", z.posix);
        return;
      }
    }
  }
  // POSIX offsets have the opposite sign of UTC offsets.
  long west = -offset;
  char sign = west < 0 ? '-' : '+';
  if (west < 0) west = -west;
  snprintf(buf, len, "UTC%c%ld:%02ld", sign, west / 3600, (west % 3600) / 60);
}

}  // namespace door
