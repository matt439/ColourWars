#!/usr/bin/env python3
"""Analyse ColourWars Study B results against the pre-registered pass rules.

Standard library only. Reads one or more results files exported by index.html,
plus an optional regions file that says where each authored mistake, each
non-standable object and each seat is. Prints a per-variant PASS/FAIL table and
writes a Markdown report.

The pass rules are the ones fixed in docs/drawing-mistakes-research.md,
section 8, Study B. They are constants here on purpose: change the document
first, before any participant is run, never after.

    python analyse.py RESULTS.json [RESULTS.json ...] --report REPORT.md
                      [--regions REGIONS.json] [--primary V1] [--json EVAL.json]

The file formats are in scripts/studies/README.md.
"""

import argparse
import glob
import json
import math
import os
import re
import statistics
import sys

BASELINE = "V0"
FORMAT = "colourwars-study-b-results"

# Pre-registered pass rules (section 8, Study B). Do not tune after data exists.
PAIRED_MIN_RATE = 0.65          # "drawn by a person" chosen over V0 in >= 65% of trials
PAIRED_MIN_LOWER = 0.50         # Wilson 95% lower bound must be above 50%
WILSON_Z = 1.959963984540054    # two-sided 95%
BACKDROP_MAX_SHARE = 0.10       # a backdrop mistake fails if > 10% of participants click it
GLITCH_EXTRA_PARTICIPANTS = 1   # glitch comments <= V0's rate plus one participant

# The glitch word list is a pass rule too, so it is a constant here and not an
# option: it cannot be swapped once results exist. The report prints it.
GLITCH_PATTERN = (r"\b(glitch\w*|jitter\w*|bug\w*|broken|error\w*|artifact\w*|artefact\w*|"
                  r"corrupt\w*|flicker\w*|misalign\w*|render\w*)")

REGION_KINDS = ("mistake", "not_standable", "seat", "crop")
REGION_TEST = {"mistake": "bug", "not_standable": "stand", "seat": "find", "crop": "team"}
VARIANT_KEYS = ("variant", "variants", "path")

PASS, FAIL, NO_DATA = "PASS", "FAIL", "NO DATA"

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


# ------------------------------------------------------------------ statistics

def wilson_interval(k, n, z=WILSON_Z):
    """Wilson score interval for k successes in n trials."""
    if n == 0:
        return (0.0, 1.0)
    p = k / n
    denom = 1 + z * z / n
    centre = (p + z * z / (2 * n)) / denom
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    return (max(0.0, centre - half), min(1.0, centre + half))


def percentile_nearest_rank(values, q):
    """Nearest-rank percentile: the smallest value with at least q of the data at or below it."""
    if not values:
        return None
    s = sorted(values)
    rank = max(1, math.ceil(q * len(s)))
    return s[rank - 1]


def pct(x):
    return "-" if x is None else "%.1f%%" % (100 * x)


def ms(x):
    return "-" if x is None else ("%d ms" % x if x == int(x) else "%.1f ms" % x)


# ------------------------------------------------------------------ input

def load_results(paths):
    files = []
    for p in paths:
        if os.path.isdir(p):
            files.extend(sorted(glob.glob(os.path.join(p, "*.json"))))
        else:
            matched = sorted(glob.glob(p))
            files.extend(matched if matched else [p])
    participants = {}
    warnings = []
    for f in files:
        try:
            with open(f, encoding="utf-8") as fh:
                data = json.load(fh)
        except (OSError, UnicodeDecodeError, json.JSONDecodeError) as e:
            warnings.append("%s: could not be read as JSON (%s); skipped." % (os.path.basename(f), e))
            continue
        if not isinstance(data, dict) or data.get("format") != FORMAT:
            warnings.append("%s: not a Study B results file; skipped." % os.path.basename(f))
            continue
        if not data.get("consent"):
            warnings.append("%s: no consent recorded; excluded." % os.path.basename(f))
            continue
        pid = data.get("participant")
        if pid in participants:
            old = participants[pid]
            # Keep the complete file, or the one with more trials.
            if (data.get("complete"), len(data.get("trials", []))) > (old.get("complete"), len(old.get("trials", []))):
                participants[pid] = data
            warnings.append("Participant %s appears in more than one file; kept the most complete." % pid)
            continue
        if not data.get("complete"):
            warnings.append("Participant %s did not finish; partial trials are used." % pid)
        participants[pid] = data
    return participants, warnings, files


