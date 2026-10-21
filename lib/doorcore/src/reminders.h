#pragma once
#include <stddef.h>
#include <stdint.h>

#include <string>

namespace door {

constexpr int kMaxReminders = 8;
constexpr int kReminderTextMax = 40;

// Either repeats weekly (weekdays != 0, date empty) or happens once (date set).
struct Reminder {
  uint8_t weekdays;  // bit 0 = Sunday ... bit 6 = Saturday
  char date[11];     // "YYYY-MM-DD" or ""
  char text[kReminderTextMax + 1];
};

struct ReminderList {
  Reminder items[kMaxReminders];
  int count;
};

// Display-safe copy of user text: printable ASCII only, smart quotes/dashes
// mapped to ASCII, whitespace collapsed and trimmed, truncated to `outLen - 1`.
void sanitizeReminderText(const char* in, char* out, size_t outLen);

bool isValidIsoDate(const char* date);

// One line per reminder: "<weekday mask>|<date>|<text>".
std::string serializeReminders(const ReminderList& list);
void parseReminders(const char* data, ReminderList& out);

bool addReminder(ReminderList& list, const Reminder& r);
bool removeReminder(ReminderList& list, int index);

bool reminderApplies(const Reminder& r, const char* isoDate, int weekday);

// Indices of reminders for the given day, in list order. Returns the count.
int remindersFor(const ReminderList& list, const char* isoDate, int weekday,
                 int* indices);

// Drops one-off reminders dated before `today`. Returns true if any were removed.
bool pruneExpiredReminders(ReminderList& list, const char* today);

// "Every day", "Weekdays", "Weekends", "Tue, Thu" or "Wed 2026-11-04".
std::string describeSchedule(const Reminder& r);

}  // namespace door
