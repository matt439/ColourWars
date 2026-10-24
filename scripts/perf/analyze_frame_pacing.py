"""Summarize bounded CPU wall-time captures; never certify a hardware baseline."""

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path


TIMING_SCOPE = "match_draw_begin_to_begin_cpu_wall_ms"
ENGINE_TIMING_SCOPE = "matched_completed_application_frame_cpu_wall_ms"
FIELDS = ["frame", "frame_ms", "update_ms", "draw_ms", "updates", "objects", "projectiles"]
FIELDS_V2 = ["frame", "application_frame", "frame_ms", "update_ms", "draw_ms",
             "begin_frame_ms", "submit_ms", "end_frame_ms", "updates", "objects", "projectiles"]
ENGINE_FIELDS = ("begin_frame_ms", "submit_ms", "end_frame_ms")


def percentile(values, fraction):
    """Nearest rank, including the smallest and largest observations."""
    ordered = sorted(values)
    return ordered[max(0, math.ceil(len(ordered) * fraction) - 1)]


def series(values):
    return {
        "min": min(values),
        "mean": math.fsum(values) / len(values),
        "p50": percentile(values, 0.50),
        "p95": percentile(values, 0.95),
        "p99": percentile(values, 0.99),
        "max": max(values),
    }


def integer(value, name, minimum=0):
    # Reject booleans and fractional/coerced counters, including JSON true.
    if isinstance(value, bool) or str(value) != str(int(value)) or int(value) < minimum:
        raise ValueError(f"{name} must be an integer >= {minimum}")
    return int(value)


def capture_schema(capture):
    if not isinstance(capture, dict) or type(capture.get("schema_version")) is not int or \
            capture["schema_version"] not in (1, 2) or capture.get("timing_scope") != TIMING_SCOPE:
        raise ValueError("Unsupported capture schema or timing scope")
    return capture["schema_version"]


def engine_duration(value, name):
    # The engine phases may genuinely take zero measured nanoseconds. Missing
    # timings, a JSON boolean or a non-finite clock value must not become zero.
    if isinstance(value, bool):
        raise ValueError(f"{name} must be finite and nonnegative")
    try:
        number = float(value)
    except (ValueError, TypeError, OverflowError) as error:
        raise ValueError(f"{name} must be finite and nonnegative") from error
    if not math.isfinite(number) or number < 0:
        raise ValueError(f"{name} must be finite and nonnegative")
    return number