def load_regions(path):
    if not path:
        return []
    with open(path, encoding="utf-8") as fh:
        data = json.load(fh)
    out = []
    for i, r in enumerate(data.get("regions", [])):
        rect = r.get("rect")
        kind = r.get("kind")
        if kind not in REGION_KINDS:
            raise SystemExit("regions[%d]: kind must be mistake, not_standable, seat or crop" % i)
        if not (isinstance(rect, list) and len(rect) == 4 and rect[2] > rect[0] and rect[3] > rect[1]):
            raise SystemExit("regions[%d]: rect must be [x0, y0, x1, y1] in 0..1" % i)
        if kind == "mistake" and r.get("layer") not in ("gameplay", "backdrop"):
            raise SystemExit("regions[%d]: a mistake needs layer gameplay or backdrop" % i)
        if kind == "seat" and not isinstance(r.get("seat"), int):
            raise SystemExit("regions[%d]: a seat region needs an integer seat" % i)
        if "id" not in r and kind != "seat":
            raise SystemExit("regions[%d]: needs an id" % i)
        if kind in ("seat", "crop") and any(k in r.get("where", {}) for k in VARIANT_KEYS):
            raise SystemExit("regions[%d]: a %s must hold in every variant; its where may not use "
                             "variant, variants or path" % (i, kind))
        out.append(r)
    return out


def region_applies(region, trial):
    where = region.get("where", {})
    for key, want in where.items():
        if key == "variants":
            if trial.get("variant") not in want:
                return False
        elif key == "path":
            if trial.get("path") != want.replace("\\", "/"):
                return False
        elif trial.get(key) != want:
            return False
    return True


def inside(point, rect):
    return point is not None and rect[0] <= point["x"] <= rect[2] and rect[1] <= point["y"] <= rect[3]


def context_of(t):
    """The same key as contextKey in index.html."""
    if t.get("context") is not None:
        return t["context"]
    parts = []
    for k in ("resolution", "players", "palette", "shot", "identity_cues"):
        v = t.get(k)
        parts.append("" if v is None else ("true" if v is True else "false" if v is False else str(v)))
    return "|".join(parts)


def trials_of(participants, test):
    for pid in sorted(participants):
        for t in participants[pid].get("trials", []):
            if t.get("test") == test:
                yield pid, t


# ------------------------------------------------------------------ rules

def rule_paired(participants, variant):
    k = n = 0
    per = {}
    for pid, t in trials_of(participants, "paired"):
        if t.get("variant") != variant:
            continue
        n += 1
        k += 1 if t.get("chose_variant") else 0
        a = per.setdefault(pid, [0, 0])
        a[0] += 1 if t.get("chose_variant") else 0
        a[1] += 1
    if n == 0:
        return NO_DATA, "no paired trials", {}
    rate = k / n
    lo, hi = wilson_interval(k, n)
    maj_v = sum(1 for a in per.values() if 2 * a[0] > a[1])
    maj_0 = sum(1 for a in per.values() if 2 * a[0] < a[1])
    ties = len(per) - maj_v - maj_0
    detail = {"k": k, "n": n, "rate": rate, "lower": lo, "upper": hi,
              "maj_variant": maj_v, "maj_v0": maj_0, "ties": ties, "participants": len(per)}
    ok = rate >= PAIRED_MIN_RATE and lo > PAIRED_MIN_LOWER
    reason = "%d/%d = %s, 95%% CI %s-%s; participants majority %d for, %d against, %d tied" % (
        k, n, pct(rate), pct(lo), pct(hi), maj_v, maj_0, ties)
    if not ok:
        why = []
        if rate < PAIRED_MIN_RATE:
            why.append("rate below 65%")
        if lo <= PAIRED_MIN_LOWER:
            why.append("lower bound not above 50%")
        reason += " (" + ", ".join(why) + ")"
    return (PASS if ok else FAIL), reason, detail


