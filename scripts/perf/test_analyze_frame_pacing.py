import copy
import csv
import json
from pathlib import Path
import tempfile
import unittest

from analyze_frame_pacing import FIELDS, TIMING_SCOPE, analyze, summarize


class FramePacingChecks(unittest.TestCase):
    def setUp(self):
        self.capture = {
            "schema_version": 1, "requested_frames": 100, "captured_frames": 100,
            "complete": True, "reason": "complete", "timing_scope": TIMING_SCOPE,
        }
        self.rows = [dict(zip(FIELDS, map(str, (index, index, 0.25, 0.5, index % 3, 42, index % 7))))
                     for index in range(1, 101)]
        self.metadata = {"load_condition": "shared", "load_note": "Concurrent test suite",
                         "hidden": True, "baseline_accepted": False}

    def result(self):
        return summarize(self.capture, self.rows, self.metadata)

    def test_statistics_count_actual_rendered_intervals(self):
        result = self.result()
        self.assertEqual(result["frame_ms"], {"min": 1, "mean": 50.5, "p50": 50,
                                              "p95": 95, "p99": 99, "max": 100})
        self.assertEqual(result["threshold_counts"], {"over_1000_div_60_ms": 84,
                                                      "over_1000_div_30_ms": 67, "over_50_ms": 50})
        self.assertEqual(result["simulation"], {"updates": 100, "zero_update_frames": 33,
                                               "catch_up_frames": 33})
        self.assertAlmostEqual(result["aggregate_draws_per_second"], 100000 / 5050)
        self.assertEqual(result["population"], {"max_objects": 42, "max_projectiles": 6})

    def test_never_accepts_a_baseline_or_treats_hidden_shared_as_visible(self):
        self.assertFalse(self.result()["baseline_accepted"])
        self.assertTrue(any("Hidden-window" in warning for warning in self.result()["warnings"]))
        self.metadata.update(load_condition="uncontended", hidden=False, baseline_accepted=True)
        self.assertFalse(self.result()["baseline_accepted"])

    def test_rejects_incomplete_mismatched_or_unlabelled_runs(self):
        for key, invalid in (("requested_frames", 101), ("captured_frames", 99), ("complete", False),
                             ("reason", "interrupted"), ("requested_frames", True), ("schema_version", 2),
                             ("timing_scope", "simulation_dt")):
            with self.subTest(key=key, invalid=invalid):
                bad = copy.deepcopy(self.capture)
                bad[key] = invalid
                with self.assertRaises((ValueError, TypeError)):
                    summarize(bad, self.rows, self.metadata)
        for metadata in ({}, {"load_condition": "shared", "load_note": ""}):
            with self.assertRaises(ValueError):
                summarize(self.capture, self.rows, metadata)

    def test_rejects_bad_clocks_counters_and_row_order(self):
        for key, value in (("frame_ms", "0"), ("frame_ms", "nan"), ("draw_ms", "inf"),
                           ("update_ms", "-1"), ("updates", "0.5"), ("objects", "-1"),
                           ("frame", "2")):
            with self.subTest(key=key, value=value):
                rows = copy.deepcopy(self.rows)
                rows[0][key] = value
                with self.assertRaises(ValueError):
                    summarize(self.capture, rows, self.metadata)

    def test_reads_exported_files_and_fingerprints_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            capture = root / "sample.json"
            frames = root / "sample.csv"
            metadata = root / "run-metadata.json"
            capture.write_text(json.dumps(self.capture), encoding="utf-8-sig")
            metadata.write_text(json.dumps(self.metadata), encoding="utf-8-sig")
            with frames.open("w", encoding="utf-8", newline="") as stream:
                writer = csv.DictWriter(stream, FIELDS)
                writer.writeheader()
                writer.writerows(self.rows)
            result = analyze(capture, frames, metadata)
            self.assertTrue(result["capture_valid"])
            self.assertEqual(set(result["input_sha256"]), {"capture", "frames", "metadata"})
            self.assertTrue(all(len(value) == 64 for value in result["input_sha256"].values()))


if __name__ == "__main__":
    unittest.main()
