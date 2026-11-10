"""Summarize balance-runner CSVs as evidence: two measures per tool, with intervals.

Reads the `matches.csv` files that `scripts/run-balance.ps1` writes (schema 2 or
later) and reports, for each tool, its share of painted area, eliminations a
minute and deaths a minute, each with a 95% interval, and checks them against
the tool profiles the author set in `profiles.json`
(docs/gameplay-tuning.md#weapon-profiles-2026-11-10). It proposes no tuning.

Matches are grouped into independent *units* inside *strata*:

- 1v1: a stratum is one stage and one unordered tool pair {X, Y}; a unit is one
  seed's side-swap pair, the match with X on team A and the match with X on
  team B. It gives the cell (stage, X, Y) X's mean over the two, and the cell
  (stage, Y, X) Y's. A mirror's unit is its one match, both seats averaged.
- Any other roster: a stratum is one stage and one roster; a unit is one match,
  giving each tool in it the mean over its seats.

Units in a stratum are taken as independent and alike, so every estimate here
is a fixed weighted sum of cell means (a tool's marginal, the gap between two
tools, a tool against the field). Its variance comes from the spread over units
of each stratum's weighted contribution. Values two tools draw from the same
match, such as complementary paint shares, therefore count with their
covariance. Standard library only.
"""

import argparse
import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path
from statistics import NormalDist, median
import sys


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PROFILES = Path(__file__).resolve().with_name("profiles.json")
MEASURES = ("paint", "kills", "deaths")
MEASURE_LABELS = {
    "paint": "share of painted",
    "kills": "eliminations / min",
    "deaths": "deaths / min",
}
LEVELS = ("most", "high", "mid", "low", "least")
MATCH_FIELDS = ("schema_version", "stage", "seed", "players", "bot", "requested_active_seconds",
                "simulated_active_seconds")
SEAT_FIELDS = ("team", "tool", "share_of_painted", "credited_enemy_eliminations",
               "credited_enemy_projectile_deaths")
CONFIDENCE = 0.95
POWER = 0.80
Z_POWER = NormalDist().inv_cdf(POWER)
SPREAD_TOLERANCE = 1e-9


# --- Student's t, from the regularized incomplete beta (Numerical Recipes 6.4) ---

def _beta_fraction(a, b, x):
    tiny = 1e-300
    qab, qap, qam = a + b, a + 1.0, a - 1.0
    c, d = 1.0, 1.0 - qab * x / qap
    d = 1.0 / (d if abs(d) > tiny else tiny)
    h = d
    for m in range(1, 300):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        delta = d * c
        h *= delta
        if abs(delta - 1.0) < 1e-15:
            break
    return h


def _incomplete_beta(a, b, x):
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    front = math.exp(math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b)
                     + a * math.log(x) + b * math.log(1.0 - x))
    if x < (a + 1.0) / (a + b + 2.0):
        return front * _beta_fraction(a, b, x) / a
    return 1.0 - front * _beta_fraction(b, a, 1.0 - x) / b


def t_cdf(t, df):
    tail = 0.5 * _incomplete_beta(df / 2.0, 0.5, df / (df + t * t))
    return 1.0 - tail if t >= 0 else tail


def t_quantile(p, df):
    """Quantile of Student's t with df (possibly fractional) degrees of freedom."""
    if df <= 0:
        raise ValueError("t needs positive degrees of freedom")
    if df > 1e6:
        return NormalDist().inv_cdf(p)
    lo, hi = -1e3, 1e3
    for _ in range(200):
        mid = (lo + hi) / 2.0
        if t_cdf(mid, df) < p:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2.0


def two_sided(df):
    return t_quantile(1.0 - (1.0 - CONFIDENCE) / 2.0, df)


# --- Input ---

def find_csvs(inputs):
    files = []
    for item in inputs:
        path = Path(item)
        if path.is_dir():
            files.extend(p for p in path.rglob("matches.csv") if p.is_file())
        elif path.is_file():
            files.append(path)
        else:
            raise ValueError(f"No such file or directory: {item}")
    if not files:
        raise ValueError("No matches.csv found in the inputs")
    return sorted(set(p.resolve() for p in files))


def number(row, field, path, line):
    try:
        value = float(row[field])
    except (KeyError, TypeError, ValueError):
        raise ValueError(f"{path}:{line}: '{field}' is missing or not a number") from None
    if not math.isfinite(value):
        raise ValueError(f"{path}:{line}: '{field}' is not finite")
    return value


