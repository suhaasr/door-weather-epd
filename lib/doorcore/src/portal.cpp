#include "portal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schedule.h"

namespace door {

namespace {

const char* const kDayLetters[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

uint32_t uniform(uint32_t (*random32)(), uint32_t n) {
  const uint32_t limit = UINT32_MAX - UINT32_MAX % n;
  uint32_t r;
  do r = random32(); while (r >= limit);
  return r % n;
}

bool tokenMatches(const PortalState& st, const std::string& t) {
  if (t.size() != strlen(st.token)) return false;
  unsigned diff = 0;
  for (size_t i = 0; i < t.size(); ++i) diff |= static_cast<unsigned char>(t[i] ^ st.token[i]);
  return diff == 0;
}

const char kStyle[] =
    "<style>"
    "body{font-family:-apple-system,system-ui,sans-serif;max-width:480px;margin:0 auto;"
    "padding:16px;color:#111;background:#fff}"
    "h1{font-size:1.4em;margin:.2em 0}h2{font-size:1.1em;margin:1.4em 0 .5em}"
    ".sub{color:#555;margin:0 0 1em}"
    ".msg{padding:10px 12px;border-radius:8px;margin:12px 0;background:#e8f5e9}"
    ".err{background:#fdecea}"
    "ul{list-style:none;padding:0;margin:0}"
    "li{display:flex;align-items:center;gap:8px;border:2px solid #111;border-radius:10px;"
    "padding:10px 12px;margin:8px 0}"
    "li div{flex:1}li b{display:block;font-size:1.1em}li span{color:#555;font-size:.9em}"
    "input[type=text],input[type=date],input[inputmode=numeric]{width:100%;box-sizing:border-box;"
    "font-size:1.1em;padding:10px;border:2px solid #111;border-radius:8px}"
    ".days{display:flex;gap:4px;margin:8px 0}"
    ".days label{flex:1}.days input{display:none}"
    ".days span{display:block;text-align:center;padding:10px 0;border:2px solid #111;"
    "border-radius:8px;font-weight:600}"
    ".days input:checked+span{background:#111;color:#fff}"
    "button{font-size:1.05em;padding:12px 16px;border-radius:8px;border:2px solid #111;"
    "background:#111;color:#fff;font-weight:600}"
    "button.alt{background:#fff;color:#111}"
    ".del{padding:6px 10px;font-size:.9em}"
    ".wide{width:100%;margin-top:10px}"
    ".or{color:#555;margin:10px 0 6px}"
    ".empty{color:#555;font-style:italic}"
    "</style>";

std::string page(const char* title, const std::string& content) {
  std::string s =
      "<!doctype html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'><title>";
  s += title;
  s += "</title>";
  s += kStyle;
  s += "</head><body>";
  s += content;
  s += "</body></html>";
  return s;
}

std::string loginPage(const PortalState& st, bool wrong) {
  std::string c = "<h1>Door Display</h1><p class='sub'>Enter the PIN shown on the display.</p>";
  if (wrong) {
    char buf[96];
    snprintf(buf, sizeof buf, "<p class='msg err'>Wrong PIN. %d tries left.</p>",
             kPortalMaxPinAttempts - st.failedPins);
    c += buf;
  }
  c += "<form method='post' action='/login'>"
       "<input name='pin' inputmode='numeric' pattern='[0-9]{6}' maxlength='6' "
       "autocomplete='one-time-code' required autofocus placeholder='6-digit PIN'>"
       "<button class='wide'>Continue</button></form>";
  return page("Door Display", c);
}

std::string closedPage(const char* title, const char* body) {
  std::string c = "<h1>";
  c += title;
  c += "</h1><p class='sub'>";
  c += body;
  c += "</p>";
  return page(title, c);
}

const char* messageFor(const std::string& code, bool& error) {
  error = code.compare(0, 2, "e_") == 0;
  if (code == "added") return "Reminder added.";
  if (code == "deleted") return "Reminder deleted.";
  if (code == "e_text") return "Enter some reminder text.";
  if (code == "e_when") return "Pick the weekdays it repeats on, or a single date.";
  if (code == "e_both") return "Pick weekdays or a date, not both.";
  if (code == "e_date") return "That date isn't valid.";
  if (code == "e_past") return "That date has already passed.";
  if (code == "e_full") return "The reminder list is full. Delete one first.";
  error = false;
  return nullptr;
}

std::string tokenField(const PortalState& st) {
  return std::string("<input type='hidden' name='t' value='") + st.token + "'>";
}

std::string mainPage(const PortalState& st, const std::string& msgCode) {
  const ReminderList& list = *st.list;
  std::string c = "<h1>Door Display reminders</h1><p class='sub'>";
  if (st.today[0]) {
    c += "Today is ";
    c += weekdayName(st.todayWeekday);
    c += " ";
    c += st.today;
    c += ". ";
  }
  c += "Reminders show on the display on their day.</p>";

  bool error = false;
  if (const char* m = messageFor(msgCode, error)) {
    c += error ? "<p class='msg err'>" : "<p class='msg'>";
    c += m;
    c += "</p>";
  }

  c += "<ul>";
  if (list.count == 0) c += "<li class='empty'>No reminders yet.</li>";
  for (int i = 0; i < list.count; ++i) {
    const Reminder& r = list.items[i];
    char idx[8];
    snprintf(idx, sizeof idx, "%d", i);
    c += "<li><div><b>" + htmlEscape(r.text) + "</b><span>" +
         htmlEscape(describeSchedule(r)) +
         "</span></div><form method='post' action='/delete'>" + tokenField(st) +
         "<input type='hidden' name='i' value='" + idx +
         "'><button class='alt del'>Delete</button></form></li>";
  }
  c += "</ul>";

  if (list.count < kMaxReminders) {
    char maxLen[8];
    snprintf(maxLen, sizeof maxLen, "%d", kReminderTextMax);
    c += "<h2>Add a reminder</h2><form method='post' action='/add' id='add'>" + tokenField(st) +
         "<input type='text' name='text' maxlength='" + maxLen +
         "' required placeholder='e.g. Pack lunch'>"
         "<p class='or'>Repeat every</p><div class='days'>";
    for (int d = 0; d < 7; ++d) {
      char cb[96];
      snprintf(cb, sizeof cb, "<label><input type='checkbox' name='d%d' value='1'><span>%s</span></label>",
               d, kDayLetters[d]);
      c += cb;
    }
    c += "</div><p class='or'>or only on</p><input type='date' name='date'";
    if (st.today[0]) c += std::string(" min='") + st.today + "'";
    c += "><button class='wide'>Add reminder</button></form>"
         "<script>"
         "var f=document.getElementById('add'),d=f.date,b=f.querySelectorAll('.days input');"
         "d.onchange=function(){if(d.value)b.forEach(function(x){x.checked=false})};"
         "b.forEach(function(x){x.onchange=function(){if(x.checked)d.value=''}});"
         "</script>";
  } else {
    c += "<p class='msg'>The list is full. Delete a reminder to add another.</p>";
  }

  c += "<h2>Finished?</h2><form method='post' action='/done'>" + tokenField(st) +
       "<button class='wide'>Done, update the display</button></form>"
       "<form method='post' action='/wifi'>" + tokenField(st) +
       "<button class='alt wide'>Change Wi-Fi or ZIP code</button></form>";
  return page("Door Display reminders", c);
}

PortalResponse redirect(const PortalState& st, const char* msg) {
  PortalResponse r;
  r.status = 303;
  r.location = std::string("/?t=") + st.token + (msg ? std::string("&m=") + msg : "");
  return r;
}

PortalResponse ok(std::string body, int status = 200) {
  PortalResponse r;
  r.status = status;
  r.body = std::move(body);
  return r;
}

}  // namespace

std::string htmlEscape(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c;
    }
  }
  return out;
}

