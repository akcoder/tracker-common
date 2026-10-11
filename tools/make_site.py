#!/usr/bin/env python3
"""Build a tracker's install page (the GitHub Pages site root) from the shared template.

  make_site.py PROJECT_DIR/docs/site.json OUTDIR

site.json (all text is HTML written by the project):
  name, slug (the YAML name, e.g. "f1-tracker"), repo ("owner/name"), tagline, description,
  ap_name ("F1 Tracker - XXXX"), logo, favicon (file names, copied beside the page),
  hero {file, alt}, features [{title, text}], after_features (HTML), sources [{name, url, used, licence}],
  sources_note, footer (HTML <p> lines), ota_file ("f1-tracker-esp32s3.ota.bin"), sdcard (bool),
  assets [paths relative to site.json's folder, copied into OUTDIR]
"""
import json, os, re, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__))
TEMPLATE = os.path.join(HERE, "..", "site", "index.template.html")


def esc_attr(s):
    return s.replace("&", "&amp;").replace('"', "&quot;")


def build(cfg):
    p = dict(cfg)
    p["description"] = esc_attr(p.get("description", p.get("tagline", "")))
    hero = p.get("hero")
    p["hero"] = (f'  <img class="shot" src="{hero["file"]}" alt="{esc_attr(hero.get("alt", ""))}">\n\n' if hero else "")
    p["features"] = "".join(
        f'    <li><strong>{f["title"]}</strong> &mdash; {f["text"]}</li>\n' for f in p.get("features", []))
    p["after_features"] = p.get("after_features", "")
    rows = []
    has_lic = any(s.get("licence") for s in p.get("sources", []))
    rows.append("    <tr><th>Source</th><th>Used for</th>" + ("<th>Licence</th>" if has_lic else "") + "</tr>\n")
    for s in p.get("sources", []):
        nm = f'<a href="{s["url"]}">{s["name"]}</a>' if s.get("url") else s["name"]
        rows.append(f'    <tr><td>{nm}</td><td>{s["used"]}</td>' + (f'<td>{s.get("licence", "")}</td>' if has_lic else "") + "</tr>\n")
    p["sources"] = "".join(rows)
    p["sources_note"] = f'  <p class="muted">{p["sources_note"]}</p>' if p.get("sources_note") else ""
    p["footer"] = p.get("footer", "")
    if p.get("sdcard"):
        p["sdcard"] = (f' With no network, copy <code>{p["ota_file"]}</code> from the '
                       f'<a href="https://github.com/{p["repo"]}/releases/latest">latest release</a> '
                       "to the root of a microSD card and put it in the slot.")
    else:
        p["sdcard"] = ""
    html = open(TEMPLATE, encoding="utf-8").read()
    html = re.sub(r"\{\{(\w+)\}\}", lambda m: str(p[m.group(1)]) if m.group(1) in p else sys.exit(f"site.json lacks {m.group(1)}"), html)
    return html


def main():
    cfg_path, out = sys.argv[1], sys.argv[2]
    cfg = json.load(open(cfg_path, encoding="utf-8"))
    base = os.path.dirname(os.path.abspath(cfg_path))
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, "index.html"), "w", encoding="utf-8").write(build(cfg))
    for a in cfg.get("assets", []):
        shutil.copy2(os.path.join(base, a), os.path.join(out, os.path.basename(a)))


if __name__ == "__main__":
    main()
