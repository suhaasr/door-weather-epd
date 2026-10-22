// Serves the reminders page on localhost for trying it in a browser:
//   build/server [port]   then open http://localhost:<port> (PIN is printed)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <map>
#include <string>

#include "portal.h"
#include "schedule.h"

using namespace door;

static uint32_t hostRandom() { return static_cast<uint32_t>(random()); }

static std::string urlDecode(const std::string& s) {
  std::string out;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '+') {
      out += ' ';
    } else if (s[i] == '%' && i + 2 < s.size()) {
      out += static_cast<char>(strtol(s.substr(i + 1, 2).c_str(), nullptr, 16));
      i += 2;
    } else {
      out += s[i];
    }
  }
  return out;
}

static void parseParams(const std::string& q, std::map<std::string, std::string>& out) {
  size_t p = 0;
  while (p < q.size()) {
    size_t amp = q.find('&', p);
    if (amp == std::string::npos) amp = q.size();
    std::string kv = q.substr(p, amp - p);
    size_t eq = kv.find('=');
    if (eq != std::string::npos) out[urlDecode(kv.substr(0, eq))] = urlDecode(kv.substr(eq + 1));
    p = amp + 1;
  }
}

int main(int argc, char** argv) {
  int port = argc > 1 ? atoi(argv[1]) : 8080;
  srandom(static_cast<unsigned>(time(nullptr)));
  ReminderList list = {};
  parseReminders("20||Pack lunch\n0|2026-10-30|Dentist 9:30 AM\n", list);
  ReminderPortal portal(&list, hostRandom);
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char today[11];
  formatIsoDate(t, today, sizeof today);
  portal.setToday(today, t.tm_wday);

  int s = socket(AF_INET, SOCK_STREAM, 0);
  int one = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof addr) || listen(s, 8)) {
    perror("bind");
    return 1;
  }
  printf("http://localhost:%d  PIN %s\n", port, portal.pin());
  fflush(stdout);
  for (;;) {
    int c = accept(s, nullptr, nullptr);
    if (c < 0) continue;
    std::string req;
    char buf[4096];
    ssize_t n;
    size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos && (n = read(c, buf, sizeof buf)) > 0) {
      req.append(buf, n);
      headerEnd = req.find("\r\n\r\n");
    }
    if (headerEnd == std::string::npos) { close(c); continue; }
    size_t len = 0;
    const char* cl = strcasestr(req.c_str(), "content-length:");
    if (cl) len = strtoul(cl + 15, nullptr, 10);
    while (req.size() < headerEnd + 4 + len && (n = read(c, buf, sizeof buf)) > 0) req.append(buf, n);

    std::string method = req.substr(0, req.find(' '));
    size_t ps = req.find(' ') + 1;
    std::string target = req.substr(ps, req.find(' ', ps) - ps);
    std::string path = target.substr(0, target.find('?'));
    std::map<std::string, std::string> params;
    if (target.find('?') != std::string::npos) parseParams(target.substr(target.find('?') + 1), params);
    parseParams(req.substr(headerEnd + 4, len), params);
    auto arg = [&](const char* name) {
      auto it = params.find(name);
      return it == params.end() ? std::string() : it->second;
    };
    PortalResponse r = portal.handle(method.c_str(), path.c_str(), arg);
    if (portal.takeChanged()) printf("saved:\n%s", serializeReminders(list).c_str());
    char head[512];
    snprintf(head, sizeof head,
             "HTTP/1.0 %d X\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\n"
             "%s%s%sContent-Length: %zu\r\nConnection: close\r\n\r\n",
             r.status, r.location.empty() ? "" : "Location: ", r.location.c_str(),
             r.location.empty() ? "" : "\r\n", r.body.size());
    std::string resp = head + r.body;
    (void)!write(c, resp.data(), resp.size());
    close(c);
    printf("%s %s -> %d\n", method.c_str(), path.c_str(), r.status);
    fflush(stdout);
  }
}
