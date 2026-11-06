#!/usr/bin/env python3
"""Self-test for the Study B kit. Standard library only, plus Node.js on PATH.

Builds a synthetic stimulus folder in a temporary directory: tiny generated
PNGs under the capture script's real paths, a manifest shaped like the one
capture-study-b.ps1 writes, and a regions.json beside it with mistakes, a
not-standable object, seats and crops. It then runs the pure logic of
index.html under Node to plan trials for twelve numbered participants and turn
scripted answers into results files exactly as the page would export them,
then runs analyse.py and checks every rule comes out as designed: V1 passes
everything and V1b fails everything. Further checks cover the counterbalancing,
the matched contexts, the display scale and the analysis edge cases.

Nothing here is participant data and nothing is written to the repository.

    python selftest.py [--keep DIR]
"""

import argparse
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from unittest import mock

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True  # keep __pycache__ out of the repository
import analyse  # noqa: E402

# The six pairs in game/objects/team_colour.h.
PALETTES = ["blue_orange", "pink_green", "blue_yellow", "purple_gold", "red_azure", "magenta_forest"]
STAGE = "crossfeed"
FRAMES = ["clean", "heavy"]
VARIANTS = ["V0", "V1", "V1b"]
NUMBERS = list(range(1, 13))           # 12 participants: whole cycles of 4 rows and of 3 variants
PARTICIPANTS = ["T-%02d" % n for n in NUMBERS]
SEAT_RECTS = {1: [0.05, 0.05, 0.25, 0.45], 2: [0.30, 0.05, 0.45, 0.45],
              3: [0.55, 0.55, 0.70, 0.95], 4: [0.75, 0.55, 0.95, 0.95]}
CLOUD = [0.10, 0.10, 0.20, 0.20]     # backdrop mistake in V1 and V1b
BRICK = [0.50, 0.80, 0.60, 0.90]     # gameplay-layer mistake, V1b only
TREE = [0.70, 0.40, 0.80, 0.50]      # not standable, every variant


def png(path, w, h, rgb):
    raw = b"".join(b"\x00" + bytes(rgb) * w for _ in range(h))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                 chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def build_fixture(root):
    stim = os.path.join(root, "stimuli")
    images = []
    colour = {"V0": (200, 200, 200), "V1": (180, 210, 180), "V1b": (210, 180, 180)}
    for v in VARIANTS:
        for pal in PALETTES:
            for frame in FRAMES:
                for players, cues in ((4, "on"), (4, "off"), (2, "on")):
                    # The capture script's layout: <variant>/<res>/<n>p/<palette>/<stage>-<frame>-cues-<on|off>.png
                    sub = "%s/1280x720/%dp/%s/%s-%s-cues-%s.png" % (v, players, pal, STAGE, frame, cues)
                    png(os.path.join(stim, *sub.split("/")), 32, 18, colour[v])
                    images.append({"variant": v, "revision": "0" * 40, "resolution": "1280x720", "players": players,
                                   "palette": pal, "stage": STAGE, "shot": "%s-%s" % (STAGE, frame), "frame": frame,
                                   "identity_cues": cues == "on", "seed": 7, "path": sub})
    # One listed image whose file is missing: the page must skip it.
    images.append({"variant": "V1", "resolution": "1920x1080", "players": 1, "palette": "purple_gold", "stage": STAGE,
                   "shot": STAGE + "-clean", "frame": "clean", "identity_cues": True,
                   "path": "V1/1920x1080/1p/purple_gold/crossfeed-clean-cues-on.png"})
    manifest = {"study": "B", "seed": 7, "preset": "x64-debug",
                "variants": {v: {"revision": "0" * 40, "note": "fixture"} for v in VARIANTS},
                "images": sorted(images, key=lambda m: m["path"])}
    with open(os.path.join(stim, "manifest.json"), "w", encoding="utf-8") as fh:
        json.dump(manifest, fh, indent=1)
    regions = {"regions": [
        {"id": "cloud_big_bump", "kind": "mistake", "layer": "backdrop", "where": {"variants": ["V1", "V1b"]}, "rect": CLOUD},
        {"id": "brick_slip", "kind": "mistake", "layer": "gameplay", "where": {"variant": "V1b"}, "rect": BRICK},
        {"id": "tree_left", "kind": "not_standable", "rect": TREE},
        # A where written from an out-of-date example: it matches nothing and must be reported.
        {"id": "stale_example", "kind": "mistake", "layer": "backdrop", "where": {"shot": "clean"}, "rect": CLOUD},
    ] + [
        {"kind": "seat", "seat": s, "side": 0 if s <= 2 else 1, "where": {"players": 4, "identity_cues": False},
         "rect": SEAT_RECTS[s]} for s in SEAT_RECTS
    ] + [
        {"id": "fig_left", "kind": "crop", "side": 0, "where": {"players": 4, "identity_cues": True, "shot": STAGE + "-heavy"},
         "rect": [0.1, 0.1, 0.3, 0.5]},
        {"id": "fig_right", "kind": "crop", "side": 1, "where": {"players": 4, "identity_cues": True, "shot": STAGE + "-heavy"},
         "rect": [0.7, 0.1, 0.9, 0.5]},
    ]}
    regions_path = os.path.join(stim, "regions.json")
    with open(regions_path, "w", encoding="utf-8") as fh:
        json.dump(regions, fh, indent=1)
    return stim, manifest, regions, regions_path