def rule_bug(participants, variant, regions):
    mistakes = [r for r in regions if r["kind"] == "mistake"]
    trials = [(pid, t) for pid, t in trials_of(participants, "bug") if t.get("variant") == variant]
    if not trials:
        return NO_DATA, "no find-the-bug trials", {}
    seen = {}      # region id -> set of participants who saw it
    clicked = {}   # region id -> set of participants who clicked it
    layer = {}
    for pid, t in trials:
        for r in mistakes:
            if not region_applies(r, t):
                continue
            seen.setdefault(r["id"], set()).add(pid)
            layer[r["id"]] = r["layer"]
            if not t.get("no_bug") and inside(t.get("click"), r["rect"]):
                clicked.setdefault(r["id"], set()).add(pid)
    no_bug = sum(1 for _, t in trials if t.get("no_bug"))
    detail = {"trials": len(trials), "no_bug": no_bug, "regions": {}}
    for rid in sorted(seen):
        detail["regions"][rid] = {"layer": layer[rid], "seen": len(seen[rid]), "clicked": len(clicked.get(rid, ()))}
    if not seen:
        return NO_DATA, "no mistake regions apply to this variant's bug trials", detail
    fails = []
    for rid, d in sorted(detail["regions"].items()):
        if d["layer"] == "gameplay" and d["clicked"] > 0:
            fails.append("gameplay mistake %s clicked by %d participant(s)" % (rid, d["clicked"]))
        if d["layer"] == "backdrop" and d["clicked"] / d["seen"] > BACKDROP_MAX_SHARE:
            fails.append("backdrop mistake %s clicked by %d/%d participants (%s > 10%%)" % (
                rid, d["clicked"], d["seen"], pct(d["clicked"] / d["seen"])))
    if fails:
        return FAIL, "; ".join(fails), detail
    return PASS, "%d mistake region(s) checked over %d trials; %d said no bug" % (len(seen), len(trials), no_bug), detail


def standable_confusions(participants, variant, regions):
    decoys = [r for r in regions if r["kind"] == "not_standable"]
    seen, clicked = {}, {}
    n = 0
    for pid, t in trials_of(participants, "stand"):
        if t.get("variant") != variant:
            continue
        n += 1
        for r in decoys:
            if not region_applies(r, t):
                continue
            seen.setdefault(r["id"], set()).add(pid)
            if any(inside(c, r["rect"]) for c in t.get("clicks", [])):
                clicked.setdefault(r["id"], set()).add(pid)
    return n, seen, clicked


def rule_stand(participants, variant, regions):
    n, seen, clicked = standable_confusions(participants, variant, regions)
    if n == 0:
        return NO_DATA, "no standable trials", {}
    if not seen:
        return NO_DATA, "no not_standable regions apply to this variant", {}
    n0, seen0, clicked0 = standable_confusions(participants, BASELINE, regions)
    detail = {}
    new, uncompared = [], []
    for rid in sorted(seen):
        c = len(clicked.get(rid, ()))
        c0 = len(clicked0.get(rid, ()))
        s0 = len(seen0.get(rid, ()))
        detail[rid] = {"seen": len(seen[rid]), "clicked": c, "v0_seen": s0, "v0_clicked": c0}
        if s0 == 0:
            # V0 never showed this object, so there is nothing to call new against.
            uncompared.append(rid)
            continue
        if c > 0 and c0 == 0:
            new.append("%s clicked by %d/%d (V0: 0/%d)" % (rid, c, len(seen[rid]), s0))
    compared = len(seen) - len(uncompared)
    note = ("; not shown in V0, so not compared: " + ", ".join(uncompared)) if uncompared else ""
    if new:
        return FAIL, "new confusion: " + "; ".join(new) + note, detail
    if compared == 0:
        return NO_DATA, "no object shown in this variant was also shown in V0" + note, detail
    return PASS, ("%d object(s) compared; no object confused that V0 did not already confuse%s"
                  % (compared, note)), detail


