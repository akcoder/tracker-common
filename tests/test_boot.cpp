// The boot / Wi-Fi status page's state machine (tracker_boot.h).
#include <cstdio>
#include <cstring>
#include "../components/tracker_common/tracker_boot.h"
using namespace tracker::boot;
static int checks = 0, failures = 0;
#define OK(c, m) do { checks++; if (!(c)) { failures++; std::printf("  FAIL  %s\n", m); } } while (0)
int main() {
  std::printf("\nTracker Common - boot status tests\n\n");
  Tracker t;
  OK(classify(true, false) == CONNECTED && classify(false, true) == AP_UP && classify(false, false) == CONNECTING, "classify");
  Step s = step(t, CONNECTING, 1000, true, false, true);
  OK(s.page == STAY && s.link_changed, "offline on the status page: stay, link changed");
  s = step(t, CONNECTED, 5000, true, false, true);
  OK(s.page == TO_RACE, "connected, race page primary");
  Tracker t2;
  s = step(t2, CONNECTED, 5000, true, false, false);
  OK(s.page == TO_CIRCUIT, "connected, other page primary");
  s = step(t2, CONNECTED, 5000, false, false, false);
  OK(s.page == STAY, "connected and already off the status page: stay");
  // 10 s offline brings the status page back, never over settings; an even start time is not a bug
  Tracker t3;
  step(t3, CONNECTING, 2000, false, false, true);
  OK(step(t3, CONNECTING, 11000, false, false, true).page == STAY, "9 s offline: not yet");
  OK(step(t3, CONNECTING, 12001, false, false, true).page == TO_STATUS, "10 s offline: the status page returns");
  Tracker t4;
  step(t4, CONNECTING, 2000, false, true, true);
  OK(step(t4, CONNECTING, 99000, false, true, true).page == STAY, "never over the settings page");
  Tracker t5;
  step(t5, CONNECTING, 0, false, false, true);
  OK(t5.lost_since == 1, "a start at t=0 is stored as 1");
  char b[64];
  body_for(AP_UP, "x", "F1 Tracker - 12AB", b, sizeof(b));
  OK(std::strstr(b, "Join 'F1 Tracker - 12AB'") != nullptr, "AP body names the network to join");
  body_for(CONNECTING, "", nullptr, b, sizeof(b));
  OK(std::strcmp(b, "Waiting for Wi-Fi details") == 0, "no credentials says so");
  std::printf("\n%d checks, %d failures\n\n", checks, failures);
  return failures ? 1 : 0;
}