def extract_page_script(dest):
    with open(os.path.join(HERE, "index.html"), encoding="utf-8") as fh:
        html = fh.read()
    scripts = re.findall(r"<script>(.*?)</script>", html, re.S)
    if len(scripts) != 1:
        raise SystemExit("expected exactly one inline <script> in index.html, found %d" % len(scripts))
    with open(dest, "w", encoding="utf-8") as fh:
        fh.write(scripts[0])
    return dest, html


# Scripted participants. The policy lives in JavaScript so the records are made
# by the page's own recordFor(), exactly as an export would make them.
HARNESS = r"""
const fs = require('fs');
const StudyB = require(process.argv[2]);
const manifest = JSON.parse(fs.readFileSync(process.argv[3], 'utf8'));
const regions = JSON.parse(fs.readFileSync(process.argv[4], 'utf8'));
const outDir = process.argv[5];
const people = JSON.parse(process.argv[6]);
const C = JSON.parse(process.argv[7]);
const onDisk = new Set(C.on_disk);
const norm = StudyB.prepare(manifest, p => onDisk.has(p), regions);
const centre = r => ({ x: +((r[0] + r[2]) / 2).toFixed(4), y: +((r[1] + r[3]) / 2).toFixed(4) });
const summary = { warnings: norm.warnings, seats: norm.seats.length, crops: norm.crops.length, images: norm.images.length,
  plans: {}, units: {} };
const fit = { display: { scale: 1, css_w: 1280, css_h: 720, dpr: 1, natural: [1280, 720], fits: true, window: [1920, 1080] } };
people.forEach(([pid, n], i) => {
  const plan = StudyB.buildPlan(norm, pid, n);
  const again = StudyB.buildPlan(norm, pid, n);
  if (JSON.stringify(plan) !== JSON.stringify(again)) throw new Error('plan not deterministic for ' + pid);
  const res = StudyB.newResults(plan, { consent: true, started: '2026-11-01T00:00:00Z', manifest_images: norm.images.length, warnings: norm.warnings });
  const count = {};
  const units = {};
  plan.blocks.forEach(b => b.trials.forEach(t => {
    const v = t.image ? t.image.variant : null;
    const key = t.test + '|' + v;
    const c = count[key] = (count[key] || 0) + 1;
    if (['bug', 'stand', 'find', 'team'].includes(t.test)) {
      const unit = (t.test === 'team' ? t.crop_id + '@' : '') + StudyB.contextKey(t.image) + (t.test === 'find' ? '#seat' + t.seat.seat : '');
      (units[t.test] = units[t.test] || []).push([unit, v]);
    }
    let r = Object.assign({ rt_ms: 1500 }, fit);
    switch (t.test) {
      case 'practice': r = { click: { x: 0.52, y: 0.4 }, hit: true, rt_ms: 900 }; break;
      case 'impressions':
        r.text = 'A bright, cheerful drawing of a little platform game.';
        if (v === 'V1' && i === 0) r.text = 'Looks a bit like a glitch near the cloud.';
        if (v === 'V1b' && i < 3) r.text = 'The bricks look glitchy and broken.';
        break;
      case 'paired': {
        const pick = v === 'V1' ? ((c + i) % 5 !== 0) : (c % 2 === 0);
        r.chosen = pick ? v : 'V0'; r.why = ''; r.switches = 3; break;
      }
      case 'bug':
        r.no_bug = true;
        if (i === 0 && v === 'V1') r = Object.assign({ no_bug: false, click: centre(C.cloud), rt_ms: 3000 }, fit);
        if (i === 0 && v === 'V1b') r = Object.assign({ no_bug: false, click: centre(C.brick), rt_ms: 3000 }, fit);
        if ((i === 1 || i === 2) && v === 'V1b') r = Object.assign({ no_bug: false, click: centre(C.cloud), rt_ms: 3000 }, fit);
        break;
      case 'stand':
        r.clicks = [{ x: 0.5, y: 0.97, t_ms: 800 }];
        if (i === 1 && v === 'V1b') r.clicks.push(Object.assign(centre(C.tree), { t_ms: 1200 }));
        break;
      case 'find':
        if (!t.seat.rect) throw new Error('find trial without a seat rect: ' + JSON.stringify(t.seat));
        r.click = centre(t.seat.rect);
        r.rt_ms = (v === 'V1b' ? 2000 : 1000) + 100 * t.seat.seat;
        break;
      case 'team':
        r.answer = t.truth;
        if (v === 'V1b' && i < 6 && t.image.palette === 'purple_gold') r.answer = t.options.find(o => o !== t.truth);
        break;
    }
    res.trials.push(StudyB.recordFor(t, r, res.trials.length));
  }));
  res.complete = true; res.finished = '2026-11-01T00:20:00Z';
  fs.writeFileSync(require('path').join(outDir, 'study-b-' + pid + '.json'), JSON.stringify(res, null, 2));
  summary.units[pid] = units;
  summary.plans[pid] = { number: n, order: plan.order, order_row: plan.order_row, rotation: plan.rotation, total: plan.total,
    notes: plan.notes,
    first_v0: plan.blocks.flatMap(b => b.trials).filter(t => t.test === 'paired' && t.first === 'V0').length,
    paired: plan.blocks.flatMap(b => b.trials).filter(t => t.test === 'paired').length };
});
// Without a regions file: no team block, and a note that find clicks cannot be scored.
const bare = StudyB.prepare({ images: manifest.images.filter(m => onDisk.has(m.path)) }, null, null);
const barePlan = StudyB.buildPlan(bare, 'T-bare', 1);
summary.bare_blocks = barePlan.blocks.map(b => b.test);
summary.bare_notes = barePlan.notes;
summary.latin = StudyB.LATIN.map(row => row.map(k => StudyB.TESTS[k]));
summary.fit = {
  exact: StudyB.displayFit(1280, 720, 1600, 800, 1),
  double: StudyB.displayFit(1280, 720, 1600, 800, 2),
  hidpi: StudyB.displayFit(1280, 720, 1200, 700, 1.25),
  toobig: StudyB.displayFit(1920, 1080, 1570, 900, 1)
};
process.stdout.write(JSON.stringify(summary));
"""


