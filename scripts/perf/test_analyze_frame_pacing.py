import copy
import csv
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from analyze_frame_pacing import ENGINE_FIELDS, ENGINE_TIMING_SCOPE, FIELDS, FIELDS_V2, TIMING_SCOPE, analyze, summarize


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

    def schema_two(self):
        capture = dict(self.capture, schema_version=2, engine_timing_scope=ENGINE_TIMING_SCOPE,
                       engine_paired_frames=100)
        rows = [dict(zip(FIELDS_V2, map(str, (index, 400 + index, index, 0.25, 0.5,
                                            (index - 1) / 10.0, index % 4, index * 2,
                                            index % 3, 42, index % 7))))
                for index in range(1, 101)]
        return capture, rows

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
                             ("reason", "interrupted"), ("requested_frames", True), ("schema_version", 3),
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

    def test_legacy_capture_does_not_invent_engine_phase_data(self):
        result = self.result()
        self.assertEqual(result["schema_version"], 1)
        for name in (*ENGINE_FIELDS, "engine_timing_scope", "engine_paired_frames", "application_frames"):
            self.assertNotIn(name, result)

    def test_schema_two_pairs_application_frames_and_summarizes_engine_phases(self):
        capture, rows = self.schema_two()
        result = summarize(capture, rows, self.metadata)
        self.assertEqual(result["schema_version"], 2)
        self.assertEqual(result["engine_timing_scope"], ENGINE_TIMING_SCOPE)
        self.assertEqual(result["engine_paired_frames"], 100)
        self.assertEqual(result["application_frames"], {"first": 401, "last": 500, "count": 100})
        self.assertEqual(result["begin_frame_ms"], {"min": 0, "mean": 4.95, "p50": 4.9,
                                                    "p95": 9.4, "p99": 9.8, "max": 9.9})
        self.assertEqual(result["submit_ms"], {"min": 0, "mean": 1.5, "p50": 1,
                                               "p95": 3, "p99": 3, "max": 3})
        self.assertEqual(result["end_frame_ms"], {"min": 2, "mean": 101, "p50": 100,
                                                  "p95": 190, "p99": 198, "max": 200})
        self.assertEqual(result["frame_ms"], self.result()["frame_ms"])
        self.assertFalse(result["baseline_accepted"])
        self.assertTrue(any("GPU/display waits" in warning and "backend-dependent" in warning
                            for warning in result["warnings"]))
        # Deliberately larger end-frame durations remain valid: the QPC interval
        # is not a budget into which the diagnostic phases must fit or sum.
        self.assertGreater(result["end_frame_ms"]["max"], result["frame_ms"]["max"])

    def test_schema_two_accepts_genuine_zero_phases_and_arbitrary_start_id(self):
        capture, rows = self.schema_two()
        capture.update(requested_frames=2, captured_frames=2, engine_paired_frames=2)
        rows = rows[:2]
        for index, row in enumerate(rows):
            row["application_frame"] = str(910 + index)
            row.update({name: "0" for name in ENGINE_FIELDS})
        result = summarize(capture, rows, self.metadata)
        self.assertEqual(result["application_frames"], {"first": 910, "last": 911, "count": 2})
        for name in ENGINE_FIELDS:
            self.assertEqual(result[name], {"min": 0, "mean": 0, "p50": 0, "p95": 0, "p99": 0, "max": 0})

    def test_schema_two_requires_exact_pairing_scope_and_count(self):
        capture, rows = self.schema_two()
        for key, invalid in (("engine_timing_scope", None), ("engine_timing_scope", "unpaired_cpu_ms"),
                             ("engine_paired_frames", True), ("engine_paired_frames", False),
                             ("engine_paired_frames", 0), ("engine_paired_frames", 99),
                             ("engine_paired_frames", 101), ("engine_paired_frames", "100"),
                             ("engine_paired_frames", 100.0), ("engine_paired_frames", None)):
            with self.subTest(key=key, invalid=invalid):
                bad = dict(capture)
                bad[key] = invalid
                with self.assertRaises(ValueError):
                    summarize(bad, rows, self.metadata)
        for key in ("engine_timing_scope", "engine_paired_frames"):
            bad = dict(capture)
            del bad[key]
            with self.assertRaises(ValueError):
                summarize(bad, rows, self.metadata)

    def test_schema_two_rejects_unavailable_invalid_and_nonconsecutive_application_ids(self):
        capture, rows = self.schema_two()
        for value in ("0", "-1", "", "0401", "401.0", "1e3", True):
            with self.subTest(value=value):
                bad = copy.deepcopy(rows)
                bad[0]["application_frame"] = value
                with self.assertRaises((ValueError, TypeError)):
                    summarize(capture, bad, self.metadata)
        for value in ("401", "403", "400"):
            with self.subTest(second_id=value):
                bad = copy.deepcopy(rows)
                bad[1]["application_frame"] = value
                with self.assertRaises(ValueError):
                    summarize(capture, bad, self.metadata)

    def test_schema_two_rejects_invalid_or_absent_phase_clocks_and_columns(self):
        capture, rows = self.schema_two()
        for name in ENGINE_FIELDS:
            for value in ("-1", "nan", "inf", "-inf", "", None, True):
                with self.subTest(name=name, value=value):
                    bad = copy.deepcopy(rows)
                    bad[0][name] = value
                    with self.assertRaises(ValueError):
                        summarize(capture, bad, self.metadata)
            bad = copy.deepcopy(rows)
            del bad[0][name]
            with self.assertRaises(ValueError):
                summarize(capture, bad, self.metadata)
        bad = copy.deepcopy(rows)
        bad[0]["gpu_ms"] = "0"
        with self.assertRaises(ValueError):
            summarize(capture, bad, self.metadata)

    def test_reads_schema_two_bom_files_and_binds_all_input_hashes(self):
        capture_data, rows = self.schema_two()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            capture = root / "sample.json"
            frames = root / "sample.csv"
            metadata = root / "run-metadata.json"
            capture.write_text(json.dumps(capture_data), encoding="utf-8-sig")
            metadata.write_text(json.dumps(self.metadata), encoding="utf-8-sig")
            with frames.open("w", encoding="utf-8-sig", newline="") as stream:
                writer = csv.DictWriter(stream, FIELDS_V2)
                writer.writeheader()
                writer.writerows(rows)
            result = analyze(capture, frames, metadata)
            self.assertEqual(result["schema_version"], 2)
            self.assertEqual(result["application_frames"], {"first": 401, "last": 500, "count": 100})
            self.assertEqual(result["input_sha256"], {
                name: hashlib.sha256(path.read_bytes()).hexdigest()
                for name, path in (("capture", capture), ("frames", frames), ("metadata", metadata))
            })

    def test_both_schemas_require_their_exact_exported_header(self):
        capture_two, _ = self.schema_two()
        for capture_data, expected, other in ((self.capture, FIELDS, FIELDS_V2),
                                              (capture_two, FIELDS_V2, FIELDS)):
            headers = (other, list(reversed(expected)), expected[:-1], expected + [expected[0]])
            for header in headers:
                with self.subTest(schema=capture_data["schema_version"], header=header):
                    with tempfile.TemporaryDirectory() as directory:
                        root = Path(directory)
                        capture = root / "sample.json"
                        frames = root / "sample.csv"
                        metadata = root / "run-metadata.json"
                        capture.write_text(json.dumps(capture_data), encoding="utf-8")
                        metadata.write_text(json.dumps(self.metadata), encoding="utf-8")
                        with frames.open("w", encoding="utf-8", newline="") as stream:
                            csv.writer(stream).writerow(header)
                        with self.assertRaisesRegex(ValueError, "Unexpected raw frame columns"):
                            analyze(capture, frames, metadata)


if __name__ == "__main__":
    unittest.main()
