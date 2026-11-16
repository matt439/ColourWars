"""Check shipped Options row spacing from its actual spritefont metrics.

This is data/geometry coverage, not a replacement for the native painter.
"""
import copy
import json
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
LABELS = ('Resolution', 'Fullscreen', 'Vsync', 'Team palette', 'Controls / audio',
          'Graphics', 'Restore defaults', 'Apply', 'Back')


def font_heights(name):
    data = (ROOT / 'game/content/fonts' / (name + '.spritefont')).read_bytes()
    if data[:8] != b'DXTKfont':
        raise ValueError('Expected the shipped DirectXTK spritefont format')
    count, = struct.unpack_from('<I', data, 8)
    spacing, = struct.unpack_from('<f', data, 12 + count * 32)
    heights = {}
    for index in range(count):
        character, left, top, right, bottom, _x, y, _advance = struct.unpack_from(
            '<I4i3f', data, 12 + index * 32)
        # The engine suppresses the one-pixel space glyph's bearings.
        heights[chr(character)] = (0 if character == 32 and right-left <= 1 and bottom-top <= 1
                                   else max(spacing, bottom-top+y))
    return heights


def layout_findings(presentation):
    main = presentation['main_menu']
    heights = font_heights(main['fonts']['options'])
    text_height = max(heights[c] for label in LABELS for c in label)
    padding = main['setup_focus_padding']['y']
    step = main['options_spacing']['y']
    shadow = main['setup_shadow_offset']
    footer = presentation['menu_style']['chrome']['footer_y']
    paper_bottom = (footer + main['options_box_size']['y']) / 2
    # Same conservative card envelope used by the native settings-menu probe.
    last_card_bottom = (main['options_position']['y'] + step * (len(LABELS)-1)
                        + text_height + padding + main['art']['keyline_width'] + 2)
    findings = []
    if step <= text_height + 2*padding + shadow:
        findings.append('Options row focus cards and shadows need positive separation')
    if last_card_bottom > paper_bottom:
        findings.append('Last Options card escapes paper')
    if main['options_position']['y'] + step * (len(LABELS)-1) >= main['options_status_position']['y']:
        findings.append('Options status must follow the last row anchor')
    return findings


class OptionsLayout(unittest.TestCase):
    def setUp(self):
        self.presentation = json.loads((ROOT / 'game/content/presentation.json').read_text())

    def test_shipped_nine_rows_have_clearance(self):
        self.assertEqual(layout_findings(self.presentation), [])

    def test_previous_zero_gap_layout_is_rejected(self):
        cramped = copy.deepcopy(self.presentation)
        cramped['main_menu']['options_spacing']['y'] = 72
        self.assertIn('Options row focus cards and shadows need positive separation', layout_findings(cramped))

    def test_spacing_cannot_push_back_out_of_paper(self):
        overflowing = copy.deepcopy(self.presentation)
        overflowing['main_menu']['options_spacing']['y'] = 90
        self.assertIn('Last Options card escapes paper', layout_findings(overflowing))


if __name__ == '__main__':
    unittest.main()
