# Tracker Common

The pieces shared by the tracker projects — [`f1-tracker`](https://github.com/akcoder/f1-tracker)
and `sky-tracker` — as an ESPHome external component, so a fix lands once.

```yaml
external_components:
  - source: github://akcoder/tracker-common@main
    components: [tracker_common]
tracker_common:
```

| File | Contents |
|---|---|
| `tracker_updlogic.h` | firmware updates without a screen: version compare, **only a newer version counts**, release notes folded to the fonts, the prompt's state machine |
| `tracker_update.h` | the update card (checking / up to date / available with notes / failed), the icon beside the gear, the install card with its bar |
| `tracker_web.h` | the web page's tab icon and header badge, and the header logo's link to the project's GitHub page |
| `tracker_jpg.h` | baseline JPEG from memory with the ROM's TJpgDec: `decode_half`, `decode_full`, `decode_fit` (box filter, window, stride), `decode_copy`; progress rows, cancel, decode-while-downloading |
| `tracker_png.h` | streaming PNG decoder: palette + tRNS, 8-bit grey/RGB/RGBA, crop, alpha over a background, box-filtered to the target, a callback per finished row |
| `tracker_sun.h` | the Sun's elevation and position from a latitude, longitude and the clock (auto-dim; the Moon's phase needs `lon_ecl`) |
| `tracker_panel.h` | the ST7701S soft reset for the Guition 4848S040, before the SPI bus claims the pins |
| `tracker_boot.h` | the boot / Wi-Fi status page's state machine and its two lines of text |
| `fonts.yaml` | the shared font set, an ESPHome package: Roboto Mono 12/14/16/18/24, Roboto Light 34, Roboto 22 |
| `tools/svgpath.py` | a small SVG path reader for the projects' logo generators |

Everything is in `namespace tracker`; a project passes its own fonts, icon and URL:

```cpp
tracker::upd::init(id(fw_update), title_font, body_font, small_font, icon_font);
tracker::web::install({FAVICON_SVG, HEADER_SVG, HEADER_SVG_LEN, "https://github.com/me/project"});
```

**Fonts, the same on every tracker:** title Roboto Light 34, all other card text (lines, buttons,
release notes) Roboto Mono 16. Pass those to `tracker::upd::init`; the 12 px font is not used.

The update card's geometry is a set of named constants (`tracker::upd::CARD_W` …) so a project's
render tool can read them rather than carry a copy that drifts.

Fonts are a package, not an include:

```yaml
packages:
  tracker_fonts:
    url: https://github.com/akcoder/tracker-common
    ref: main
    files: [fonts.yaml]
```

```bash
make -C tests      # update logic, sun, PNG and boot state machine, host-side
```

The JPEG decoder's tests need LVGL's TJpgDec and a picture, so they live with the project that has
both (f1-tracker's `tests/test_jpg.cpp`, 225 checks).

Specs: `sky-tracker` UI-68, 68a, 68b, 68c; `f1-tracker` decisions 183, 184, 194.
Keep both in step when the update screen changes.

[MIT](LICENSE).