def crop_unit(t):
    """A crop in one context: the same figure, whatever the variant."""
    cid = t.get("crop_id")
    if cid is None:
        cid = ",".join("%.4f" % v for v in (t.get("crop") or []))
    return "%s@%s" % (cid, context_of(t))


def team_units(participants, variant):
    return {crop_unit(t) for _, t in trials_of(participants, "team") if t.get("variant") == variant}


def team_cells(participants, variant, units=None):
    cells = {}
    for _, t in trials_of(participants, "team"):
        if t.get("variant") != variant or (units is not None and crop_unit(t) not in units):
            continue
        for key in (("palette", t.get("palette")), ("cell", "%s/%s" % (t.get("palette"), t.get("truth")))):
            a = cells.setdefault(key, [0, 0])
            a[0] += 1 if t.get("correct") else 0
            a[1] += 1
    return cells


def rule_team(participants, variant):
    mine_units = team_units(participants, variant)
    if not mine_units:
        return NO_DATA, "no team-ID trials", {}
    # Compare only crops shown in both this variant and V0, so both sides judge the same figures.
    matched = mine_units & team_units(participants, BASELINE)
    if not matched:
        return NO_DATA, "no crop was shown in both this variant and V0", {}
    cells = team_cells(participants, variant, matched)
    base = team_cells(participants, BASELINE, matched)
    drops, missing = [], []
    for key in sorted(cells):
        k, n = cells[key]
        if key not in base:
            missing.append(key[1])
            continue
        k0, n0 = base[key]
        if k / n < k0 / n0:
            drops.append("%s %s (%d/%d) below V0 %s (%d/%d)" % (key[1], pct(k / n), k, n, pct(k0 / n0), k0, n0))
    detail = {"cells": {"%s:%s" % k: v for k, v in cells.items()},
              "v0": {"%s:%s" % k: v for k, v in base.items()},
              "matched_crops": len(matched), "unmatched_crops": len(mine_units - matched)}
    if drops:
        return FAIL, "drop in " + "; ".join(drops), detail
    if missing:
        return NO_DATA, "no V0 trials for " + ", ".join(missing), detail
    trials = sum(v[1] for k, v in cells.items() if k[0] == "palette")
    return PASS, ("no drop in any palette or team cell (%d trials over %d crop(s) shown in both; "
                  "~200+ per palette needed to see a 2-point drop)" % (trials, len(matched))), detail


def seat_rect_for(t, regions):
    rect = t.get("seat_rect")
    if rect is None:
        for r in regions:
            if r["kind"] == "seat" and r["seat"] == t.get("seat") and region_applies(r, t):
                return r["rect"]
    return rect


def find_rows(participants, variant, regions):
    rows = []
    for _, t in trials_of(participants, "find"):
        if t.get("variant") != variant or t.get("rt_ms") is None:
            continue
        rect = seat_rect_for(t, regions)
        rows.append({"seat": t.get("seat"), "context": context_of(t), "rt": t["rt_ms"], "rect": rect,
                     "correct": inside(t.get("click"), rect) if rect is not None else None})
    return rows


def find_stats(rows):
    """Median and p90 over correct clicks only, when every click can be scored."""
    scored = [r for r in rows if r["rect"] is not None]
    d = {"trials": len(rows), "scored": len(scored), "correct": sum(1 for r in scored if r["correct"]),
         "contexts": len({r["context"] for r in rows})}
    d["all_scored"] = len(scored) == len(rows)
    d["accuracy"] = d["correct"] / len(scored) if scored else None
    times = [r["rt"] for r in scored if r["correct"]] if d["all_scored"] else [r["rt"] for r in rows]
    d["times"] = times
    d["median"] = statistics.median(times) if times else None
    d["p90"] = percentile_nearest_rank(times, 0.9)
    return d


