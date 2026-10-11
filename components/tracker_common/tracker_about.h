#pragma once
// The About page's device lines, in sky-tracker's form (UI-67, UI-17a), shared by the trackers:
//
//   name  ip
//   Wi-Fi ssid  -52 dBm
//   MAC 24:EC:4A:9C:AD:68
//   Up 3h 12m
//
// and the build date as "Oct 2 2026" (the compiler's "Oct  2 2026"). LVGL-free, so host tests can
// build it.
#include <algorithm>
#include <cstddef>
#include <cstdio>

namespace tracker {
namespace about {

// UI-17a: every duration on the device is written this one way: "45s", "12m", "1h 31m",
// "2d 4h" (a space between units, minutes after hours).
inline void fmt_dur(double s, char *buf, size_t n) {
  if (s < 0) s = 0;
  const int m = (int) (s / 60.0);
  if (m >= 24 * 60) std::snprintf(buf, n, "%dd %dh", m / 1440, (m % 1440) / 60);
  else if (m >= 60) std::snprintf(buf, n, "%dh %dm", m / 60, m % 60);
  else if (s >= 60) std::snprintf(buf, n, "%dm", m);
  else std::snprintf(buf, n, "%ds", (int) s);
}

inline void device_text(char *b, size_t n, const char *name, const char *ip, const char *ssid, int rssi,
                        const char *mac, double uptime_s) {
  char up[24];
  fmt_dur(uptime_s, up, sizeof(up));
  std::snprintf(b, n, "%s  %s\nWi-Fi %s  %d dBm\nMAC %s\nUp %s", name, ip && *ip ? ip : "-",
                ssid && *ssid ? ssid : "-", rssi, mac, up);
}

inline void tidy_date(const char *d, char *out, size_t n) {
  char m[4] = "";
  int day = 0, y = 0;
  if (std::sscanf(d, "%3s %d %d", m, &day, &y) == 3) std::snprintf(out, n, "%s %d %d", m, day, y);
  else std::snprintf(out, n, "%s", d);
}

}  // namespace about
}  // namespace tracker
