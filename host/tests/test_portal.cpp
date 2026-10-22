#include "test.h"

#include <math.h>

#include <map>

#include "portal.h"

using namespace door;
using namespace test;

static uint32_t portalSeq;
static uint32_t portalRandom() { return portalSeq++ * 2654435761u; }

TEST(portal) {
  ReminderList list = {};
  portalSeq = 1;
  ReminderPortal portal(&list, portalRandom);
  CHECK(strlen(portal.pin()) == 6 && strlen(portal.token()) == 16);
  portal.setToday("2026-10-21", 3);
  std::map<std::string, std::string> a;
  auto arg = [&](const char* n) { auto it = a.find(n); return it == a.end() ? std::string() : it->second; };
  auto req = [&](const char* m, const char* p) { return portal.handle(m, p, arg); };

  PortalResponse r = req("GET", "/");
  CHECK(r.status == 200 && r.body.find("name='pin'") != std::string::npos);
  CHECK(r.body.find(portal.token()) == std::string::npos);  // token only after the PIN

  a = {{"text", "Pack lunch"}, {"d2", "1"}};
  r = req("POST", "/add");
  CHECK(r.status == 403 && list.count == 0);  // no token
  a["t"] = "0000000000000000";
  r = req("POST", "/add");
  CHECK(r.status == 403 && list.count == 0);  // wrong token

  a = {{"pin", "000000"}};
  if (strcmp(portal.pin(), "000000") == 0) a["pin"] = "111111";
  r = req("POST", "/login");
  CHECK(r.status == 403 && portal.failedPins() == 1);
  CHECK(r.body.find("4 tries left") != std::string::npos);
  a = {{"pin", portal.pin()}};
  r = req("POST", "/login");
  CHECK(r.status == 303 && r.location == std::string("/?t=") + portal.token());

  const std::string t = portal.token();
  a = {{"t", t}, {"text", "Pack lunch"}, {"d2", "1"}, {"d4", "1"}};
  r = req("POST", "/add");
  CHECK(r.status == 303 && r.location.find("m=added") != std::string::npos);
  CHECK(list.count == 1 && list.items[0].weekdays == 0x14 && portal.takeChanged());
  CHECK(!portal.takeChanged());

  a = {{"t", t}, {"text", "<b>Dentist</b>"}, {"date", "2026-10-30"}};
  r = req("POST", "/add");
  CHECK(list.count == 2 && strcmp(list.items[1].date, "2026-10-30") == 0);
  a = {{"t", t}, {"text", "x"}, {"date", "2026-10-20"}};
  CHECK(req("POST", "/add").location.find("e_past") != std::string::npos);
  a = {{"t", t}, {"text", "x"}, {"date", "2026-10-22"}, {"d1", "1"}};
  CHECK(req("POST", "/add").location.find("e_both") != std::string::npos);
  a = {{"t", t}, {"text", "x"}};
  CHECK(req("POST", "/add").location.find("e_when") != std::string::npos);
  a = {{"t", t}, {"text", "   "}, {"d1", "1"}};
  CHECK(req("POST", "/add").location.find("e_text") != std::string::npos);
  a = {{"t", t}, {"text", "x"}, {"date", "2026-13-01"}};
  CHECK(req("POST", "/add").location.find("e_date") != std::string::npos);
  CHECK(list.count == 2);

  a = {{"t", t}, {"m", "added"}};
  r = req("GET", "/");
  CHECK(r.status == 200);
  CHECK(r.body.find("Reminder added.") != std::string::npos);
  CHECK(r.body.find("Tue, Thu") != std::string::npos);
  CHECK(r.body.find("&lt;b&gt;Dentist&lt;/b&gt;") != std::string::npos);
  CHECK(r.body.find("<b>Dentist</b>") == std::string::npos);
  CHECK(r.body.find("Today is Wednesday 2026-10-21") != std::string::npos);

  a = {{"t", t}, {"i", "0"}};
  r = req("POST", "/delete");
  CHECK(list.count == 1 && strcmp(list.items[0].text, "<b>Dentist</b>") == 0);
  a = {{"t", t}, {"i", "7"}};
  req("POST", "/delete");
  CHECK(list.count == 1);

  a = {{"t", t}};
  r = req("POST", "/done");
  CHECK(r.status == 200 && portal.done() && !portal.wifiSetupRequested());
  CHECK(req("GET", "/").status == 410);

  // Wi-Fi setup request, and PIN lockout.
  portal = ReminderPortal(&list, portalRandom);
  a = {{"t", portal.token()}};
  req("POST", "/wifi");
  CHECK(portal.done() && portal.wifiSetupRequested());
  portal = ReminderPortal(&list, portalRandom);
  a = {{"pin", "abc"}};
  for (int i = 0; i < kPortalMaxPinAttempts; ++i) req("POST", "/login");
  CHECK(portal.done());
  a = {{"pin", portal.pin()}};
  CHECK(req("POST", "/login").status == 410);
  CHECK(htmlEscape("a&'\"<>") == "a&amp;&#39;&quot;&lt;&gt;");
}