def find_matched(participants, variant, regions):
    """Per seat, the variant's and V0's stats over the contexts both were shown at."""
    mine = find_rows(participants, variant, regions)
    base = find_rows(participants, BASELINE, regions)
    out = {}
    for seat in sorted({r["seat"] for r in mine}, key=str):
        m = [r for r in mine if r["seat"] == seat]
        b = [r for r in base if r["seat"] == seat]
        shared = {r["context"] for r in m} & {r["context"] for r in b}
        out[seat] = {"variant": find_stats([r for r in m if r["context"] in shared]),
                     "v0": find_stats([r for r in b if r["context"] in shared]),
                     "matched_contexts": len(shared),
                     "unmatched_trials": sum(1 for r in m if r["context"] not in shared)}
    return out


def rule_find(participants, variant, regions):
    per = find_matched(participants, variant, regions)
    if not per:
        return NO_DATA, "no find-your-player trials", {}
    worse, missing, unscored = [], [], []
    for seat, x in per.items():
        if not x["matched_contexts"]:
            missing.append(str(seat))
            continue
        d, b = x["variant"], x["v0"]
        if not (d["all_scored"] and b["all_scored"]):
            unscored.append(str(seat))
            continue
        if d["accuracy"] < b["accuracy"]:
            worse.append("seat %s accuracy %d/%d below V0 %d/%d" % (seat, d["correct"], d["scored"], b["correct"], b["scored"]))
        if d["median"] is not None and b["median"] is not None:
            if d["median"] > b["median"]:
                worse.append("seat %s median %s > V0 %s" % (seat, ms(d["median"]), ms(b["median"])))
            if d["p90"] > b["p90"]:
                worse.append("seat %s p90 %s > V0 %s" % (seat, ms(d["p90"]), ms(b["p90"])))
        elif b["median"] is None and d["median"] is not None:
            missing.append("%s (no correct V0 click to time)" % seat)
    detail = {"seats": per}
    if worse:
        return FAIL, "; ".join(worse), detail
    if missing:
        return NO_DATA, "no V0 trials at the same contexts for seat(s) " + ", ".join(missing), detail
    if unscored:
        return NO_DATA, ("seat(s) %s have trials without a seat rectangle, so wrong clicks cannot be "
                         "told from right ones; add seat regions" % ", ".join(unscored)), detail
    return PASS, ("accuracy, median and p90 of correct clicks no worse than V0 at %d seat(s), "
                  "over matched contexts" % len(per)), detail


def glitch_comments(participants, variant, pattern):
    shown, flagged, quotes = set(), set(), []
    for pid, t in trials_of(participants, "impressions"):
        if t.get("variant") != variant:
            continue
        shown.add(pid)
        text = t.get("text", "")
        if pattern.search(text):
            flagged.add(pid)
            quotes.append((pid, t.get("path"), text))
    return shown, flagged, quotes


def rule_glitch(participants, variant, pattern):
    shown, flagged, quotes = glitch_comments(participants, variant, pattern)
    if not shown:
        return NO_DATA, "no first-look comments", {}
    shown0, flagged0, _ = glitch_comments(participants, BASELINE, pattern)
    if not shown0:
        return NO_DATA, "no V0 first-look comments", {"quotes": quotes}
    rate, rate0 = len(flagged) / len(shown), len(flagged0) / len(shown0)
    limit = rate0 + GLITCH_EXTRA_PARTICIPANTS / len(shown)
    detail = {"quotes": quotes, "flagged": len(flagged), "shown": len(shown), "v0_flagged": len(flagged0), "v0_shown": len(shown0)}
    reason = "%d/%d participants (V0 %d/%d; limit %s)" % (len(flagged), len(shown), len(flagged0), len(shown0), pct(limit))
    if rate > limit + 1e-12:
        return FAIL, reason, detail
    if quotes:
        reason += "; each flagged comment still needs a traced cause"
    return PASS, reason, detail


RULES = [
    ("paired", "Drawn by a person"),
    ("bug", "Find the bug"),
    ("stand", "Standable confusions"),
    ("team", "Team ID"),
    ("find", "Find time"),
    ("glitch", "Glitch comments"),
]


