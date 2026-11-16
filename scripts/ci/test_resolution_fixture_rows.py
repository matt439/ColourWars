"""Guard the independent resolution fixture's named focus positions.

These source checks supplement, rather than replace, the Windows pixel probe.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "scripts/acceptance/painter_visual"


class ResolutionFixtureRows(unittest.TestCase):
    def test_focus_rows_match_independent_page_literals(self):
        reference = (FIXTURES / "options_reference.h").read_text()
        labels = re.search(r"options_reference_labels\{(.*?)\};", reference, re.S).group(1)
        labels = re.findall(r'L"([^"]*)"', labels)
        self.assertEqual(len(labels), 9)
        source = (FIXTURES / "resolution_text_checks.h").read_text()
        for name, label in [("restore_defaults_row", "Restore defaults"), ("back_row", "Back")]:
            value = int(re.search(rf"\b{name} = (\d+)", source).group(1))
            self.assertEqual(labels[value], label)
        self.assertEqual(labels.index("Back") - 2, labels.index("Restore defaults"))

    def test_evidence_verifier_uses_same_independent_focus_rows(self):
        verifier = (ROOT / "scripts/check-painter/verify-resolution-text.ps1").read_text()
        cases = re.search(r"\$resolutionExpectedCases = @\((.*?)\)", verifier, re.S).group(1)
        cases = re.findall(r"'([^']*)'", cases)
        reference = (FIXTURES / "options_reference.h").read_text()
        labels = re.search(r"options_reference_labels\{(.*?)\};", reference, re.S).group(1)
        labels = re.findall(r'L"([^"]*)"', labels)
        for case, label in [("disabled-skip-back", "Back"),
                            ("fullscreen-defaults-refresh", "Restore defaults")]:
            case_index = cases.index(case) + 1
            expected_focus = re.search(
                rf"elseif \(\$caseIndex -eq {case_index}\) \{{ '([0-9]+)' \}}", verifier)
            self.assertIsNotNone(expected_focus, case)
            self.assertEqual(int(expected_focus.group(1)), labels.index(label), case)

    def test_both_extra_ornaments_are_compared_not_just_rendered(self):
        source = (FIXTURES / "resolution_text_checks.h").read_text()
        self.assertEqual(source.count("focused == restore_defaults_row || focused == back_row"), 2)
        self.assertIn('"disabled-skip-back", next, L"Next real studio display pixels: 1600x900.", back_row, false)', source)
        self.assertIn('"fullscreen-defaults-refresh", next, L"Next real studio display pixels: 1001x777.", restore_defaults_row, false)', source)
        self.assertIn("foreground_crop(actual, extra_area, 0).pixels == foreground_crop(wanted, extra_area, 0).pixels", source)
        self.assertIn("updates == 43 && readbacks == 76 && rows == 25 && crops == 6", source)


if __name__ == "__main__":
    unittest.main()
