"""Source guards for the independent Results pixel fixtures, not GPU acceptance."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ResultsReferenceBasis(unittest.TestCase):
    def test_single_match_references_include_a_separate_revealed_denominator(self):
        for name in ["round_heading_checks.h", "series_score_checks.h"]:
            source = (ROOT / "scripts/acceptance/painter_visual" / name).read_text()
            self.assertIn('L"Team share of painted surface"', source)
            self.assertIn("fixture.info.team_a_area + fixture.info.team_b_area > 0.0f", source)
            self.assertIn("fixture.info.team_a_played && fixture.info.team_b_played", source)
            self.assertIn("fixture.session.is_series() || !details_visible", source)
            self.assertIn("look.contribution_font, Vector2F::zero", source)
            self.assertIn("look.score_basis_y", source)
            self.assertIn("row_region.contains(basis.bounds())", source)
            self.assertIn("heading.draw(list); score.draw(list); outcome.draw(list); basis.draw(list);", source)
            self.assertNotIn("snapshot.menu_text().results.share_basis", source)

    def test_series_rows_remain_hidden_for_single_match(self):
        source = (ROOT / "scripts/acceptance/painter_visual/series_score_checks.h").read_text()
        self.assertIn("const bool visible = fixture.session.is_series() && details_visible;", source)
        self.assertEqual(source.count("Colour(), Vector2F::zero, !visible)"), 2)
        source = (ROOT / "scripts/acceptance/painter_visual/round_heading_checks.h").read_text()
        self.assertIn("const bool rows_visible = fixture.session.is_series() && details_visible;", source)
        self.assertEqual(source.count("Colour(), Vector2F::zero, !rows_visible)"), 2)


if __name__ == "__main__":
    unittest.main()
