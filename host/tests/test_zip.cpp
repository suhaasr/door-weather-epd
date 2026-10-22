#include "test.h"

#include <math.h>

#include <map>

#include "forecast.h"

using namespace door;
using namespace test;

TEST(zip) {
  CHECK(isValidZip("94110"));
  CHECK(isValidZip("02134"));
  CHECK(!isValidZip("9411"));
  CHECK(!isValidZip("941101"));
  CHECK(!isValidZip("94a10"));
  CHECK(!isValidZip(nullptr));
  std::string json = readFile("fixtures/zippopotam_94110.json");
  float lat = 0, lon = 0;
  char place[40];
  CHECK(parseZipLookup(json.data(), json.size(), lat, lon, place, sizeof place));
  CHECK(lat > 37.75f && lat < 37.76f);
  CHECK(lon < -122.41f && lon > -122.42f);
  CHECK(strcmp(place, "San Francisco, CA") == 0);
  CHECK(!parseZipLookup("{}", 2, lat, lon, place, sizeof place));
  char url[64];
  buildZipLookupUrl(url, sizeof url, "94110");
  CHECK(strcmp(url, "https://api.zippopotam.us/us/94110") == 0);
}
