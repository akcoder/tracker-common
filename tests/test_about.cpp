// The About page's device text and durations (tracker_about.h).
#include <cstdio>
#include <cstring>
#include "../components/tracker_common/tracker_about.h"
using namespace tracker::about;
static int checks = 0, failures = 0;
static void is(const char *got, const char *want) {
  checks++;
  if (std::strcmp(got, want) != 0) { failures++; std::printf("  FAIL  got \"%s\", want \"%s\"\n", got, want); }
}
int main() {
  std::printf("\nTracker Common - About text tests\n\n");
  char b[160];
  fmt_dur(-5, b, sizeof(b)); is(b, "0s");
  fmt_dur(45, b, sizeof(b)); is(b, "45s");
  fmt_dur(60, b, sizeof(b)); is(b, "1m");
  fmt_dur(12 * 60 + 59, b, sizeof(b)); is(b, "12m");
  fmt_dur(3600, b, sizeof(b)); is(b, "1h 0m");
  fmt_dur(91 * 60, b, sizeof(b)); is(b, "1h 31m");
  fmt_dur(24 * 3600, b, sizeof(b)); is(b, "1d 0h");
  fmt_dur(52 * 3600 + 1800, b, sizeof(b)); is(b, "2d 4h");
  device_text(b, sizeof(b), "f1-tracker-9cad68", "192.168.1.52", "home-ssid", -52, "24:EC:4A:9C:AD:68", 3 * 3600 + 12 * 60);
  is(b, "f1-tracker-9cad68  192.168.1.52\nWi-Fi home-ssid  -52 dBm\nMAC 24:EC:4A:9C:AD:68\nUp 3h 12m");
  device_text(b, sizeof(b), "x", "", nullptr, -90, "m", 5);
  is(b, "x  -\nWi-Fi -  -90 dBm\nMAC m\nUp 5s");
  tidy_date("Oct  2 2026", b, sizeof(b)); is(b, "Oct 2 2026");
  tidy_date("garbage", b, sizeof(b)); is(b, "garbage");
  std::printf("\n%d checks, %d failures\n\n", checks, failures);
  return failures ? 1 : 0;
}