def read_matches(files):
    """Return one dict per match, with its active seats, refusing bad, empty or repeated input."""
    matches, seen, sources = [], {}, []
    for path in files:
        raw = path.read_bytes()
        sources.append({"path": str(path), "sha256": hashlib.sha256(raw).hexdigest().upper(), "rows": 0})
        reader = csv.DictReader(io.StringIO(raw.decode("utf-8-sig"), newline=""))
        missing = [f for f in MATCH_FIELDS if f not in (reader.fieldnames or [])]
        if missing:
            raise ValueError(f"{path}: missing columns {', '.join(missing)}")
        for line, row in enumerate(reader, start=2):
            schema = int(number(row, "schema_version", path, line))
            if schema < 2:
                raise ValueError(f"{path}:{line}: schema {schema} has no elimination counts; need 2 or later")
            players = int(number(row, "players", path, line))
            if not 1 <= players <= 4:
                raise ValueError(f"{path}:{line}: players must be 1..4")
            minutes = number(row, "simulated_active_seconds", path, line) / 60.0
            if minutes <= 0:
                raise ValueError(f"{path}:{line}: simulated_active_seconds must be positive")
            seats = []
            for seat in range(1, players + 1):
                prefix = f"p{seat}_"
                for field in SEAT_FIELDS:
                    if prefix + field not in row:
                        raise ValueError(f"{path}:{line}: missing column {prefix + field}")
                tool, team = row[prefix + "tool"], row[prefix + "team"]
                if tool in ("", "none") or team not in ("a", "b"):
                    raise ValueError(f"{path}:{line}: seat {seat} has no tool or team")
                seats.append({
                    "seat": seat, "team": team, "tool": tool,
                    "paint": number(row, prefix + "share_of_painted", path, line),
                    "kills": number(row, prefix + "credited_enemy_eliminations", path, line) / minutes,
                    "deaths": number(row, prefix + "credited_enemy_projectile_deaths", path, line) / minutes,
                })
            match = {
                "stage": row["stage"], "seed": int(number(row, "seed", path, line)),
                "players": players, "bot": row["bot"],
                "seconds": number(row, "requested_active_seconds", path, line), "seats": seats,
            }
            key = (match["stage"], match["seed"], players, match["bot"], match["seconds"],
                   tuple((s["team"], s["tool"]) for s in seats))
            if key in seen:
                raise ValueError(f"{path}:{line}: repeats the match at {seen[key]}; pass each run once")
            seen[key] = f"{path}:{line}"
            matches.append(match)
            sources[-1]["rows"] += 1
    if not matches:
        raise ValueError("The inputs hold no match rows")
    return matches, sources


# --- Units and strata ---

def mean(values):
    return math.fsum(values) / len(values)


def sample_var(values):
    if len(values) < 2:
        return None
    m = mean(values)
    return math.fsum((v - m) ** 2 for v in values) / (len(values) - 1)


def pair_strata(matches):
    """1v1 matches as side-swap units. Returns (strata, counts)."""
    by_seed = {}
    for match in matches:
        a, b = sorted(match["seats"], key=lambda s: s["team"])
        if a["tool"] == b["tool"]:
            key = (match["stage"], (a["tool"], a["tool"]), match["seed"])
            if key in by_seed:
                raise ValueError(f"Two {a['tool']} mirrors on {match['stage']} seed {match['seed']}")
            by_seed[key] = {"mirror": (a, b)}
            continue
        x, y = sorted((a["tool"], b["tool"]))
        slot = by_seed.setdefault((match["stage"], (x, y), match["seed"]), {})
        orientation = "x_on_a" if a["tool"] == x else "x_on_b"
        if orientation in slot:
            raise ValueError(f"Two matches put {x} on the same team against {y} "
                             f"on {match['stage']} seed {match['seed']}")
        slot[orientation] = (a, b) if orientation == "x_on_a" else (b, a)  # (x seat, y seat)
    strata, swapped, unswapped, side_halves = {}, 0, 0, []
    for (stage, (x, y), seed), slot in sorted(by_seed.items()):
        if "mirror" in slot:
            unit = {(stage, x, x): {m: mean([s[m] for s in slot["mirror"]]) for m in MEASURES}}
        else:
            if len(slot) < 2:
                # One side only carries the whole side advantage: counted, not estimated.
                unswapped += 1
                continue
            swapped += 1
            xa, xb = slot["x_on_a"][0], slot["x_on_b"][0]
            side_halves.append(((stage, x, y), (xa["paint"] - xb["paint"]) / 2.0, xa["paint"], xb["paint"]))
            orientations = list(slot.values())
            unit = {(stage, x, y): {m: mean([o[0][m] for o in orientations]) for m in MEASURES},
                    (stage, y, x): {m: mean([o[1][m] for o in orientations]) for m in MEASURES}}
        strata.setdefault((stage, (x, y)), []).append(unit)
    return strata, {"swapped": swapped, "unswapped": unswapped, "side_halves": side_halves}


