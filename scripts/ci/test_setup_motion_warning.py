"""Supplement the Windows motion probe with a guard against stale Ready fixtures."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "scripts/acceptance/painter_visual/setup_motion_checks.h").read_text()


class SetupMotionWarning(unittest.TestCase):
    def test_expected_warning_is_authored_before_input(self):
        start = SOURCE.index('// Neutral Ready is refused')
        end = SOURCE.index('const Frame endpoint', start)
        block = SOURCE[start:end]
        self.assertIn('L"Choose a team with Left/Right, then Ready."', block)
        self.assertIn('style.detail_font', block)
        self.assertIn('look.team_warning_position', block)
        self.assertIn('TextureFilter::linear', block)
        self.assertLess(block.index('choice_warning.draw(warning_list)'),
                        block.index('tap(0, GamepadButton::a)'))
        self.assertIn('no_team_warning.pixels != no_team.pixels', block)

    def test_whole_frame_equality_covers_arrival_settle_and_repeat(self):
        self.assertNotIn('require(no_team.pixels == frame().pixels', SOURCE)
        self.assertEqual(SOURCE.count('no_team_warning.pixels == frame().pixels'), 3)
        self.assertIn('tick(settle_time); require(no_team_warning.pixels == frame().pixels', SOURCE)
        self.assertEqual(SOURCE.count('tap(0, GamepadButton::a); require(no_team_warning.pixels == frame().pixels'), 2)


if __name__ == '__main__':
    unittest.main()
