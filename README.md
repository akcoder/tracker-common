# Tracker Common

The pieces shared by the tracker projects — [`f1-tracker`](https://github.com/akcoder/f1-tracker)
and `sky-tracker` — as an ESPHome external component, so a fix lands once.

```yaml
external_components:
  - source: github://akcoder/tracker-common@main
    components: [tracker_common]
tracker_common:
```

| Header | Contents |
|---|---|
| `tracker_updlogic.h` | firmware updates without a screen: version compare, **only a newer version counts**, release notes folded to the fonts, the prompt's state machine |
| `tracker_update.h` | the update card (checking / up to date / available with notes / failed), the icon beside the gear, the install card with its bar |
| `tracker_web.h` | the web page's tab icon and header badge, and the header logo's link to the project's GitHub page |

Everything is in `namespace tracker`; a project passes its own fonts, icon and URL:

```cpp
tracker::upd::init(id(fw_update), title_font, body_font, small_font, icon_font);
tracker::web::install({FAVICON_SVG, HEADER_SVG, HEADER_SVG_LEN, "https://github.com/me/project"});
```

The update card's geometry is a set of named constants (`tracker::upd::CARD_W` …) so a project's
render tool can read them rather than carry a copy that drifts.

```bash
make -C tests      # the update logic, host-side
```

Specs: `sky-tracker` UI-68, 68a, 68b, 68c; `f1-tracker` decisions 183, 184, 194.
Keep both in step when the update screen changes.

[MIT](LICENSE).
