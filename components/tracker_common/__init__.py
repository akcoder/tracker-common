"""Tracker Common: the pieces the tracker projects (sky-tracker, f1-tracker) share, as an ESPHome
external component. It only has to make the headers part of the build, in dependency order, so
the YAML's lambdas can call them:

* tracker_updlogic.h  firmware updates, the part testable without a screen (version compare,
                      "only a NEWER version counts", release notes, the prompt's state machine)
* tracker_update.h    the update card, the icon beside the gear, the install card (LVGL)
* tracker_web.h       the web page's tab icon and header badge, and the header logo's link
                      to the project's GitHub page

Everything is namespace tracker::; a project passes its own fonts, icon and URL.
"""
import esphome.codegen as cg
import esphome.config_validation as cv

CODEOWNERS = ["@akcoder"]
DEPENDENCIES = ["lvgl"]

CONFIG_SCHEMA = cv.Schema({})

HEADERS = ["tracker_updlogic.h", "tracker_update.h", "tracker_web.h"]


async def to_code(config):
    for header in HEADERS:
        cg.add_global(cg.RawStatement(f'#include "esphome/components/tracker_common/{header}"'))
