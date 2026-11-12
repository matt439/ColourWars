import re
import unittest

import paint_colour_vision as pcv


class Grid:
    """A pixel accessor shaped like Pillow's PixelAccess, from rows of colours."""

    def __init__(self, rows):
        self.rows = rows

    def __getitem__(self, xy):
        x, y = xy
        return self.rows[y][x]


class ColourMathChecks(unittest.TestCase):
    def test_matrices_match_the_team_palette_case(self):
        source = (pcv.REPO / "scripts/acceptance/team_palette.cpp").read_text(encoding="utf-8")
        block = source[source.index("matrices[3][3][3]"):source.index("double rgb[3]")]
        numbers = [float(n) for n in re.findall(r"-?\d+\.\d+", block)]
        ours = [v for name in ("protan", "deutan", "tritan") for row in pcv.MATRICES[name] for v in row]
        self.assertEqual(numbers, ours)

    def test_ciede2000_reference_pair(self):
        # Sharma, Wu and Dalal (2005), pair 1.
        self.assertAlmostEqual(pcv.ciede2000((50.0, 2.6772, -79.7751), (50.0, 0.0, -82.7485)), 2.0425, places=4)

    def test_pair_results_match_the_roadmap_record(self):
        pairs = pcv.shipped_pairs()
        worst = {key: min((pcv.difference(a, b, v), v) for v in pcv.VISIONS) for key, (a, b) in pairs.items()}
        self.assertEqual((round(worst["purple_gold"][0], 1), worst["purple_gold"][1]), (43.6, "tritan"))
        self.assertEqual((round(worst["pink_green"][0], 1), worst["pink_green"][1]), (27.3, "deutan"))
        self.assertEqual(round(worst["blue_orange"][0], 1), 58.1)


class PaintChecks(unittest.TestCase):
    A, B = (123, 31, 162), (255, 194, 10)
    SKY, EDGE = (201, 211, 217), (0, 0, 0)

    def test_shades_are_paint_and_greys_are_not(self):
        self.assertEqual(pcv.shade_of(self.A, self.A), ("white", 0.0))
        self.assertEqual(pcv.shade_of(self.B, (0xE8, 0xB0, 0x09))[0], "black")
        self.assertEqual(pcv.shade_of(self.A, (0x9B, 0x55, 0xB8))[0], "white")
        self.assertIsNone(pcv.shade_of(self.A, self.SKY))
        self.assertIsNone(pcv.shade_of(self.B, (255, 255, 255)))

    def test_strip_contacts_skip_own_shades_and_solid_swatches(self):
        light = pcv.apply_shade(self.A, ("white", 0.3))
        rows = [[self.SKY] * 12 for _ in range(12)]
        rows[1][1:5] = [self.A, light, self.A, self.B]
        rows[2][1:5] = [self.EDGE] * 4
        for y in range(3, 11):
            rows[y][3:11] = [self.B] * 8  # a swatch, not paint
        contacts = pcv.paint_contacts(Grid(rows), (12, 12), (self.A, self.B))
        count_a, edges_a = contacts[0]
        self.assertEqual(count_a, 3)
        self.assertNotIn(light, edges_a)
        self.assertEqual(edges_a[("team", 1, ("white", 0.0))], 1)
        self.assertEqual(contacts[1][0], 1)

    def test_recolour_follows_the_other_team(self):
        rows = [[self.SKY, self.A, self.B, self.SKY]]
        contacts = pcv.paint_contacts(Grid(rows), (4, 1), (self.A, self.B))
        pairs = pcv.shipped_pairs()
        report = pcv.measure(contacts, pairs["pink_green"])
        deutan = report["visions"]["deutan"]
        self.assertAlmostEqual(deutan["pair"], 27.3, places=1)
        self.assertEqual(deutan["team_a"]["edges"], 2)
        self.assertEqual(report["worst_pair"][0], "deutan")


if __name__ == "__main__":
    unittest.main()