def check(cond, msg, failures):
    print(("ok    " if cond else "FAIL  ") + msg)
    if not cond:
        failures.append(msg)


def run_json(argv):
    """Runs analyse.main and returns its exit code and --json output."""
    rc = analyse.main(argv)
    path = argv[argv.index("--json") + 1]
    with open(path, encoding="utf-8") as fh:
        return rc, json.load(fh)


def page_checks(summary, html, failures):
    check(any("Missing file" in w for w in summary["warnings"]), "page skips a listed image whose file is missing", failures)
    check(summary["seats"] == 4 and summary["crops"] == 2, "page reads 4 seats and 2 crops from regions.json", failures)
    check(any("stale_example" in w and "matches no loaded image" in w for w in summary["warnings"]),
          "page warns about a region whose where matches no image", failures)

    # Defect 3: the order row comes from the participant number, and 12 numbers fill the 4 rows evenly.
    plans = summary["plans"]
    rows = [0, 0, 0, 0]
    ok_rows = True
    for p in plans.values():
        rows[p["order_row"]] += 1
        ok_rows &= p["order_row"] == (p["number"] - 1) % 4 and p["order"] == summary["latin"][p["order_row"]]
    check(ok_rows and rows == [3, 3, 3, 3], "order row is (number - 1) mod 4 and rows are filled %s" % rows, failures)
    rots = sorted(p["rotation"] for p in plans.values())
    check(rots == sorted([(n - 1) % 3 for n in NUMBERS]), "variant rotation is (number - 1) mod 3", failures)

    first = sum(p["first_v0"] for p in plans.values())
    total = sum(p["paired"] for p in plans.values())
    check(0.35 < first / total < 0.65, "paired order randomised: V0 shown first in %d of %d trials" % (first, total), failures)

    # Defect 2: V0 and every variant are shown at the same contexts (and seats, and crops).
    for test in ("bug", "stand", "find", "team"):
        per_variant = {v: set() for v in VARIANTS}
        once = True
        cycles_ok = True
        for pid in PARTICIPANTS:
            units = summary["units"][pid].get(test, [])
            keys = [u for u, _ in units]
            once &= len(keys) == len(set(keys))
            for u, v in units:
                per_variant[v].add(u)
        # Over each whole cycle of 3 consecutive numbers, every unit is shown once in every variant.
        for start in range(0, len(PARTICIPANTS), len(VARIANTS)):
            seen = {}
            for pid in PARTICIPANTS[start:start + len(VARIANTS)]:
                for u, v in summary["units"][pid].get(test, []):
                    seen.setdefault(u, []).append(v)
            cycles_ok &= all(sorted(vs) == sorted(VARIANTS) for vs in seen.values())
        same = all(per_variant[v] == per_variant["V0"] for v in VARIANTS) and per_variant["V0"]
        check(bool(same) and once and cycles_ok,
              "%s: V0 and each variant share the same %d %s; each participant sees each once; "
              "each 3-person cycle shows each in every variant" % (
                  test, len(per_variant["V0"]), "seat contexts" if test == "find" else "crops" if test == "team" else "contexts"),
              failures)

    check("team" not in summary["bare_blocks"] and any("crops" in n for n in summary["bare_notes"]),
          "no regions file: team block skipped with a note", failures)
    check(any("seat regions" in n for n in summary["bare_notes"]),
          "no regions file: the plan notes that find clicks cannot be scored", failures)

    # Defect 5: whole-number device scale, never shrunk.
    f = summary["fit"]
    check(f["exact"]["scale"] == 1 and f["exact"]["css_w"] == 1280 and f["exact"]["fits"], "1280x720 in 1600x800 at DPR 1 is 1:1", failures)
    check(f["double"]["scale"] == 2 and f["double"]["css_w"] == 1280, "1280x720 in 1600x800 CSS at DPR 2 is 2 device px per pixel", failures)
    check(f["hidpi"]["scale"] == 1 and abs(f["hidpi"]["css_w"] - 1024) < 1e-9, "1280x720 at DPR 1.25 is 1024 CSS px, one device px per pixel", failures)
    check(not f["toobig"]["fits"] and f["toobig"]["scale"] == 1, "1920x1080 in 1570x900 does not fit and is not shrunk", failures)
    css = re.search(r"<style>(.*?)</style>", html, re.S).group(1)
    check("max-width: 100%" not in css and "max-height" not in css and "width: 100%; image" not in css,
          "no CSS rule squeezes a frame to the window", failures)

    # Defect 10: the practice is recorded, so the page must not say otherwise.
    check("not recorded" not in html and "It is not scored." in html, "practice intro says it is not scored, not that it is unrecorded", failures)


