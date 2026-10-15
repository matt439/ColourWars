"""Publish a technically checked, accepted bank without re-encoding its masters.

    python scripts/audio/audio_pipeline.py build
    python scripts/audio/publish_runtime_bank.py

Normal CMake builds only verify the published artifact and its receipt. Python
and FFmpeg belong to this explicit authoring workflow, not the game build.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil

import audio_pipeline as pipeline


def accepted_masters(manifest: dict, report: dict, acceptance: dict) -> dict[str, str]:
    measured = {cue["id"]: cue["sha256"] for cue in report["cues"]}
    pipeline.require(acceptance.get("status") == "accepted", "production set is not accepted")
    pipeline.require(acceptance.get("master_sha256") == measured,
                     "accepted master hashes differ from the current production set")
    pipeline.require([cue["id"] for cue in manifest["cues"]] == [cue["id"] for cue in report["cues"]],
                     "production report cue order differs")
    return measured


def publish(output: Path, root: Path = pipeline.REPOSITORY) -> dict:
    production = root / "game/audio/paint_exe"
    manifest, measured = pipeline.validate(production / "cues.json", production / "sources.json", root)
    acceptance = json.loads((production / "acceptance.json").read_text(encoding="utf-8-sig"))
    masters = accepted_masters(manifest, measured, acceptance)
    report = json.loads((output / "build-report.json").read_text(encoding="utf-8-sig"))
    pipeline.require(report.get("warnings") == [], "build report contains technical listening flags")
    pipeline.require({cue["id"]: cue["sha256"] for cue in report["exports"]} == masters,
                     "encoded bank was built from different masters")
    source = output / "paint_exe.xwb"
    bank = pipeline.inspect_bank(source, report["exports"])
    pipeline.require(bank["sha256"] == report["bank"]["sha256"], "bank differs from its build report")
    receipt = {"version": 1, "bank": "paint_exe.xwb", "bytes": bank["bytes"],
               "sha256": bank["sha256"], "waves": [cue["id"] for cue in manifest["cues"]],
               "accepted_master_sha256": masters,
               "music_encoding": {"format": "MS-ADPCM", "samples_per_block": 512,
                                  "stereo_block_bytes": 524,
                                  "loop_padding": "Hold the first boundary sample for at most 511 derived frames; accepted masters remain unchanged.",
                                  "encoder": "FFmpeg 1012-frame overlapping windows, retain first 512 frames per independent block."},
               "music_exports": [{key: exported[key] for key in
                   ("id", "sha256", "export_sha256", "frames", "export_frames", "padding_frames",
                    "loop_start", "loop_length", "decoded")}
                   for exported in report["exports"] if exported["id"].startswith("music.")],
               "warnings": report["warnings"], "tools": report["tools"]}
    shutil.copyfile(source, root / "game/content/sounds/paint_exe.xwb")
    (root / "scripts/audio/runtime-bank.json").write_text(
        json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=pipeline.REPOSITORY / "out/audio-production")
    args = parser.parse_args()
    try:
        receipt = publish(args.output.resolve())
        print(f"Published {len(receipt['waves'])} accepted waves ({receipt['bytes']:,} bytes).")
        return 0
    except (pipeline.InvalidAudio, OSError, ValueError, KeyError) as error:
        print(f"Runtime bank publication failed: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