def evaluate(participants, regions, pattern):
    variants = set()
    for p in participants.values():
        for t in p.get("trials", []):
            if t.get("variant"):
                variants.add(t["variant"])
    variants.discard(BASELINE)
    out = {}
    for v in sorted(variants):
        res = {
            "paired": rule_paired(participants, v),
            "bug": rule_bug(participants, v, regions),
            "stand": rule_stand(participants, v, regions),
            "team": rule_team(participants, v),
            "find": rule_find(participants, v, regions),
            "glitch": rule_glitch(participants, v, pattern),
        }
        outcomes = [r[0] for r in res.values()]
        if FAIL in outcomes:
            overall = FAIL
        elif NO_DATA in outcomes:
            overall = "INCOMPLETE"
        else:
            overall = PASS
        out[v] = {"overall": overall, "rules": res}
    return out


# ------------------------------------------------------------------ output

def design_warnings(participants, regions):
    """Checks on the run itself: counterbalancing, regions that match nothing, frames that did not fit."""
    out = []
    numbers = {}
    rows, rotations = {}, {}
    for pid, p in sorted(participants.items()):
        n = p.get("participant_number")
        if n is None:
            out.append("Participant %s has no participant number (an older results file?); "
                       "its test order and variant rotation cannot be checked." % pid)
        else:
            numbers.setdefault(n, []).append(pid)
        if p.get("order_row") is not None:
            rows[p["order_row"]] = rows.get(p["order_row"], 0) + 1
        if p.get("rotation") is not None:
            rotations[p["rotation"]] = rotations.get(p["rotation"], 0) + 1
    for n, pids in sorted(numbers.items()):
        if len(pids) > 1:
            out.append("Participant number %s was used by %s; each person needs their own number." % (n, ", ".join(pids)))
    counts = [rows.get(r, 0) for r in range(4)]
    if rows and max(counts) - min(counts) > 1:
        out.append("Test order rows are uneven: %s participants in rows 1-4. Number participants 1, 2, 3, ... "
                   "without gaps so each row is used equally." % "/".join(str(c) for c in counts))
    variants = set()
    for p in participants.values():
        variants.update(p.get("variants") or [])
    if rotations and variants:
        rc = [rotations.get(r, 0) for r in range(len(variants))]
        if max(rc) - min(rc) > 1:
            out.append("Variant rotations are uneven: %s participants per rotation. Every frame is shown in every "
                       "variant equally only over whole cycles of %d participants." % ("/".join(str(c) for c in rc), len(variants)))
    for r in regions:
        test = REGION_TEST[r["kind"]]
        trials = [t for _, t in trials_of(participants, test)]
        if trials and not any(region_applies(r, t) for t in trials):
            out.append("Region %s (%s): its where %s matches no %s trial, so it scored nothing. "
                       "Check its keys against the manifest's path and shot names." % (
                           r.get("id", "seat %s" % r.get("seat")), r["kind"], json.dumps(r.get("where", {})), test))
    unfit = sum(1 for p in participants.values() for t in p.get("trials", [])
                if isinstance(t.get("display"), dict) and t["display"].get("fits") is False)
    if unfit:
        out.append("%d trial(s) showed a frame larger than the window (fits: false). Those frames were not "
                   "shrunk, but part of them may have been off screen." % unfit)
    return out


