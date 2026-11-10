import csv
import json
from pathlib import Path
import tempfile
import unittest

import analyze


SEATS = ("team", "tool", "share_of_painted", "credited_enemy_eliminations", "credited_enemy_projectile_deaths")
HEADER = list(analyze.MATCH_FIELDS) + [f"p{n}_{f}" for n in range(1, 5) for f in SEATS]


def row(stage, seed, seats, seconds=60.0, bot="match:hard:hard", schema=2):
    """seats: (tool, share, kills, deaths) per seat; teams alternate a, b, a, b."""
    values = {"schema_version": schema, "stage": stage, "seed": seed, "players": len(seats), "bot": bot,
              "requested_active_seconds": 60, "simulated_active_seconds": seconds}
    for n in range(1, 5):
        if n <= len(seats):
            tool, share, kills, deaths = seats[n - 1]
            team = "a" if n % 2 else "b"
        else:
            tool, share, kills, deaths, team = "none", 0, 0, 0, "none"
        values.update({f"p{n}_team": team, f"p{n}_tool": tool, f"p{n}_share_of_painted": share,
                       f"p{n}_credited_enemy_eliminations": kills, f"p{n}_credited_enemy_projectile_deaths": deaths})
    return values


def one_v_one(stage, seed, x, y, x_share, x_kills, y_kills):
    return row(stage, seed, [(x, x_share, x_kills, y_kills), (y, 1 - x_share, y_kills, x_kills)])


