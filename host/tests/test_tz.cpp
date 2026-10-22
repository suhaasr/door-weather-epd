#include "test.h"

#include <math.h>

#include <map>

#include "tz.h"

using namespace door;
using namespace test;

TEST(tz) {
  char buf[48];
  posixTzForIana("America/New_York", -14400, buf, sizeof buf);
  CHECK(strcmp(buf, "EST5EDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Los_Angeles", -25200, buf, sizeof buf);
  CHECK(strcmp(buf, "PST8PDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Indiana/Knox", -18000, buf, sizeof buf);
  CHECK(strcmp(buf, "CST6CDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Indiana/Indianapolis", -14400, buf, sizeof buf);
  CHECK(strcmp(buf, "EST5EDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Phoenix", -25200, buf, sizeof buf);
  CHECK(strcmp(buf, "MST7") == 0);
  posixTzForIana("Europe/Somewhere", 3600, buf, sizeof buf);
  CHECK(strcmp(buf, "UTC-1:00") == 0);
  posixTzForIana(nullptr, -19800, buf, sizeof buf);
  CHECK(strcmp(buf, "UTC+5:30") == 0);
}