void initPortal(PortalState& st, ReminderList* list, uint32_t (*random32)()) {
  memset(&st, 0, sizeof st);
  st.list = list;
  st.todayWeekday = -1;
  for (int i = 0; i < 6; ++i) st.pin[i] = static_cast<char>('0' + uniform(random32, 10));
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < 16; ++i) st.token[i] = kHex[uniform(random32, 16)];
}

PortalResponse handlePortalRequest(PortalState& st, const char* method, const char* path,
                                   const std::function<std::string(const char*)>& arg) {
  const bool post = strcmp(method, "POST") == 0;
  if (st.done) {
    return ok(closedPage("Reminders closed",
                         "To make more changes, press RESET on the display, then hold BOOT."),
              410);
  }
  if (strcmp(path, "/") == 0 && !post) {
    if (tokenMatches(st, arg("t"))) return ok(mainPage(st, arg("m")));
    return ok(loginPage(st, false));
  }
  if (strcmp(path, "/login") == 0 && post) {
    if (arg("pin") == st.pin) return redirect(st, nullptr);
    if (++st.failedPins >= kPortalMaxPinAttempts) {
      st.done = true;
      return ok(closedPage("Too many wrong PINs",
                           "The reminders page is closed. Press RESET on the display, then "
                           "hold BOOT to get a new PIN."),
                403);
    }
    return ok(loginPage(st, true), 403);
  }
  if (!post || (strcmp(path, "/add") && strcmp(path, "/delete") && strcmp(path, "/done") &&
                strcmp(path, "/wifi"))) {
    return ok(closedPage("Not found", "<a href='/'>Back</a>"), 404);
  }
  if (!tokenMatches(st, arg("t"))) return ok(loginPage(st, false), 403);

  if (strcmp(path, "/add") == 0) {
    if (st.list->count >= kMaxReminders) return redirect(st, "e_full");
    Reminder r = {};
    sanitizeReminderText(arg("text").c_str(), r.text, sizeof r.text);
    if (!r.text[0]) return redirect(st, "e_text");
    for (int d = 0; d < 7; ++d) {
      char name[4];
      snprintf(name, sizeof name, "d%d", d);
      if (!arg(name).empty()) r.weekdays |= 1 << d;
    }
    std::string date = arg("date");
    if (!date.empty()) {
      if (!isValidIsoDate(date.c_str())) return redirect(st, "e_date");
      if (st.today[0] && strcmp(date.c_str(), st.today) < 0) return redirect(st, "e_past");
      snprintf(r.date, sizeof r.date, "%s", date.c_str());
    }
    if (r.weekdays && r.date[0]) return redirect(st, "e_both");
    if (!r.weekdays && !r.date[0]) return redirect(st, "e_when");
    addReminder(*st.list, r);
    st.changed = true;
    return redirect(st, "added");
  }
  if (strcmp(path, "/delete") == 0) {
    std::string i = arg("i");
    if (!i.empty() && removeReminder(*st.list, atoi(i.c_str()))) st.changed = true;
    return redirect(st, "deleted");
  }
  if (strcmp(path, "/done") == 0) {
    st.done = true;
    return ok(closedPage("All set", "The display is updating now. You can close this page."));
  }
  // /wifi
  st.done = true;
  st.wifiSetupRequested = true;
  return ok(closedPage("Wi-Fi setup",
                       "Look at the display: it now shows the name and password of a setup "
                       "hotspot. Join it to change the Wi-Fi network or ZIP code."));
}

}  // namespace door