class BalanceAnalysis(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.dir = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, rows, name="run"):
        folder = self.dir / name
        folder.mkdir(parents=True, exist_ok=True)
        path = folder / "matches.csv"
        with path.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(stream, fieldnames=HEADER)
            writer.writeheader()
            writer.writerows(rows)
        return path

    def load(self, rows):
        matches, _ = analyze.read_matches([self.write(rows)])
        return matches

    def matrix(self, seeds=range(6)):
        """Sniper kills a lot and paints little against a sprayer, plus a sniper mirror."""
        rows = []
        for seed in seeds:
            jitter = (seed % 3 - 1) * 0.01
            rows.append(one_v_one("arena", seed, "sniper", "sprayer", 0.15 + jitter, 2 + seed % 2, 0))
            rows.append(one_v_one("arena", seed, "sprayer", "sniper", 0.85 - jitter, 0, 2))
            rows.append(one_v_one("arena", seed, "sniper", "sniper", 0.5, 1, 1))
        return rows

    def league(self, shares, seeds=range(6), noise=0.01):
        """Each listed pair, both sides, every seed. shares[(x, y)] is x's mean share against y."""
        rows = []
        for (x, y), share in shares.items():
            for seed in seeds:
                e = (seed % 3 - 1) * noise
                rows.append(one_v_one("arena", seed, x, y, share + e, 1 + seed % 2, 1))
                rows.append(one_v_one("arena", seed, y, x, 1 - share - e, 1, 1 + seed % 2))
        return rows

    # Marginal paint: sprayer 0.5, roller 0.6, mister 0.6, bucket 0.3.
    RANKED = {("roller", "sprayer"): 0.6, ("mister", "sprayer"): 0.6, ("sprayer", "bucket"): 0.7,
              ("roller", "bucket"): 0.7, ("mister", "bucket"): 0.7, ("mister", "roller"): 0.5}

    def paint_verdicts(self, profiles, **league):
        stated = {tool: {} for pair in self.RANKED for tool in pair}
        stated.update({t: {"paint": level} for t, level in profiles.items()})
        result = analyze.analyze(self.load(self.league(self.RANKED, **league)), stated)[0]
        return {t: (v["paint"]["verdict"], v["paint"]["note"]) for t, v in result["profiles"].items() if v}

    def test_t_quantiles_match_tables(self):
        self.assertAlmostEqual(analyze.two_sided(1), 12.706, places=3)
        self.assertAlmostEqual(analyze.two_sided(4), 2.776, places=3)
        self.assertAlmostEqual(analyze.two_sided(30), 2.042, places=3)
        self.assertAlmostEqual(analyze.two_sided(10 ** 7), 1.960, places=3)

    def test_a_pair_is_the_mean_of_both_orientations(self):
        rows = [one_v_one("arena", 0, "sniper", "roller", 0.2, 1, 0),
                one_v_one("arena", 0, "roller", "sniper", 0.6, 0, 3)]
        result = analyze.analyze(self.load(rows), {})[0]
        paint = result["table"][("paint", "arena", "sniper", "roller")]
        self.assertAlmostEqual(paint["mean"], (0.2 + 0.4) / 2)
        self.assertIsNone(paint["se"])  # one seed: no interval
        self.assertAlmostEqual(result["table"][("kills", "arena", "sniper", "roller")]["mean"], 2.0)
        self.assertEqual((result["swapped"], result["unswapped"], result["units"]), (1, 0, 1))

    def test_rates_are_per_simulated_minute(self):
        rows = [row("arena", 0, [("sprayer", 0.5, 3, 1), ("roller", 0.5, 1, 3)], seconds=30.0)]
        seats = self.load(rows)[0]["seats"]
        self.assertAlmostEqual(seats[0]["kills"], 6.0)
        self.assertAlmostEqual(seats[0]["deaths"], 2.0)

    def test_mirrors_stay_out_of_marginals_unless_asked(self):
        rows = self.matrix()
        without = analyze.analyze(self.load(rows), {})[0]["marginals"]["sniper"]["kills"]
        within = analyze.analyze(self.load(rows), {}, include_mirrors=True)[0]["marginals"]["sniper"]["kills"]
        self.assertAlmostEqual(without["mean"], (2.5 + 2) / 2)
        self.assertAlmostEqual(within["mean"], ((2.5 + 2) / 2 + 1) / 2)

    def test_a_pair_missing_its_swap_is_counted_once(self):
        rows = [one_v_one("arena", 0, "sniper", "roller", 0.2, 1, 0)]
        result = analyze.analyze(self.load(rows), {})[0]
        self.assertEqual((result["swapped"], result["unswapped"]), (0, 1))

    def test_shared_matches_count_with_their_covariance(self):
        # Sniper's and sprayer's shares are complements in every unit, so their
        # difference has twice sniper's standard error, not sqrt(2) times it.
        matches = self.load(self.matrix())
        result = analyze.analyze(matches, {})[0]
        strata, _ = analyze.pair_strata(matches)
        cells = analyze.cells_of(strata)
        weights = {t: analyze.tool_weights(cells, t, "pairs", False) for t in ("sniper", "sprayer")}
        diff = analyze.contrast(strata, "paint", analyze.minus(weights["sniper"], weights["sprayer"]))
        sniper = result["marginals"]["sniper"]["paint"]
        self.assertAlmostEqual(diff["se"], 2 * sniper["se"])
        self.assertAlmostEqual(diff["mean"], 2 * sniper["mean"] - 1)

    def test_side_effect_counts_each_swap_once(self):
        rows = []
        for seed in range(4):
            e = (seed % 3 - 1) * 0.01
            rows.append(one_v_one("arena", seed, "sniper", "sprayer", 0.25 + e, 1, 1))  # sniper on A
            rows.append(one_v_one("arena", seed, "sprayer", "sniper", 0.85, 1, 1))  # sniper on B: 0.15
        result = analyze.analyze(self.load(rows), {})[0]
        halves = [0.05 + (seed % 3 - 1) * 0.005 for seed in range(4)]
        sd = analyze.math.sqrt(analyze.sample_var(halves))
        self.assertEqual(result["swapped"], 4)
        self.assertAlmostEqual(result["side_effect"]["mean"], sum(halves) / 4)
        self.assertAlmostEqual(result["side_effect"]["half_width"], analyze.two_sided(3) * sd / 2)

    def test_a_pair_missing_its_swap_is_left_out_of_estimates(self):
        rows = [one_v_one("arena", seed, "sniper", "sprayer", 0.25 + seed * 0.001, 1, 1) for seed in range(4)]
        rows += [one_v_one("arena", seed, "sprayer", "sniper", 0.85, 1, 1) for seed in range(2)]
        result = analyze.analyze(self.load(rows), {})[0]
        self.assertEqual((result["swapped"], result["unswapped"]), (2, 2))
        self.assertAlmostEqual(result["marginals"]["sniper"]["paint"]["mean"], 0.20025)

    def test_no_spread_claims_no_interval(self):
        rows = [one_v_one("arena", seed, x, y, s, 1, 1) for seed in range(4)
                for x, y, s in (("sniper", "roller", 0.2), ("roller", "sniper", 0.8))]
        result = analyze.analyze(self.load(rows), {"sniper": {"paint": "least"}})[0]
        self.assertIsNone(result["marginals"]["sniper"]["paint"]["half_width"])
        self.assertEqual(result["profiles"]["sniper"]["paint"]["verdict"], "no interval")
        self.assertIn("(no interval)", analyze.render([result], [], ""))

    def test_rounding_is_not_spread(self):
        # Each pair's mean is exactly 0.3 + 0.05 * k, though its two shares move with the seed.
        rows = []
        for seed in range(4):
            e = 0.01 * (seed + 1)
            for k, (x, y) in enumerate((("sniper", "roller"), ("sniper", "sprayer"), ("roller", "sprayer"))):
                rows.append(one_v_one("arena", seed, x, y, 0.3 + 0.05 * k + e, 1, 1))
                rows.append(one_v_one("arena", seed, y, x, 1 - (0.3 + 0.05 * k - e), 1, 1))
        result = analyze.analyze(self.load(rows), {"roller": {"paint": "high"}})[0]
        self.assertIsNone(result["marginals"]["roller"]["paint"]["half_width"])
        self.assertEqual(result["profiles"]["roller"]["paint"]["verdict"], "no interval")

    def test_profiles_confirm_and_flag(self):
        rows = self.matrix()
        good = analyze.analyze(self.load(rows), {"sniper": {"kills": "most", "paint": "least"}})[0]
        self.assertEqual(good["profiles"]["sniper"]["kills"]["verdict"], "confirmed")
        self.assertEqual(good["profiles"]["sniper"]["paint"]["verdict"], "confirmed")
        bad = analyze.analyze(self.load(rows), {"sniper": {"paint": "most"}, "sprayer": {"kills": "high"}})[0]
        self.assertEqual(bad["profiles"]["sniper"]["paint"]["verdict"], "off")
        self.assertEqual(bad["profiles"]["sprayer"]["kills"]["verdict"], "off")

    def test_stated_order_holds(self):
        verdicts = self.paint_verdicts({"sprayer": "mid", "roller": "high", "mister": "high", "bucket": "low"})
        self.assertEqual({t: v for t, (v, _) in verdicts.items()},
                         {"sprayer": "confirmed", "roller": "confirmed", "mister": "confirmed", "bucket": "confirmed"})

    def test_a_reversed_order_is_off_for_both_tools(self):
        verdicts = self.paint_verdicts({"sprayer": "high", "roller": "mid"})
        self.assertEqual(verdicts["sprayer"], ("off", "below roller (mid)"))
        self.assertEqual(verdicts["roller"], ("off", "above sprayer (high)"))

    def test_mid_needs_evidence_and_is_off_at_an_extreme(self):
        self.assertEqual(self.paint_verdicts({"bucket": "mid"})["bucket"][0], "off")  # the outright least
        self.assertEqual(self.paint_verdicts({"sprayer": "mid"})["sprayer"][0], "confirmed")
        # Too little, too noisy data: nothing is distinguishable, so mid is not confirmed.
        weak = self.paint_verdicts({"sprayer": "mid"}, seeds=range(3), noise=0.3)
        self.assertEqual(weak["sprayer"][0], "consistent")

    def test_without_stated_peers_high_uses_the_median_other(self):
        self.assertEqual(self.paint_verdicts({"roller": "high"})["roller"],
                         ("confirmed", "against the median of the others"))
        self.assertEqual(self.paint_verdicts({"bucket": "high"})["bucket"][0], "off")

    def test_between(self):
        # Mister 0.6 is above sprayer 0.5 and level with roller 0.6: not shown to be between them.
        self.assertEqual(self.paint_verdicts({"mister": {"between": ["sprayer", "roller"]}})["mister"][0],
                         "consistent")
        self.assertEqual(self.paint_verdicts({"roller": {"between": ["sprayer", "bucket"]}})["roller"],
                         ("off", "above all of sprayer, bucket"))
        self.assertEqual(self.paint_verdicts({"sprayer": {"between": ["roller", "bucket"]}})["sprayer"][0],
                         "confirmed")

    def test_bad_profiles_are_refused(self):
        matches = self.load(self.matrix())
        for profiles, message in (({"sniper": {"paint": "lots"}}, "not a known level"),
                                  ({"bucket": {"paintt": "high"}}, "not a known measure"),
                                  ({"sniper": {"paint": {"between": ["sprayer"]}}}, "two or more other tools"),
                                  ({"sniper": {"paint": {"between": ["sprayer", "rollr"]}}, "sprayer": {}},
                                   "unknown tools")):
            with self.assertRaisesRegex(ValueError, message):
                analyze.analyze(matches, profiles)

    def test_shipped_profiles_judge_every_stated_level(self):
        profiles = json.loads(analyze.DEFAULT_PROFILES.read_text(encoding="utf-8"))["tools"]
        tools = sorted(profiles)
        pairs = [(x, y) for n, x in enumerate(tools) for y in tools[n + 1:]]
        shares = {pair: 0.4 + 0.02 * i for i, pair in enumerate(pairs)}
        result = analyze.analyze(self.load(self.league(shares)), profiles)[0]
        for tool, levels in profiles.items():
            for measure in levels:
                self.assertNotEqual(result["profiles"][tool][measure]["verdict"], "not in data", (tool, measure))

    def test_a_profiled_tool_missing_from_the_data_is_said(self):
        result = analyze.analyze(self.load(self.matrix()), {"roller": {"paint": "high"}})[0]
        self.assertEqual(result["profiles"]["roller"]["paint"]["verdict"], "not in data")
        self.assertIn("not in this data: roller", analyze.render([result], [], ""))

    def test_repeated_match_is_refused(self):
        rows = self.matrix()
        first = self.write(rows, "one")
        second = self.write(rows, "two")
        with self.assertRaisesRegex(ValueError, "repeats the match"):
            analyze.read_matches([first, second])

    def test_schema_one_and_empty_input_are_refused(self):
        with self.assertRaisesRegex(ValueError, "schema 1"):
            self.load([dict(one_v_one("arena", 0, "sniper", "roller", 0.2, 1, 0), schema_version=1)])
        with self.assertRaisesRegex(ValueError, "no match rows"):
            self.load([])

    def test_a_byte_order_mark_is_read(self):
        path = self.write(self.matrix())
        path.write_bytes(b"\xef\xbb\xbf" + path.read_bytes())
        matches, _ = analyze.read_matches([path])
        self.assertEqual(len(matches), 18)

    def test_groups_split_by_roster_and_bot(self):
        quad = [row("arena", seed, [("sprayer", 0.3, 1 + seed, 0), ("roller", 0.2, 0, 1),
                                    ("sprayer", 0.3, 0, 0), ("roller", 0.2, 0, 0)]) for seed in range(3)]
        other_bot = [dict(r, bot="skirmish") for r in self.matrix(seeds=range(2))]
        groups = analyze.analyze(self.load(self.matrix() + quad + other_bot), {})
        self.assertEqual([(g["players"], g["bot"], g["kind"]) for g in groups],
                         [(2, "match:hard:hard", "pairs"), (2, "skirmish", "pairs"),
                          (4, "match:hard:hard", "seats")])
        seats = groups[2]
        self.assertAlmostEqual(seats["marginals"]["sprayer"]["kills"]["mean"], 1.0)
        text = analyze.render([seats], [], "")
        self.assertIn("| Tool | arena |", text)
        self.assertRegex(text, r"\| roller \| 0\.20")

    def test_output_is_deterministic_and_refuses_the_tree(self):
        path = self.write(self.matrix())
        first, second = self.dir / "a", self.dir / "b"
        self.assertEqual(analyze.main([str(path), "--out", str(first)]), 0)
        self.assertEqual(analyze.main([str(path.parent), "--out", str(second)]), 0)
        self.assertEqual((first / "summary.json").read_bytes(), (second / "summary.json").read_bytes())
        refused = analyze.REPO_ROOT / "docs" / "balance-analyzer-test"
        self.assertEqual(analyze.main([str(path), "--out", str(refused)]), 2)
        self.assertFalse(refused.exists())

    def test_one_tool_rosters_stay_out_of_seat_marginals_unless_asked(self):
        mixed = [row("arena", seed, [("sprayer", 0.3 + seed * 0.01, 1, 0), ("roller", 0.2, 0, 1),
                                     ("sprayer", 0.3, 0, 0), ("roller", 0.2, 0, 0)]) for seed in range(3)]
        same = [row("arena", seed, [("sprayer", 0.25 + seed * 0.01, 1, 1)] * 4) for seed in range(3)]
        matches = self.load(mixed + same)
        without = analyze.analyze(matches, {})[0]["marginals"]["sprayer"]["paint"]["mean"]
        within = analyze.analyze(matches, {}, include_mirrors=True)[0]["marginals"]["sprayer"]["paint"]["mean"]
        self.assertAlmostEqual(without, 0.305)
        self.assertAlmostEqual(within, (0.305 + 0.26) / 2)

    @unittest.skipUnless(analyze.os.name == "nt", "Windows path spellings")
    def test_other_spellings_of_the_repository_are_refused(self):
        root = str(analyze.REPO_ROOT)
        share = "\\\\localhost\\" + root[0].lower() + "$" + root[2:]
        for spelling in ("\\\\?\\" + root + "\\docs\\x", root.upper() + "\\DOCS\\x", share + "\\docs\\x"):
            with self.assertRaisesRegex(ValueError, "outside out/", msg=spelling):
                analyze.refuse_tree_output(spelling)
        analyze.refuse_tree_output("\\\\?\\" + root + "\\out\\x")


if __name__ == "__main__":
    unittest.main()