def baseline_summary(participants, regions):
    lines = []
    trials = [(pid, t) for pid, t in trials_of(participants, "bug") if t.get("variant") == BASELINE]
    if trials:
        clicks = sum(1 for _, t in trials if not t.get("no_bug"))
        lines.append("- Find the bug on V0: %d of %d trials had a click (false alarms, since V0 is the reference)." % (clicks, len(trials)))
    n0, seen0, clicked0 = standable_confusions(participants, BASELINE, regions)
    if seen0:
        conf = ", ".join("%s %d/%d" % (rid, len(clicked0.get(rid, ())), len(seen0[rid])) for rid in sorted(seen0))
        lines.append("- Standable confusions on V0: " + conf + ".")
    rows = find_rows(participants, BASELINE, regions)
    for seat in sorted({r["seat"] for r in rows}, key=str):
        d = find_stats([r for r in rows if r["seat"] == seat])
        acc = ", %d/%d on the right player" % (d["correct"], d["scored"]) if d["scored"] else ", clicks not scored"
        lines.append("- Find time on V0, seat %s: median %s, p90 %s over %d timed trials of %d%s." % (
            seat, ms(d["median"]), ms(d["p90"]), len(d["times"]), d["trials"], acc))
    return lines


def write_report(path, participants, warnings, files, regions_path, regions, primary, results, pattern):
    L = []
    L.append("# Study B results")
    L.append("")
    L.append("Generated by `scripts/studies/study-b/analyse.py`. The pass rules are those fixed in "
             "docs/drawing-mistakes-research.md, section 8, before the study ran.")
    L.append("")
    L.append("- Results files: %d; participants with consent: %d." % (len(files), len(participants)))
    L.append("- Regions file: %s." % (os.path.basename(regions_path) if regions_path else "none (bug, standable and seat rules cannot be scored)"))
    L.append("- Primary comparison, chosen in advance: **%s** against V0." % primary)
    L.append("- Glitch word pattern (case-insensitive), fixed in the script: `%s`" % pattern.pattern)
    L.append("")
    L.append("| Variant | Overall | " + " | ".join(name for _, name in RULES) + " |")
    L.append("|---|---|" + "---|" * len(RULES))
    for v, r in results.items():
        mark = " (primary)" if v == primary else ""
        L.append("| %s%s | **%s** | %s |" % (v, mark, r["overall"], " | ".join(r["rules"][k][0] for k, _ in RULES)))
    L.append("")
    for v, r in results.items():
        L.append("## %s" % v)
        L.append("")
        for k, name in RULES:
            outcome, reason, _ = r["rules"][k]
            L.append("- **%s: %s.** %s." % (name, outcome, reason))
        d = r["rules"]["bug"][2].get("regions", {})
        if d:
            L.append("")
            L.append("| Mistake region | Layer | Participants who saw it | Clicked it |")
            L.append("|---|---|---|---|")
            for rid, x in d.items():
                L.append("| %s | %s | %d | %d |" % (rid, x["layer"], x["seen"], x["clicked"]))
        fd = r["rules"]["find"][2].get("seats", {})
        if fd:
            L.append("")
            L.append("Find time over the contexts both this variant and V0 were shown at; times are of correct clicks only.")
            L.append("")
            L.append("| Seat | Matched contexts | Accuracy | Median | p90 | V0 accuracy | V0 median | V0 p90 |")
            L.append("|---|---|---|---|---|---|---|---|")
            for seat, x in fd.items():
                a, b = x["variant"], x["v0"]

                def acc(d):
                    return "%d/%d" % (d["correct"], d["scored"]) if d["all_scored"] and d["scored"] else "not scored"
                L.append("| %s | %d | %s | %s | %s | %s | %s | %s |" % (
                    seat, x["matched_contexts"], acc(a), ms(a["median"]), ms(a["p90"]), acc(b), ms(b["median"]), ms(b["p90"])))
        quotes = r["rules"]["glitch"][2].get("quotes", [])
        if quotes:
            L.append("")
            L.append("First-look comments matching the glitch words, each to be traced to a cause:")
            L.append("")
            for pid, p, text in quotes:
                L.append("- %s on `%s`: \"%s\"" % (pid, p, text.replace("\n", " ")))
        L.append("")
    _, _, v0quotes = glitch_comments(participants, BASELINE, pattern)
    L.append("## V0 baseline")
    L.append("")
    L.extend(baseline_summary(participants, regions) or ["- No baseline trials beyond the comparisons above."])
    if v0quotes:
        L.append("- V0 first-look comments matching the glitch words: %d." % len(v0quotes))
    L.append("")
    L.append("## Reading the result")
    L.append("")
    L.append("- A variant passes only if every rule passes. INCOMPLETE means a rule had no data, which is not a pass.")
    L.append("- Paired-choice trials are not independent; read a borderline result per participant (the majority counts above).")
    L.append("- If a heavier variant only squeaks past, ship the next lighter one.")
    L.append("- Team ID needs about 200+ trials per palette to detect a 2-point drop; the counts above say how far short this run is.")
    if warnings:
        L.append("")
        L.append("## Input and design warnings")
        L.append("")
        for w in warnings:
            L.append("- " + w)
    L.append("")
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(L))


