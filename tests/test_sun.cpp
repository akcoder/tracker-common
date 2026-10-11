// The Sun's elevation (tracker_sun.h, ported from sky-tracker) against known solar
// geometry. Everything here is a fact about
// the sky that can be checked by hand, so a wrong sign or a missing equation of
// time shows up as a number rather than as a screen that is slightly too dim.
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "../components/tracker_common/tracker_sun.h"

static int checks = 0, failures = 0;
static void okf(bool c, const char *fmt, ...) {
  checks++;
  if (!c) {
    failures++;
    va_list ap; va_start(ap, fmt);
    std::printf("  FAIL  "); std::vprintf(fmt, ap); std::printf("\n"); va_end(ap);
  }
}

using namespace tracker;

// days since the epoch for a civil date (Howard Hinnant)
static double day0(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned) (y - era * 400);
  const unsigned doy = (unsigned) ((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return ((double) era * 146097 + (double) doe - 719468) * 86400.0;
}

struct Day { double max_el, max_min, min_el; };

// One UTC day sampled every minute: the day's highest and lowest elevation and
// the minute (UTC) of the highest.
static Day scan(int y, int m, int d, double lat, double lon) {
  Day r{-999, 0, 999};
  const double t0 = day0(y, m, d);
  for (int mi = 0; mi < 1440; mi++) {
    const double e = tracker::sun::elevation(t0 + mi * 60.0, lat, lon);
    if (e > r.max_el) { r.max_el = e; r.max_min = mi; }
    if (e < r.min_el) r.min_el = e;
  }
  return r;
}

static void test_geometry() {
  std::printf("solar geometry\n");
  const double LAT = 61.581, LON = -149.439;      // the device's default

  const Day js = scan(2026, 6, 21, LAT, LON);
  okf(js.max_el > 51.7 && js.max_el < 52.0, "June solstice noon: 90-61.58+23.44 = 51.9, got %.2f", js.max_el);
  okf(js.max_min > 21 * 60 + 53 && js.max_min < 22 * 60 + 5,
      "...at ~21:58 UTC (longitude + equation of time), got minute %.0f", js.max_min);
  okf(js.min_el > -5.4 && js.min_el < -4.6, "midsummer midnight: only ~-5 deg, got %.2f", js.min_el);

  const Day ds = scan(2026, 12, 21, LAT, LON);
  okf(ds.max_el > 4.9 && ds.max_el < 5.4, "December solstice noon: ~5 deg (+refraction), got %.2f", ds.max_el);
  okf(ds.min_el < -51.0 && ds.min_el > -52.5, "winter midnight: ~-52, got %.2f", ds.min_el);

  const Day eq = scan(2026, 3, 20, 0.0, 0.0);
  okf(eq.max_el > 89.0 && eq.max_el < 90.1, "equator at the equinox: overhead, got %.2f", eq.max_el);

  // The equation of time. The Sun is 16 min ahead of the clock in early
  // November, so solar noon at Greenwich is ~11:44 UTC, not 12:00. A formula
  // without it (the first version here) puts it at 12:00.
  const Day nov = scan(2026, 11, 3, 0.0, 0.0);
  okf(nov.max_min > 11 * 60 + 40 && nov.max_min < 11 * 60 + 48,
      "3 Nov, Greenwich: solar noon ~11:44 UTC, got minute %.0f", nov.max_min);
  const Day feb = scan(2026, 2, 11, 0.0, 0.0);
  okf(feb.max_min > 12 * 60 + 11 && feb.max_min < 12 * 60 + 17,
      "11 Feb, Greenwich: ~12:14 UTC (the Sun is behind), got minute %.0f", feb.max_min);

  // East longitude is positive: 90 E has its noon six hours EARLIER in UTC.
  const Day east = scan(2026, 3, 20, 0.0, 90.0);
  okf(east.max_min > 5 * 60 + 55 && east.max_min < 6 * 60 + 15,
      "90 E: noon ~06:07 UTC, got minute %.0f", east.max_min);
  const Day west = scan(2026, 3, 20, 0.0, -90.0);
  okf(west.max_min > 17 * 60 + 55 && west.max_min < 18 * 60 + 15,
      "90 W: noon ~18:07 UTC, got minute %.0f", west.max_min);

  // The southern hemisphere has the opposite seasons.
  const Day ct = scan(2026, 6, 21, -33.9, 18.4);
  okf(ct.max_el > 32.3 && ct.max_el < 33.1, "Cape Town, June: 90-33.9-23.44 = 32.7, got %.2f", ct.max_el);
  const Day ct2 = scan(2026, 12, 21, -33.9, 18.4);
  okf(ct2.max_el > 79.0, "Cape Town, December: ~79.6, got %.2f", ct2.max_el);
}

int main() {
  std::printf("\nTracker Common - sun tests\n\n");
  test_geometry();
  std::printf("\n%d checks, %d failures\n\n", checks, failures);
  return failures ? 1 : 0;
}
