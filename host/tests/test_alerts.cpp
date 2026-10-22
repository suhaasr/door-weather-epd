#include "test.h"

#include <math.h>

#include <map>

#include "extras.h"

using namespace door;
using namespace test;

TEST(alerts) {
  char url[400], buf[32];
  Forecast f = {};
  std::string json;
  buildNwsAlertsUrl(url, sizeof url, 37.7631f, -122.4129f);
  CHECK(strcmp(url, "https://api.weather.gov/alerts/active?point=37.7631,-122.4129") == 0);
  json = readFile("fixtures/nws_alerts_ak.json");
  CHECK(parseNwsAlerts(json.data(), json.size(), f));
  CHECK(f.alertsValid && f.alertCount == 4);  // two High Surf Advisories merged
  CHECK(strcmp(f.alerts[0].event, "Blizzard Warning") == 0);  // Extreme; the cancelled one is skipped
  CHECK(f.alerts[0].severity == 4 && f.alerts[1].severity == 3 && f.alerts[2].severity == 3);
  CHECK(strcmp(f.alerts[1].event, "High Wind Watch") == 0);
  CHECK(strcmp(f.alerts[0].ends, "2026-10-24T01:00") == 0);
  CHECK(strcmp(f.alerts[3].event, "High Surf Advisory") == 0);
  CHECK(strcmp(f.alerts[3].starts, "2026-10-23T04:00") == 0);
  CHECK(strcmp(f.alerts[3].ends, "2026-10-25T22:00") == 0);
  int idx[kMaxAlerts];
  CHECK(alertsFor(f, "2026-10-23", idx) == 4 && idx[0] == 0 && idx[3] == 3);
  CHECK(alertsFor(f, "2026-10-22", idx) == 0);
  CHECK(alertsFor(f, "2026-10-25", idx) == 2 && idx[0] == 1 && idx[1] == 3);
  formatAlertUntil(f.alerts[0], "2026-10-23", buf, sizeof buf);
  CHECK(strcmp(buf, "until Sat 1 AM") == 0);
  formatAlertUntil(f.alerts[0], "2026-10-24", buf, sizeof buf);
  CHECK(strcmp(buf, "until 1 AM") == 0);
  WeatherAlert a = {};
  strcpy(a.ends, "2026-10-21T16:30");
  formatAlertUntil(a, "2026-10-21", buf, sizeof buf);
  CHECK(strcmp(buf, "until 4:30 PM") == 0);
  const char* none = "{\"features\":[]}";
  CHECK(parseNwsAlerts(none, strlen(none), f) && f.alertCount == 0);
  CHECK(!parseNwsAlerts("oops", 4, f));
  CHECK(weekdayOfIsoDate("2026-10-21") == 3);
  CHECK(weekdayOfIsoDate("2028-02-29") == 2);
  CHECK(weekdayOfIsoDate("2026-02-30") == -1);

}