def inside_repo_unignored(path):
    p = os.path.normcase(os.path.abspath(path))
    root = os.path.normcase(REPO_ROOT)
    try:
        common = os.path.commonpath([p, root])
    except ValueError:
        # Another drive (D:, a USB stick) is never inside the repository.
        return False
    if common != root:
        return False
    rel = os.path.relpath(p, root).replace("\\", "/")
    return not rel.startswith("out/")


def main(argv=None):
    ap = argparse.ArgumentParser(description="Apply the Study B pass rules to exported results.")
    ap.add_argument("results", nargs="+", help="results JSON files, globs or folders of them")
    ap.add_argument("--regions", help="regions JSON (mistakes, not_standable objects, seats, crops)")
    ap.add_argument("--report", required=True, help="Markdown report to write (outside the repository, or under out/)")
    ap.add_argument("--primary", default="V1", help="the primary comparison chosen in advance (default V1)")
    ap.add_argument("--json", help="also write the evaluation as JSON to this path")
    args = ap.parse_args(argv)

    for target in [args.report] + ([args.json] if args.json else []):
        if inside_repo_unignored(target):
            print("error: %s is inside the repository. Participant results stay out of it; "
                  "write under out/ or outside the repository." % target, file=sys.stderr)
            return 2

    participants, warnings, files = load_results(args.results)
    if not participants:
        print("error: no usable results files.", file=sys.stderr)
        for w in warnings:
            print("  " + w, file=sys.stderr)
        return 2
    regions = load_regions(args.regions)
    warnings = warnings + design_warnings(participants, regions)
    pattern = re.compile(GLITCH_PATTERN, re.IGNORECASE)
    results = evaluate(participants, regions, pattern)

    print("Study B: %d participant(s), primary comparison %s" % (len(participants), args.primary))
    width = max([7] + [len(v) for v in results])
    header = "%-*s  %-10s  %s" % (width, "Variant", "Overall", "  ".join("%-9s" % k for k, _ in RULES))
    print(header)
    print("-" * len(header))
    for v, r in results.items():
        print("%-*s  %-10s  %s" % (width, v, r["overall"], "  ".join("%-9s" % r["rules"][k][0] for k, _ in RULES)))
    print()
    for v, r in results.items():
        for k, name in RULES:
            outcome, reason, _ = r["rules"][k]
            if outcome != PASS:
                print("%s %s: %s - %s" % (v, name, outcome, reason))
    if args.primary not in results:
        print("warning: the primary comparison %s has no data." % args.primary)
    for w in warnings:
        print("warning: " + w)

    os.makedirs(os.path.dirname(os.path.abspath(args.report)), exist_ok=True)
    write_report(args.report, participants, warnings, files, args.regions, regions, args.primary, results, pattern)
    print("Report written to %s" % args.report)
    if args.json:
        out = {v: {"overall": r["overall"], "rules": {k: {"outcome": x[0], "reason": x[1]} for k, x in r["rules"].items()}}
               for v, r in results.items()}
        os.makedirs(os.path.dirname(os.path.abspath(args.json)), exist_ok=True)
        with open(args.json, "w", encoding="utf-8", newline="\n") as fh:
            json.dump({"primary": args.primary, "participants": len(participants), "glitch_pattern": GLITCH_PATTERN,
                       "warnings": warnings, "variants": out}, fh, indent=2)
    return 0


if __name__ == "__main__":
    sys.exit(main())
