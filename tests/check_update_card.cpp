// The update card builds on the host against a stand-in entity (tracker::upd::HostUpdate). A syntax
// and link-free compile check: behaviour tests need an LVGL display, which the project harness has
// (sky-tracker's host tests cover UI-68 with it). make -C tests check_card LVGL_DIR=<lvgl>  CONF_DIR=<dir with lv_conf.h>
#define TRACKER_HOST_TEST 1
#include "lvgl.h"
#include "tracker_update.h"
int main() {
  tracker::upd::HostUpdate u;
  tracker::upd::init(&u, nullptr, nullptr, nullptr, nullptr);
  tracker::upd::check_now();
  tracker::upd::idle_close();
  tracker::upd::run_hook = nullptr;
  return 0;
}