def analysis_checks(tmp, results_dir, regions_path, failures):
    # Duplicate exports can omit complete or explicitly set it to null. Both
    # are incomplete, so comparing either with true/false must not raise.
    duplicates = os.path.join(tmp, "duplicates")
    os.makedirs(duplicates)
    for index, complete in enumerate((None, False, True, None)):
        result = {"format": analyse.FORMAT, "participant": "duplicate", "consent": True,
                  "trials": [{}] * (index + 1)}
        if index != 0:
            result["complete"] = complete
        with open(os.path.join(duplicates, "%d.json" % index), "w", encoding="utf-8") as fh:
            json.dump(result, fh)
    people, _, _ = analyse.load_results([duplicates])
    check(people["duplicate"].get("complete") is True and len(people["duplicate"]["trials"]) == 3,
          "duplicate exports normalise missing/null completion and prefer a completed result", failures)

    report = os.path.join(tmp, "report.md")
    rc, ev = run_json([results_dir, "--regions", regions_path, "--report", report,
                       "--json", os.path.join(tmp, "evaluation.json"), "--primary", "V1"])
    check(rc == 0, "analyse.py exits 0", failures)
    v = ev["variants"]
    for rule in ("paired", "bug", "stand", "team", "find", "glitch"):
        check(v["V1"]["rules"][rule]["outcome"] == "PASS", "V1 %s passes: %s" % (rule, v["V1"]["rules"][rule]["reason"]), failures)
        check(v["V1b"]["rules"][rule]["outcome"] == "FAIL", "V1b %s fails: %s" % (rule, v["V1b"]["rules"][rule]["reason"]), failures)
    check(v["V1"]["overall"] == "PASS" and v["V1b"]["overall"] == "FAIL", "overall verdicts V1 PASS, V1b FAIL", failures)
    check(not any("uneven" in w for w in ev["warnings"]), "no counterbalancing warning for 12 consecutive numbers", failures)

    # Defect 7: a region that matches no trial is reported.
    check(any("stale_example" in w and "matches no" in w for w in ev["warnings"]), "analysis warns about a region matching no trial", failures)

    # Defect 8: the glitch pattern is fixed, printed and recorded.
    with open(report, encoding="utf-8") as fh:
        text = fh.read()
    check(analyse.GLITCH_PATTERN in text and ev.get("glitch_pattern") == analyse.GLITCH_PATTERN,
          "report and JSON state the glitch pattern", failures)
    try:
        analyse.main([results_dir, "--report", report, "--glitch-pattern", "x"])
        swapped = True
    except SystemExit:
        swapped = False
    check(not swapped, "--glitch-pattern is no longer accepted", failures)

    # Without a regions file the region rules cannot be scored, and that is not a pass.
    rc, ev2 = run_json([results_dir, "--report", os.path.join(tmp, "report-noregions.md"), "--json", os.path.join(tmp, "ev2.json")])
    check(ev2["variants"]["V1"]["overall"] == "INCOMPLETE" and ev2["variants"]["V1"]["rules"]["bug"]["outcome"] == "NO DATA",
          "no regions file: V1 is INCOMPLETE, not PASS", failures)

    # Reports may not land in the tracked repository.
    rc = analyse.main([results_dir, "--report", os.path.join(HERE, "should-not-exist.md")])
    check(rc == 2 and not os.path.exists(os.path.join(HERE, "should-not-exist.md")), "refuses a report path inside the repository", failures)

    # Defect 4: a report on another drive is not "inside the repository", and does not raise.
    drive = os.path.splitdrive(analyse.REPO_ROOT)[0]
    other = ("Q:" if drive.upper() != "Q:" else "R:") + "\\study-b\\report.md"
    try:
        # Do not query an unrelated volume (which may be locked or removable).
        with mock.patch("_paths.os.path.realpath", side_effect=lambda path: str(path)):
            outside = analyse.inside_repo_unignored(other) is False
    except ValueError:
        outside = False
    check(outside, "a report on another drive (%s) is accepted without a ValueError" % other, failures)

    # Defect 11: a truncated download is skipped with a warning, not a traceback.
    broken = os.path.join(tmp, "results-broken")
    shutil.copytree(results_dir, broken)
    with open(os.path.join(broken, "study-b-truncated.json"), "w", encoding="utf-8") as fh:
        fh.write('{"format": "colourwars-study-b-results", "trials": [')
    with open(os.path.join(broken, "study-b-binary.json"), "wb") as fh:
        fh.write(b"\xff\xfe\x00garbage")
    rc, ev3 = run_json([broken, "--regions", regions_path, "--report", os.path.join(tmp, "r3.md"), "--json", os.path.join(tmp, "ev3.json")])
    check(rc == 0 and sum("could not be read" in w for w in ev3["warnings"]) == 2 and ev3["participants"] == len(PARTICIPANTS),
          "two unreadable results files are skipped with warnings", failures)

    # Defect 3: uneven order rows are reported.
    subset = os.path.join(tmp, "results-subset")
    os.makedirs(subset)
    for n in (1, 5, 9, 2):
        shutil.copy(os.path.join(results_dir, "study-b-T-%02d.json" % n), subset)
    rc, ev4 = run_json([subset, "--regions", regions_path, "--report", os.path.join(tmp, "r4.md"), "--json", os.path.join(tmp, "ev4.json")])
    check(any("Test order rows are uneven: 3/1/0/0" in w for w in ev4["warnings"]), "uneven order rows (participants 1, 5, 9, 2) are reported", failures)