def summarize(capture, rows, metadata):
    if not isinstance(capture, dict) or not isinstance(metadata, dict):
        raise ValueError("Capture and metadata must be JSON objects")
    schema = capture_schema(capture)
    fields = FIELDS if schema == 1 else FIELDS_V2
    requested = integer(capture.get("requested_frames"), "requested_frames", 1)
    captured = integer(capture.get("captured_frames"), "captured_frames", 1)
    if requested > 36000 or captured != requested or len(rows) != captured:
        raise ValueError("Capture length does not match its requested rendered intervals")
    if capture.get("complete") is not True or capture.get("reason") != "complete":
        raise ValueError("Interrupted/incomplete captures cannot be summarized as complete measurements")
    if schema == 2:
        paired = capture.get("engine_paired_frames")
        if capture.get("engine_timing_scope") != ENGINE_TIMING_SCOPE or \
                type(paired) is not int or paired != captured:
            raise ValueError("Schema 2 requires the exact engine timing scope and every captured frame paired")
    if metadata.get("load_condition") not in ("shared", "uncontended", "unknown"):
        raise ValueError("Record load_condition as shared, uncontended or unknown")
    if not isinstance(metadata.get("load_note"), str) or not metadata["load_note"].strip():
        raise ValueError("Record an explicit load_note")
    parsed = []
    for index, row in enumerate(rows, 1):
        if set(row) != set(fields) or integer(row["frame"], "frame", 1) != index:
            raise ValueError("Raw frames must use the exact columns and chronological sequence")
        frame = {name: integer(row[name], name) for name in ("updates", "objects", "projectiles")}
        for name in ("frame_ms", "update_ms", "draw_ms"):
            number = float(row[name])
            if not math.isfinite(number) or number < 0 or (name == "frame_ms" and number == 0):
                raise ValueError(f"{name} must be finite and {'positive' if name == 'frame_ms' else 'nonnegative'}")
            frame[name] = number
        if schema == 2:
            application_frame = integer(row["application_frame"], "application_frame", 1)
            if parsed and application_frame != parsed[-1]["application_frame"] + 1:
                raise ValueError("Schema 2 application frame IDs must be positive and consecutive")
            frame["application_frame"] = application_frame
            for name in ENGINE_FIELDS:
                frame[name] = engine_duration(row[name], name)
        parsed.append(frame)
    intervals = [frame["frame_ms"] for frame in parsed]
    elapsed_ms = math.fsum(intervals)
    warnings = [
        "CPU draw-begin intervals include intervening engine/submit/Present wall time indirectly; "
        "they are not GPU timestamps, separate Present durations or measured scan-out.",
        "Update and draw CPU components are diagnostics, not a sum accounting for each interval.",
        "Threshold exceedances are observations, not a strict pass/fail test at 59.94/60 Hz.",
        "This instrumented capture does not establish a supported hardware baseline.",
    ]
    if schema == 2:
        warnings.append("Matched engine begin_frame/end_frame CPU wall times may include GPU/display waits; "
                        "the split between draw and submit is backend-dependent. These phases are not "
                        "GPU time or CPU busy time and need not sum to the draw-begin interval.")
    if metadata["load_condition"] != "uncontended":
        warnings.append(f"{metadata['load_condition']} load: do not treat this as an uncontended acceptance baseline.")
    if metadata.get("hidden", True):
        warnings.append("Hidden-window Present may return rapidly while occluded; this does not qualify visible frame pacing.")
    result = {
        "schema_version": schema,
        "capture_valid": True,
        "baseline_accepted": False,
        "timing_scope": TIMING_SCOPE,
        "percentile_method": "nearest_rank",
        "frames": captured,
        "elapsed_interval_seconds": elapsed_ms / 1000.0,
        "aggregate_draws_per_second": captured * 1000.0 / elapsed_ms,
        "frame_ms": series(intervals),
        "update_ms": series([frame["update_ms"] for frame in parsed]),
        "draw_ms": series([frame["draw_ms"] for frame in parsed]),
        "threshold_counts": {
            "over_1000_div_60_ms": sum(value > 1000.0 / 60.0 for value in intervals),
            "over_1000_div_30_ms": sum(value > 1000.0 / 30.0 for value in intervals),
            "over_50_ms": sum(value > 50.0 for value in intervals),
        },
        "simulation": {
            "updates": sum(frame["updates"] for frame in parsed),
            "zero_update_frames": sum(frame["updates"] == 0 for frame in parsed),
            "catch_up_frames": sum(frame["updates"] > 1 for frame in parsed),
        },
        "population": {
            "max_objects": max(frame["objects"] for frame in parsed),
            "max_projectiles": max(frame["projectiles"] for frame in parsed),
        },
        "warnings": warnings,
        "metadata": metadata,
    }
    if schema == 2:
        result.update({name: series([frame[name] for frame in parsed]) for name in ENGINE_FIELDS})
        result.update({
            "engine_timing_scope": ENGINE_TIMING_SCOPE,
            "engine_paired_frames": captured,
            "application_frames": {"first": parsed[0]["application_frame"],
                                   "last": parsed[-1]["application_frame"], "count": captured},
        })
    return result


def analyze(capture_path, frames_path, metadata_path):
    capture = json.loads(capture_path.read_text(encoding="utf-8-sig"))
    metadata = json.loads(metadata_path.read_text(encoding="utf-8-sig"))
    schema = capture_schema(capture)
    with frames_path.open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != (FIELDS if schema == 1 else FIELDS_V2):
            raise ValueError("Unexpected raw frame columns")
        rows = list(reader)
    result = summarize(capture, rows, metadata)
    result["input_sha256"] = {
        name: hashlib.sha256(path.read_bytes()).hexdigest()
        for name, path in (("capture", capture_path), ("frames", frames_path), ("metadata", metadata_path))
    }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("capture", "frames", "metadata", "output"):
        parser.add_argument(f"--{name}", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = analyze(args.capture, args.frames, args.metadata)
        args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    except (ValueError, TypeError, OverflowError, OSError) as error:
        parser.exit(1, f"Invalid frame-pacing capture: {error}\n")
    print(f"Recorded {result['frames']} rendered intervals; {result['metadata']['load_condition']} load; baseline pending.")


if __name__ == "__main__":
    main()
