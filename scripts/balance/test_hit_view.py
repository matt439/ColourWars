"""hit_view.py reads the runner's lines and only the passed jobs' logs."""
import json
from pathlib import Path
import tempfile
import unittest

import hit_view

LINE = ("hit-view stage=crossfeed seed=1000 shooter=0 tool=sniper victim_tool=roller hits=10 "
        "outside=3 partial=1 shooter_down=2 eliminations=1 pane_px=360 body_px=80")


class HitViewTests(unittest.TestCase):
    def test_parse_line(self):
        row = hit_view.parse_line(LINE)
        self.assertEqual((row["tool"], row["hits"], row["outside"], row["partial"]), ("sniper", 10, 3, 1))
        self.assertIsNone(hit_view.parse_line("PASS balance runner: 1 completed matches"))
        with self.assertRaises(ValueError):
            hit_view.parse_line(LINE.replace("outside=3", "outside=10"))

    def test_job_rows_reads_passed_variant_jobs_only(self):
        with tempfile.TemporaryDirectory() as scratch:
            out = Path(scratch)

            def job(name, variant, outcome):
                base = out / "jobs" / name
                (base / "attempt-1").mkdir(parents=True)
                (base / "attempt-1" / "run.log").write_text("noise\n" + LINE + "\n", encoding="utf-8")
                (base / "job.json").write_text(json.dumps(
                    {"outcome": outcome, "attempt": "attempt-1", "job": {"variant": variant}}), encoding="utf-8")
            job("a", "baseline", "passed")
            job("b", "baseline", "failed")
            job("c", "other", "passed")
            job("identity-repeat", "baseline", "passed")
            rows, jobs = hit_view.job_rows(out, "baseline")
            self.assertEqual((len(rows), jobs), (1, 1))
            summary = hit_view.summarize(rows)["groups"]["all"]
            self.assertEqual(summary["hits"], 10)
            self.assertAlmostEqual(summary["unconfirmed_fraction"], 0.4)


if __name__ == "__main__":
    unittest.main()