def seat_strata(matches):
    """Other rosters: one unit per match, one stratum per stage and roster."""
    strata = {}
    for match in matches:
        roster = "/".join(f"{s['team']}:{s['tool']}" for s in sorted(match["seats"], key=lambda s: (s["team"], s["tool"])))
        unit = {}
        for tool in sorted({s["tool"] for s in match["seats"]}):
            seats = [s for s in match["seats"] if s["tool"] == tool]
            unit[(match["stage"], roster, tool)] = {m: mean([s[m] for s in seats]) for m in MEASURES}
        strata.setdefault((match["stage"], roster), []).append(unit)
    return strata


def contrast(strata, measure, weights):
    """A weighted sum of cell means, with its standard error and Welch-Satterthwaite df.

    The standard error is None when a stratum it needs has a single unit, or when
    every unit agrees exactly: no spread was observed, so no interval is claimed.
    """
    estimate, variance, df_den, missing = 0.0, 0.0, 0.0, set(weights)
    for units in strata.values():
        cells = [cell for cell in units[0] if cell in weights]
        if not cells:
            continue
        missing.difference_update(cells)
        g = [math.fsum(weights[c] * unit[c][measure] for c in cells) for unit in units]
        estimate += mean(g)
        var = sample_var(g)
        if var is not None and var <= (SPREAD_TOLERANCE * max(1.0, max(abs(v) for v in g))) ** 2:
            var = 0.0  # rounding, not spread
        if var is None:
            variance = None
            continue
        if variance is not None:
            v = var / len(g)
            variance += v
            df_den += v * v / (len(g) - 1)
    if missing:
        raise ValueError(f"No units for cells {sorted(missing)}")
    if not variance:
        return {"mean": estimate, "se": None, "df": None, "half_width": None}
    se = math.sqrt(variance)
    df = variance * variance / df_den
    return {"mean": estimate, "se": se, "df": df, "half_width": two_sided(df) * se}


def detectable(stat):
    """Approximate smallest A/B change found 80% of the time at two-sided 5%.

    Two equal, independent runs: the difference has sqrt(2) times one run's
    error and, with equal spreads, twice its degrees of freedom.
    """
    if not stat or stat["se"] is None:
        return None
    return (two_sided(2.0 * stat["df"]) + Z_POWER) * math.sqrt(2.0) * stat["se"]


# --- Analysis ---

def cells_of(strata):
    return sorted({cell for units in strata.values() for cell in units[0]})


def tool_weights(cells, tool, kind, include_mirrors):
    if kind == "pairs":
        chosen = [c for c in cells if c[1] == tool and (include_mirrors or c[2] != tool)]
    else:
        # A roster of one tool says nothing about it against others, like a 1v1 mirror.
        chosen = [c for c in cells if c[2] == tool and (include_mirrors or not one_tool_roster(c[1]))]
    return {c: 1.0 / len(chosen) for c in chosen} if chosen else {}


def one_tool_roster(roster):
    return len({seat.split(":", 1)[1] for seat in roster.split("/")}) == 1


def minus(a, b, scale=1.0):
    out = dict(a)
    for cell, w in b.items():
        out[cell] = out.get(cell, 0.0) - scale * w
    return out


def significantly(stat):
    """+1 above zero, -1 below, 0 neither, None when there is no interval."""
    if stat["se"] is None:
        return None
    return 1 if stat["mean"] > stat["half_width"] else (-1 if stat["mean"] < -stat["half_width"] else 0)


