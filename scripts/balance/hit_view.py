"""Tally REV#37's shooter-pane hit views from a balance matrix's job logs.

The Debug balance runner prints one `hit-view` line per seat of each 1v1
match: the ticks on which that seat's projectile hurt its one opponent (a
victim-keyed hit cue fired), how many of those found the victim's body wholly
outside the shooter's own pane camera, how many showed under half of it, and
how many landed while the shooter was dead or respawning. This reads the
`run.log` of each passed job in a `run-balance-matrix.ps1` output (its
`job.json` names the attempt), optionally one variant, and reports per tool
and overall. It counts; it proposes no feedback design. Standard library only.
"""

import argparse
import json
from pathlib import Path
import sys

FIELDS = ("hits", "outside", "partial", "shooter_down", "eliminations")


def parse_line(line):
    """One `hit-view key=value ...` line as a dict, or None for any other line."""
    parts = line.strip().split()
    if not parts or parts[0] != "hit-view":
        return None
    row = dict(part.split("=", 1) for part in parts[1:])
    for name in FIELDS + ("seed", "shooter", "pane_px", "body_px"):
        row[name] = int(row[name])
    if row["outside"] + row["partial"] > row["hits"] or row["shooter_down"] > row["hits"]:
        raise ValueError("hit-view line classifies more hits than it counts: " + line.strip())
    return row


def job_rows(out, variant=None):
    """Every hit-view row from the passed attempts under a matrix output."""
    rows, jobs = [], 0
    for receipt in sorted((Path(out) / "jobs").glob("*/job.json")):
        info = json.loads(receipt.read_text(encoding="utf-8"))
        if info.get("outcome") != "passed" or receipt.parent.name == "identity-repeat":
            continue
        if variant is not None and info["job"]["variant"] != variant:
            continue
        jobs += 1
        log = receipt.parent / info["attempt"] / "run.log"
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            row = parse_line(line)
            if row is not None:
                rows.append(row)
    return rows, jobs


def summarize(rows):
    """Totals by shooter tool and overall, with the two pane fractions."""
    groups = {}
    for row in rows:
        for key in (row["tool"], "all"):
            total = groups.setdefault(key, dict.fromkeys(FIELDS, 0) | {"seats": 0})
            total["seats"] += 1
            for name in FIELDS:
                total[name] += row[name]
    for total in groups.values():
        hits = total["hits"]
        total["outside_fraction"] = total["outside"] / hits if hits else None
        total["partial_fraction"] = total["partial"] / hits if hits else None
        total["unconfirmed_fraction"] = (total["outside"] + total["partial"]) / hits if hits else None
    sizes = sorted({(row["pane_px"], row["body_px"]) for row in rows})
    return {"groups": groups, "pane_and_body_pixels": sizes}


def render(summary, jobs, label):
    def percent(value):
        return "n/a" if value is None else f"{100.0 * value:.1f}%"
    lines = [f"# Shooter-pane hit views{(': ' + label) if label else ''}", "",
             f"{jobs} passed jobs; pane/body pixel heights seen: {summary['pane_and_body_pixels']}", "",
             "| Shooter tool | Seats | Hit ticks | Outside pane | Under half in view | Shooter down | Eliminations |",
             "|---|---|---|---|---|---|---|"]
    groups = summary["groups"]
    for key in sorted(k for k in groups if k != "all") + (["all"] if "all" in groups else []):
        g = groups[key]
        lines.append(f"| {key} | {g['seats']} | {g['hits']} | {g['outside']} ({percent(g['outside_fraction'])}) | "
                     f"{g['partial']} ({percent(g['partial_fraction'])}) | {g['shooter_down']} | {g['eliminations']} |")
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("matrix", type=Path, help="a run-balance-matrix.ps1 output directory")
    parser.add_argument("--variant", help="only this design variant's jobs")
    parser.add_argument("--json", type=Path, help="also write the summary as JSON")
    parser.add_argument("--label", default="")
    args = parser.parse_args(argv)
    rows, jobs = job_rows(args.matrix, args.variant)
    if not rows:
        print("hit_view.py: no hit-view lines found (1v1 jobs from a runner with REV#37's tally?)", file=sys.stderr)
        return 1
    summary = summarize(rows)
    sys.stdout.write(render(summary, jobs, args.label))
    if args.json:
        args.json.write_text(json.dumps({"jobs": jobs, **summary}, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