def unit_checks(failures):
    """Rules on hand-made trials, for cases the scripted run does not reach."""
    def person(n, trials):
        return {"participant_number": n, "trials": trials}

    # Defect 1: an object V0 never showed cannot be a new confusion.
    tree = {"id": "tree", "kind": "not_standable", "rect": TREE}
    bush = {"id": "bush", "kind": "not_standable", "where": {"variant": "V1"}, "rect": [0.1, 0.1, 0.2, 0.2]}
    on_bush = [{"x": 0.15, "y": 0.15}]
    ps = {"A": person(1, [{"test": "stand", "variant": "V0", "clicks": []},
                          {"test": "stand", "variant": "V1", "clicks": on_bush}])}
    out, reason, _ = analyse.rule_stand(ps, "V1", [tree, bush])
    check(out == analyse.PASS and "bush" in reason, "stand: an object only in V1 is not compared (%s: %s)" % (out, reason), failures)
    out, reason, _ = analyse.rule_stand(ps, "V1", [bush])
    check(out == analyse.NO_DATA, "stand: with no object shown in V0 the rule is NO DATA (%s)" % reason, failures)
    ps["A"]["trials"].append({"test": "stand", "variant": "V1", "clicks": [{"x": 0.75, "y": 0.45}]})
    out, reason, _ = analyse.rule_stand(ps, "V1", [tree, bush])
    check(out == analyse.FAIL and "tree" in reason, "stand: a V0-shown object clicked only in V1 still fails", failures)

    # Defect 9: quick wrong clicks do not pass the find rule.
    rect = SEAT_RECTS[1]
    right, wrong = {"x": 0.15, "y": 0.25}, {"x": 0.9, "y": 0.9}

    def find(v, ctx, rt, click):
        return {"test": "find", "variant": v, "context": ctx, "seat": 1, "seat_rect": rect, "rt_ms": rt, "click": click}
    ps = {"A": person(1, [find("V0", "c1", 1000, right), find("V0", "c2", 1100, right),
                          find("V1", "c1", 400, wrong), find("V1", "c2", 1000, right)])}
    out, reason, _ = analyse.rule_find(ps, "V1", [])
    check(out == analyse.FAIL and "accuracy" in reason, "find: a fast wrong click fails on accuracy (%s)" % reason, failures)
    ps = {"A": person(1, [find("V0", "c1", 1000, right), find("V0", "c2", 1100, right),
                          find("V1", "c1", 1000, right), find("V1", "c2", 1100, right), find("V1", "c9", 9000, right)])}
    out, reason, d = analyse.rule_find(ps, "V1", [])
    check(out == analyse.PASS and d["seats"][1]["matched_contexts"] == 2,
          "find: a context V0 never showed is left out of the comparison (%s)" % reason, failures)
    ps = {"A": person(1, [dict(find("V0", "c1", 1000, right), seat_rect=None), dict(find("V1", "c1", 900, right), seat_rect=None)])}
    out, reason, _ = analyse.rule_find(ps, "V1", [])
    check(out == analyse.NO_DATA, "find: unscorable clicks give NO DATA, not PASS (%s)" % reason, failures)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keep", help="copy the generated fixture, results and report to this folder (outside the repo)")
    args = ap.parse_args()
    node = shutil.which("node")
    if not node:
        print("Node.js is not on PATH; the page-logic half of this test needs it.", file=sys.stderr)
        return 2
    failures = []
    tmp = tempfile.mkdtemp(prefix="study-b-selftest-")
    try:
        stim, manifest, regions, regions_path = build_fixture(tmp)
        on_disk = [m["path"] for m in manifest["images"] if os.path.exists(os.path.join(stim, *m["path"].split("/")))]
        js, html = extract_page_script(os.path.join(tmp, "page.js"))
        subprocess.run([node, "--check", js], check=True)
        print("ok    node --check on the page script")
        harness = os.path.join(tmp, "harness.js")
        with open(harness, "w", encoding="utf-8") as fh:
            fh.write(HARNESS)
        results_dir = os.path.join(tmp, "results")
        os.makedirs(results_dir)
        consts = {"on_disk": on_disk, "cloud": CLOUD, "brick": BRICK, "tree": TREE}
        people = [[pid, n] for pid, n in zip(PARTICIPANTS, NUMBERS)]
        out = subprocess.run([node, harness, js, os.path.join(stim, "manifest.json"), regions_path, results_dir,
                              json.dumps(people), json.dumps(consts)],
                             check=True, capture_output=True, text=True)
        summary = json.loads(out.stdout)
        check(summary["images"] == len(on_disk), "page loads every image that exists (%d)" % summary["images"], failures)

        page_checks(summary, html, failures)

        lo, hi = analyse.wilson_interval(65, 100)
        check(abs(lo - 0.5525) < 5e-4 and abs(hi - 0.7364) < 5e-4, "Wilson interval for 65/100 is %.4f-%.4f" % (lo, hi), failures)
        check(analyse.percentile_nearest_rank(list(range(1, 11)), 0.9) == 9, "nearest-rank p90 of 1..10 is 9", failures)

        analysis_checks(tmp, results_dir, regions_path, failures)
        unit_checks(failures)

        if args.keep:
            shutil.copytree(tmp, args.keep, dirs_exist_ok=True)
            print("Fixture kept in %s" % args.keep)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print()
    print("selftest: %s" % ("PASS" if not failures else "FAIL (%d)" % len(failures)))
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
