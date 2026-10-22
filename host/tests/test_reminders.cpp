#include "test.h"

#include <math.h>

#include <map>

#include "reminders.h"

using namespace door;
using namespace test;

TEST(reminders) {
  char out[kReminderTextMax + 1];
  sanitizeReminderText("  Pack\t lunch\n ", out, sizeof out);
  CHECK(strcmp(out, "Pack lunch") == 0);
  sanitizeReminderText("Kid\xE2\x80\x99s \xE2\x80\x9Cshow\xE2\x80\x9D \xE2\x80\x94 gym \xF0\x9F\x8D\x8E!", out, sizeof out);
  CHECK(strcmp(out, "Kid's \"show\" - gym !") == 0);
  sanitizeReminderText("0123456789012345678901234567890123456789XYZ", out, sizeof out);
  CHECK(strlen(out) == static_cast<size_t>(kReminderTextMax));
  char small[6];
  sanitizeReminderText("abcd efgh", small, sizeof small);
  CHECK(strcmp(small, "abcd") == 0);  // no dangling space

  CHECK(isValidIsoDate("2026-02-28"));
  CHECK(isValidIsoDate("2028-02-29"));
  CHECK(!isValidIsoDate("2026-02-29"));
  CHECK(!isValidIsoDate("2026-13-01"));
  CHECK(!isValidIsoDate("2026-1-01"));
  CHECK(!isValidIsoDate(""));

  ReminderList list;
  parseReminders("20||Pack lunch\n0|2026-10-30|Dentist\nbad line\n0||no schedule\n"
                 "3|2026-10-30|both\n0|2026-02-30|bad date\n127||Water plants",
                 list);
  CHECK(list.count == 3);
  CHECK(list.items[0].weekdays == 0x14);  // Tue + Thu
  CHECK(strcmp(list.items[1].date, "2026-10-30") == 0);
  ReminderList again;
  parseReminders(serializeReminders(list).c_str(), again);
  CHECK(again.count == 3 && strcmp(again.items[2].text, "Water plants") == 0);

  CHECK(describeSchedule(list.items[0]) == "Tue, Thu");
  CHECK(describeSchedule(list.items[1]) == "Fri 2026-10-30");
  CHECK(describeSchedule(list.items[2]) == "Every day");
  Reminder wk = {0x3E, "", "x"};
  CHECK(describeSchedule(wk) == "Weekdays");

  int idx[kMaxReminders];
  CHECK(remindersFor(list, "2026-10-20", 2, idx) == 2);  // Tuesday
  CHECK(idx[0] == 0 && idx[1] == 2);
  CHECK(remindersFor(list, "2026-10-21", 3, idx) == 1);  // Wednesday
  CHECK(remindersFor(list, "2026-10-30", 5, idx) == 2);  // the dentist Friday

  CHECK(!pruneExpiredReminders(list, "2026-10-30"));
  CHECK(pruneExpiredReminders(list, "2026-10-31"));
  CHECK(list.count == 2);
  CHECK(!pruneExpiredReminders(list, ""));

  for (int i = list.count; i < kMaxReminders; ++i) CHECK(addReminder(list, wk));
  CHECK(!addReminder(list, wk));
  CHECK(removeReminder(list, 0) && list.count == kMaxReminders - 1);
  CHECK(!removeReminder(list, 99));
}