def validate_profiles(profiles):
    """Refuse a malformed profiles document before any data is read."""
    if not isinstance(profiles, dict):
        raise ValueError("profiles: 'tools' must be an object")
    for tool, levels in profiles.items():
        if not isinstance(levels, dict):
            raise ValueError(f"profiles: {tool} must map measures to levels")
        for measure, level in levels.items():
            if measure not in MEASURES:
                raise ValueError(f"profiles: {tool}.{measure} is not a known measure ({', '.join(MEASURES)})")
            if isinstance(level, dict):
                listed = level.get("between")
                if (set(level) != {"between"} or not isinstance(listed, list) or len(listed) < 2
                        or len(set(listed)) != len(listed) or tool in listed):
                    raise ValueError(f"profiles: {tool}.{measure} 'between' needs two or more other tools")
                unknown = [o for o in listed if o not in profiles]
                if unknown:
                    raise ValueError(f"profiles: {tool}.{measure} 'between' names unknown tools {unknown}")
            elif level not in LEVELS:
                raise ValueError(f"profiles: {tool}.{measure} = {level!r} is not a known level ({', '.join(LEVELS)})")


RANK = {"least": 0, "low": 1, "mid": 2, "high": 3, "most": 4}


def check_profiles(strata, weights, profiles):
    """Each stated level becomes confirmed, consistent, off, no interval or not in data.

    Comparisons are unadjusted at 95%. The levels are an order, least < low <
    mid < high < most, and a tool is judged against the other tools' stated
    levels on the same measure: a tool stated higher than another should be
    significantly above it. Off: some stated order is significantly reversed
    (the note names the tools). Confirmed: every stated order holds
    significantly. Consistent: neither.

    Further: most and least are also judged against every other tool. mid is
    also off as the outright most or least. A tool whose level no other tool's
    differs from (or that has none to compare with) is judged against the median
    of the other tools' marginals instead, chosen by point estimate. between
    [tools]: confirmed when above one listed tool and below another, off when
    beyond all of them on one side.
    """
    verdicts, tools = {}, sorted(weights)

    def sign(tool, other, measure):
        return significantly(contrast(strata, measure, minus(weights[tool], weights[other])))

    for tool, levels in sorted(profiles.items()):
        for measure, level in sorted(levels.items()):
            text = "between " + ", ".join(level["between"]) if isinstance(level, dict) else level
            entry = {"level": text, "verdict": "not in data", "note": ""}
            verdicts.setdefault(tool, {})[measure] = entry
            if tool not in weights or len(tools) < 2:
                continue
            if isinstance(level, dict):
                present = [o for o in level["between"] if o in weights]
                if len(present) < 2:
                    continue
                signs = [sign(tool, o, measure) for o in present]
                if None in signs:
                    entry["verdict"] = "no interval"
                elif all(s == 1 for s in signs) or all(s == -1 for s in signs):
                    entry["verdict"] = "off"
                    entry["note"] = ("above " if signs[0] == 1 else "below ") + "all of " + ", ".join(present)
                elif 1 in signs and -1 in signs:
                    entry["verdict"] = "confirmed"
                else:
                    entry["verdict"] = "consistent"
                continue
            others = [o for o in tools if o != tool]
            everyone = {o: sign(tool, o, measure) for o in others}
            if None in everyone.values():
                entry["verdict"] = "no interval"
                continue
            peers = {o: RANK[profiles[o][measure]] for o in others
                     if isinstance(profiles.get(o, {}).get(measure), str)
                     and RANK[profiles[o][measure]] != RANK[level]}
            reversed_by, held = [], True
            for other, rank in sorted(peers.items()):
                want = 1 if RANK[level] > rank else -1
                if everyone[other] == -want:
                    reversed_by.append(f"{'below' if want == 1 else 'above'} {other} ({profiles[other][measure]})")
                held = held and everyone[other] == want
            if level in ("most", "least"):
                want = 1 if level == "most" else -1
                beaten = [o for o, s in everyone.items() if s == -want]
                reversed_by += [f"{'below' if want == 1 else 'above'} {o}" for o in beaten
                                if not any(o in r for r in reversed_by)]
                held = held and all(s == want for s in everyone.values())
            elif level == "mid" and (all(s == 1 for s in everyone.values()) or all(s == -1 for s in everyone.values())):
                reversed_by.append("the outright " + ("most" if all(s == 1 for s in everyone.values()) else "least"))
            if not peers and level in ("high", "mid", "low"):
                # No stated order to test: judge against the median other tool.
                ordered = sorted(others, key=lambda o: contrast(strata, measure, weights[o])["mean"])
                middle = ordered[(len(ordered) - 1) // 2: len(ordered) // 2 + 1]
                centre = {}
                for o in middle:
                    centre = minus(centre, weights[o], -1.0 / len(middle))
                vs = significantly(contrast(strata, measure, minus(weights[tool], centre)))
                if vs is None:
                    entry["verdict"] = "no interval"
                    continue
                want = {"high": 1, "low": -1, "mid": 0}[level]
                if level != "mid" and vs == -want:
                    reversed_by.append(f"{'below' if want == 1 else 'above'} the median of the others")
                held = (vs == want) if level != "mid" else (1 in everyone.values() and -1 in everyone.values())
                entry["note"] = "against the median of the others"
            if reversed_by:
                entry["verdict"], entry["note"] = "off", "; ".join(reversed_by)
            else:
                entry["verdict"] = "confirmed" if held else "consistent"
    return verdicts


def analyze_group(matches, profiles, include_mirrors):
    players = matches[0]["players"]
    kind = "pairs" if players == 2 else "seats"
    if kind == "pairs":
        strata, counts = pair_strata(matches)
    else:
        strata, counts = seat_strata(matches), {}
    cells = cells_of(strata)
    tools = sorted({c[-1] if kind == "seats" else c[1] for c in cells})
    stages = sorted({c[0] for c in cells})
    weights = {t: tool_weights(cells, t, kind, include_mirrors) for t in tools}
    weights = {t: w for t, w in weights.items() if w}
    result = {
        "kind": kind, "players": players, "bot": matches[0]["bot"], "seconds": matches[0]["seconds"],
        "matches": len(matches), "stages": stages, "tools": tools,
        "seeds": sorted({m["seed"] for m in matches}),
        "units": sum(len(u) for u in strata.values()),
        "marginals": {t: {m: contrast(strata, m, weights[t]) for m in MEASURES} for t in weights},
        "profiles": check_profiles(strata, weights, profiles),
    }
    table = {}
    for m in MEASURES:
        for stage in stages:
            for tool in tools:
                if kind == "pairs":
                    for other in tools:
                        if (stage, tool, other) in cells:
                            table[(m, stage, tool, other)] = contrast(strata, m, {(stage, tool, other): 1.0})
                else:
                    chosen = [c for c in cells if c[0] == stage and c[2] == tool]
                    if chosen:
                        table[(m, stage, tool)] = contrast(strata, m, {c: 1.0 / len(chosen) for c in chosen})
    result["table"] = table
    if kind == "pairs":
        result["swapped"], result["unswapped"] = counts["swapped"], counts["unswapped"]
        halves = counts["side_halves"]
        result["side_effect"] = side_effect(halves)
        result["orientation_correlation"] = orientation_correlation(halves)
    result["sensitivity"] = sensitivity(result, strata, cells, kind, include_mirrors)
    return result


def side_effect(halves):
    """Mean of half the A-B paint difference, once per swapped pair, with a t interval."""
    values = [h[1] for h in halves]
    if not values:
        return None
    var = sample_var(values)
    if not var or var <= (SPREAD_TOLERANCE * max(1.0, max(abs(v) for v in values))) ** 2:
        return {"mean": mean(values), "half_width": None}
    se = math.sqrt(var / len(values))
    return {"mean": mean(values), "half_width": two_sided(len(values) - 1) * se}


def orientation_correlation(halves):
    """Within-cell correlation of X's paint as team A with X's paint as team B over seeds."""
    by_cell = {}
    for cell, _, a, b in halves:
        by_cell.setdefault(cell, []).append((a, b))
    xs, ys = [], []
    for values in by_cell.values():
        if len(values) < 2:
            continue
        ma, mb = mean([a for a, _ in values]), mean([b for _, b in values])
        xs += [a - ma for a, _ in values]
        ys += [b - mb for _, b in values]
    sxx, syy = math.fsum(x * x for x in xs), math.fsum(y * y for y in ys)
    if len(xs) < 3 or sxx <= 0 or syy <= 0:
        return None
    return math.fsum(x * y for x, y in zip(xs, ys)) / math.sqrt(sxx * syy)


def sensitivity(result, strata, cells, kind, include_mirrors):
    rows = {}
    for m in MEASURES:
        if kind == "pairs":
            keys = [(m,) + c for c in cells if include_mirrors or c[1] != c[2]]
        else:
            keys = [k for k in result["table"] if k[0] == m]
        stats = [result["table"][k] for k in keys if k in result["table"]]
        sds = [s["se"] * math.sqrt(n) for s, n in ((s, unit_count(strata, k[1:], kind)) for s, k in zip(stats, keys))
               if s["se"] is not None and n] if kind == "pairs" else []
        hws = [s["half_width"] for s in stats if s["half_width"] is not None]
        cell_mde = [detectable(s) for s in stats if s["se"] is not None]
        margs = [result["marginals"][t][m] for t in result["marginals"]]
        pick = lambda xs: median(xs) if xs else None
        rows[m] = {
            "cell_sd_median": pick(sds), "cell_half_width_median": pick(hws),
            "cell_detectable_median": pick(cell_mde),
            "tool_half_width_median": pick([s["half_width"] for s in margs if s["half_width"] is not None]),
            "tool_detectable_median": pick([detectable(s) for s in margs if s["se"] is not None]),
        }
    return rows


def unit_count(strata, cell, kind):
    stage, x, y = cell
    return len(strata.get((stage, tuple(sorted((x, y)))), []))


def analyze(matches, profiles, include_mirrors=False):
    validate_profiles(profiles)
    groups = {}
    for match in matches:
        groups.setdefault((match["players"], match["bot"], match["seconds"]), []).append(match)
    return [analyze_group(members, profiles, include_mirrors) for _, members in sorted(groups.items())]


# --- Output ---

def fmt(value, digits=3):
    return "-" if value is None else f"{value:.{digits}f}"


def describe(measure, verdict):
    text = f"{measure} {verdict['level']}: "
    text += f"**{verdict['verdict']}**" if verdict["verdict"] == "off" else verdict["verdict"]
    return text + (f" ({verdict['note']})" if verdict.get("note") else "")


def pm(stat, digits=3):
    if not stat:
        return ""
    if stat.get("half_width") is None:
        return f"{stat['mean']:.{digits}f} (no interval)"
    return f"{stat['mean']:.{digits}f} ± {stat['half_width']:.{digits}f}"


def render(groups, sources, label):
    lines = [f"# Balance summary{': ' + label if label else ''}", "",
             "Evidence, not acceptance: bot matches describe these bots with these tools "
             "(docs/gameplay-tuning.md#weapon-profiles-2026-11-10). Intervals are 95%. Profile "
             "checks are unadjusted comparisons, so among many checks a few can flip by chance; "
             "'no interval' means some stratum had one unit or no spread.", "",
             "## Inputs", "", "| CSV | rows | SHA256 |", "|---|---|---|"]
    for s in sources:
        lines.append(f"| `{s['path']}` | {s['rows']} | `{s['sha256']}` |")
    for g in groups:
        lines += ["", f"## {g['players']} players, `{g['bot']}`, {g['seconds']:g} s: {g['matches']} matches", "",
                  f"Stages: {', '.join(g['stages'])}. Seeds: {', '.join(map(str, g['seeds']))}. "
                  f"Independent units: {g['units']}."]
        if g["kind"] == "pairs":
            lines.append(f"Side-swapped pairs: {g['swapped']}; pairs missing their swapped match: {g['unswapped']}.")
            if g["side_effect"]:
                lines.append(f"Team A side advantage in share of painted (half the A-B difference "
                             f"for one tool across a swap): {pm(g['side_effect'])}.")
            lines.append(f"Within-cell correlation of a tool's share between its two orientations: "
                         f"{fmt(g['orientation_correlation'], 2)}. Near zero, a tuning change cannot "
                         "be read as a paired comparison on the same seeds.")
        else:
            lines.append("Not 1v1: a unit is one match; each tool's value is the mean over its seats in it.")
        lines += ["", "### Tools", "",
                  "| Tool | " + " | ".join(MEASURE_LABELS[m] for m in MEASURES) + " | Profile |",
                  "|---|" + "---|" * len(MEASURES) + "---|"]
        for tool in sorted(g["marginals"]):
            prof = g["profiles"].get(tool, {})
            text = "; ".join(describe(m, v) for m, v in prof.items()) or "none stated"
            lines.append(f"| {tool} | " + " | ".join(pm(g["marginals"][tool][m]) for m in MEASURES)
                         + f" | {text} |")
        absent = sorted(t for t, prof in g["profiles"].items() if t not in g["marginals"])
        if absent:
            lines += ["", "Stated in the profiles but not in this data: " + ", ".join(absent) + "."]
        if g["kind"] == "pairs":
            lines.append("")
            lines.append("A tool's marginal is the equal-weight mean of its cells against other tools"
                         " (stage x opponent), mirrors excluded unless asked.")
        lines += ["", "### Sensitivity", "",
                  "| Measure | cell sd | cell ± | cell A/B detectable | tool ± | tool A/B detectable |",
                  "|---|---|---|---|---|---|"]
        for m in MEASURES:
            s = g["sensitivity"][m]
            lines.append(f"| {MEASURE_LABELS[m]} | {fmt(s['cell_sd_median'])} | {fmt(s['cell_half_width_median'])} | "
                         f"{fmt(s['cell_detectable_median'])} | {fmt(s['tool_half_width_median'])} | "
                         f"{fmt(s['tool_detectable_median'])} |")
        lines += ["", "Medians over non-mirror cells and over tools; cell sd is the spread of one unit. "
                  "Detectable: the change an unpaired comparison of two runs of this size finds 80% "
                  "of the time at two-sided 5%."]
        for m in ("paint", "kills"):
            if g["kind"] == "pairs":
                lines += ["", f"### {MEASURE_LABELS[m].capitalize()} by stage (row tool against column)", ""]
                for stage in g["stages"]:
                    lines += [f"**{stage}**", "", "| | " + " | ".join(g["tools"]) + " |",
                              "|---|" + "---|" * len(g["tools"])]
                    for tool in g["tools"]:
                        lines.append(f"| {tool} | " + " | ".join(
                            pm(g["table"].get((m, stage, tool, other)), 2) for other in g["tools"]) + " |")
                    lines.append("")
            else:
                lines += ["", f"### {MEASURE_LABELS[m].capitalize()} by stage (mean over rosters)", "",
                          "| Tool | " + " | ".join(g["stages"]) + " |", "|---|" + "---|" * len(g["stages"])]
                for tool in g["tools"]:
                    lines.append(f"| {tool} | " + " | ".join(
                        pm(g["table"].get((m, stage, tool)), 2) for stage in g["stages"]) + " |")
    return "\n".join(lines).rstrip() + "\n"


def jsonable(groups, sources):
    out = []
    for g in groups:
        item = {k: v for k, v in g.items() if k != "table"}
        item["table"] = [dict(key=list(k), **v) for k, v in sorted(g["table"].items())]
        out.append(item)
    return {"schema_version": 1, "sources": sources, "groups": out}


def refuse_tree_output(out_dir):
    """Refuse a folder inside the repository unless it is under out/.

    Compared by file identity, not spelling, so a long-path or UNC spelling of
    the repository is still the repository.
    """
    chain = [Path(os.path.abspath(out_dir))]
    chain += list(chain[0].parents)
    for index, ancestor in enumerate(chain):
        try:
            same = ancestor.exists() and os.path.samefile(ancestor, REPO_ROOT)
        except OSError:
            same = False
        if same:
            if index == 0 or chain[index - 1].name.lower() != "out":
                raise ValueError(f"Refusing to write {out_dir}: inside the repository but outside out/")
            return


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("inputs", nargs="+", help="matches.csv files or folders searched for them")
    parser.add_argument("--out", type=Path, help="folder for summary.md and summary.json (default: print)")
    parser.add_argument("--profiles", type=Path, default=DEFAULT_PROFILES)
    parser.add_argument("--label", default="", help="a title for the summary")
    parser.add_argument("--include-mirrors", action="store_true",
                        help="count a tool against itself in its marginals")
    args = parser.parse_args(argv)
    try:
        if args.out:
            refuse_tree_output(args.out)
        profiles = json.loads(args.profiles.read_text(encoding="utf-8"))["tools"]
        matches, sources = read_matches(find_csvs(args.inputs))
        groups = analyze(matches, profiles, args.include_mirrors)
        text = render(groups, sources, args.label)
        if args.out:
            args.out.mkdir(parents=True, exist_ok=True)
            (args.out / "summary.md").write_text(text, encoding="utf-8", newline="\n")
            (args.out / "summary.json").write_text(
                json.dumps(jsonable(groups, sources), indent=1, sort_keys=True) + "\n",
                encoding="utf-8", newline="\n")
            print(f"Wrote {args.out / 'summary.md'}")
        else:
            sys.stdout.write(text)
    except ValueError as error:
        print(f"analyze.py: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
