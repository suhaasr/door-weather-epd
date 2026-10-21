#include "reminders.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace door {

namespace {
const char* const kShortDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int weekdayOf(const char* iso) {
  struct tm t = {};
  t.tm_year = atoi(iso) - 1900;
  t.tm_mon = atoi(iso + 5) - 1;
  t.tm_mday = atoi(iso + 8);
  t.tm_hour = 12;
  t.tm_isdst = -1;
  mktime(&t);
  return t.tm_wday;
}
}  // namespace

void sanitizeReminderText(const char* in, char* out, size_t outLen) {
  if (outLen == 0) return;
  size_t n = 0;
  bool pendingSpace = false;
  const unsigned char* p = reinterpret_cast<const unsigned char*>(in ? in : "");
  while (*p && n + 1 < outLen) {
    char c = 0;
    if (*p < 0x80) {
      c = static_cast<char>(*p++);
    } else if (p[0] == 0xE2 && p[1] == 0x80 && p[2]) {  // U+2010..U+203F punctuation
      unsigned char b = p[2];
      if (b >= 0x98 && b <= 0x9B) c = '\'';
      else if (b >= 0x9C && b <= 0x9F) c = '"';
      else if (b >= 0x90 && b <= 0x95) c = '-';
      else if (b == 0xA6) c = '.';
      p += 3;
    } else {
      ++p;  // skip the rest of this UTF-8 sequence
      while ((*p & 0xC0) == 0x80) ++p;
    }
    if (c == '\t' || c == '\n' || c == '\r' || c == ' ') {
      pendingSpace = n > 0;
      continue;
    }
    if (c < 0x20 || c > 0x7E) continue;
    if (pendingSpace) {
      if (n + 2 >= outLen) break;  // no room for both the space and the next char
      out[n++] = ' ';
    }
    pendingSpace = false;
    out[n++] = c;
  }
  out[n] = '\0';
}

bool isValidIsoDate(const char* d) {
  if (!d || strlen(d) != 10 || d[4] != '-' || d[7] != '-') return false;
  for (int i : {0, 1, 2, 3, 5, 6, 8, 9})
    if (d[i] < '0' || d[i] > '9') return false;
  int y = atoi(d), m = atoi(d + 5), day = atoi(d + 8);
  if (y < 2000 || y > 2099 || m < 1 || m > 12 || day < 1) return false;
  static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int maxDay = kDays[m - 1] + (m == 2 && y % 4 == 0 ? 1 : 0);
  return day <= maxDay;
}

std::string serializeReminders(const ReminderList& list) {
  std::string s;
  for (int i = 0; i < list.count; ++i) {
    const Reminder& r = list.items[i];
    char line[80];
    snprintf(line, sizeof line, "%u|%s|%s\n", r.weekdays & 0x7F, r.date, r.text);
    s += line;
  }
  return s;
}

void parseReminders(const char* data, ReminderList& out) {
  memset(&out, 0, sizeof out);
  const char* p = data ? data : "";
  while (*p && out.count < kMaxReminders) {
    const char* end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    std::string line(p, end);
    p = *end ? end + 1 : end;
    size_t a = line.find('|');
    size_t b = a == std::string::npos ? a : line.find('|', a + 1);
    if (b == std::string::npos) continue;
    Reminder r = {};
    r.weekdays = static_cast<uint8_t>(atoi(line.substr(0, a).c_str()) & 0x7F);
    std::string date = line.substr(a + 1, b - a - 1);
    if (!date.empty()) {
      if (!isValidIsoDate(date.c_str())) continue;
      snprintf(r.date, sizeof r.date, "%s", date.c_str());
    }
    sanitizeReminderText(line.c_str() + b + 1, r.text, sizeof r.text);
    if (!r.text[0] || (r.weekdays == 0) == (r.date[0] == '\0')) continue;
    out.items[out.count++] = r;
  }
}

bool addReminder(ReminderList& list, const Reminder& r) {
  if (list.count >= kMaxReminders) return false;
  list.items[list.count++] = r;
  return true;
}

bool removeReminder(ReminderList& list, int index) {
  if (index < 0 || index >= list.count) return false;
  for (int i = index; i + 1 < list.count; ++i) list.items[i] = list.items[i + 1];
  --list.count;
  return true;
}

bool reminderApplies(const Reminder& r, const char* isoDate, int weekday) {
  if (r.date[0]) return strcmp(r.date, isoDate) == 0;
  return weekday >= 0 && weekday < 7 && (r.weekdays >> weekday) & 1;
}

int remindersFor(const ReminderList& list, const char* isoDate, int weekday,
                 int* indices) {
  int n = 0;
  for (int i = 0; i < list.count; ++i)
    if (reminderApplies(list.items[i], isoDate, weekday)) indices[n++] = i;
  return n;
}

bool pruneExpiredReminders(ReminderList& list, const char* today) {
  if (!isValidIsoDate(today)) return false;
  bool removed = false;
  for (int i = list.count - 1; i >= 0; --i) {
    if (list.items[i].date[0] && strcmp(list.items[i].date, today) < 0) {
      removeReminder(list, i);
      removed = true;
    }
  }
  return removed;
}

std::string describeSchedule(const Reminder& r) {
  if (r.date[0]) return std::string(kShortDays[weekdayOf(r.date)]) + " " + r.date;
  switch (r.weekdays & 0x7F) {
    case 0x7F: return "Every day";
    case 0x3E: return "Weekdays";
    case 0x41: return "Weekends";
  }
  std::string s;
  for (int d = 0; d < 7; ++d) {
    if (!((r.weekdays >> d) & 1)) continue;
    if (!s.empty()) s += ", ";
    s += kShortDays[d];
  }
  return s;
}

}  // namespace door
