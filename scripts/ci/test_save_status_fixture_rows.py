"""Guard save-status fixture navigation; Windows retains exact raster checks."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "scripts/acceptance/painter_visual"


class SaveStatusFixtureRows(unittest.TestCase):
    def test_options_stage_finishes_on_apply_and_next_row_discards(self):
        reference = (FIXTURES / "options_reference.h").read_text()
        labels = re.search(r"options_reference_labels\{(.*?)\};", reference, re.S).group(1)
        labels = re.findall(r'L"([^"]*)"', labels)
        source = (FIXTURES / "save_status_checks.h").read_text()
        apply_row = int(re.search(r"options_apply_row = (\d+)", source).group(1))
        self.assertEqual(labels[apply_row], "Apply")
        self.assertEqual(labels[apply_row + 1], "Back")
        self.assertIn("page == 0 ? options_apply_row - staged_row", source)
        self.assertIn("else { tap(Key::down); tap(Key::enter); }", source)


if __name__ == "__main__":
    unittest.main()
