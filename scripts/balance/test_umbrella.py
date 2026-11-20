import itertools
import tempfile
import unittest
from pathlib import Path

import umbrella


STAGES = ('crossfeed', 'close_quarters', 'press_tower', 'turbulence_slab_trial', 'sluice_works')
PROFILES = (('sprayer', 'grenade'), ('sniper', 'grenade'), ('roller', 'grenade'),
            ('mister', 'grenade'), ('bucket', 'grenade'), ('sprayer', 'wall'))
RULES = ('standard', 'endless_paint')


def sample(cell, seed, seat):
    # The two seat orders finish with different totals. The statistic is the
    # average of their individual shares, so pooling the raw areas is detectable.
    total = 100 if seat == 1 else 300
    share = (0.1 if seat == 1 else 0.5) + (0.2 if cell.sub == 'wall' else 0)
    if cell.variant == 'endless_paint':
        share += 0.1
    paint = round(share * total)
    return dict(stage=cell.stage, opponent=cell.opponent, variant=cell.variant, sub=cell.sub,
                seed=seed, umbrella_seat=seat, ticks=3600, paint=paint, opposing_paint=total-paint,
                paintable=600, kills=seat, deaths=3-seat, open=100, alive=3000,
                hits=2 if seat == 1 else 0, breaks=0)


def cell_rows(cell):
    return [sample(cell, seed, seat) for seed in range(1000, 1012) for seat in (1, 2)]


def line(row):
    return 'CW-CANOPY ' + ' '.join(f'{key}={value}' for key, value in row.items())


class UmbrellaQualification(unittest.TestCase):
    def test_design_covers_every_requested_cell_once(self):
        cells = umbrella.cells()
        expected = {umbrella.Cell(stage, opponent, rule, sub)
                    for stage, (opponent, sub), rule in itertools.product(STAGES, PROFILES, RULES)}
        self.assertEqual(set(cells), expected)
        self.assertEqual(len(cells), 60)

    def read(self, cell, lines):
        with tempfile.TemporaryDirectory() as temp:
            log = Path(temp) / 'job.log'
            log.write_text('\n'.join(lines) + '\n', encoding='utf-8')
            return umbrella.read_rows(log, cell)

    def test_reads_both_seats_for_each_seed_among_probe_messages(self):
        cell = umbrella.cells()[0]
        lines = [line(row) for row in reversed(cell_rows(cell))]
        rows = self.read(cell, ['Starting headless match', *lines, 'PASS umbrella'])
        self.assertEqual({(r['seed'], r['umbrella_seat']) for r in rows},
                         set(itertools.product(range(1000, 1012), (1, 2))))

    def test_refuses_wrong_cell_and_incomplete_or_repeated_seat_pair(self):
        cell = umbrella.Cell('crossfeed', 'sprayer', 'standard', 'grenade')
        lines = [line(row) for row in cell_rows(cell)]
        bad_runs = {
            'missing': lines[:-1],
            'duplicate instead of missing': [*lines[:-1], lines[0]],
            'unexpected seed': [lines[0].replace('seed=1000', 'seed=999'), *lines[1:]],
            'different sub': [lines[0].replace('sub=grenade', 'sub=wall'), *lines[1:]],
            'different stage': [lines[0].replace('stage=crossfeed', 'stage=press_tower'), *lines[1:]],
        }
        for reason, bad_run in bad_runs.items():
            with self.subTest(reason=reason), self.assertRaises(ValueError):
                self.read(cell, bad_run)

    def test_refuses_malformed_or_unphysical_measures(self):
        cell = umbrella.cells()[0]
        fixture = cell_rows(cell)
        for field, value in [('paint', 'nan'), ('opposing_paint', 'inf'), ('hits', -1), ('ticks', 1),
                             ('umbrella_seat', 3), ('open', 3001), ('paintable', 1)]:
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.read(cell, [line(dict(fixture[0], **{field: value})),
                                 *(line(row) for row in fixture[1:])])
        with self.assertRaises(ValueError):
            self.read(cell, [line(fixture[0]).replace(' paint=', ' missing='),
                             *(line(row) for row in fixture[1:])])

    def test_summary_preserves_rule_and_sub_and_averages_each_seat_pair(self):
        rows = [row for cell in umbrella.cells() for row in cell_rows(cell)]
        summary = umbrella.summarize(rows)
        groups = {(g['variant'], g['opponent'], g['sub']): g for g in summary}
        self.assertEqual(len(groups), 12)
        for (variant, _, sub), group in groups.items():
            self.assertEqual((group['matches'], group['pairs']), (120, 60))
            self.assertEqual(group['independent_seeds'], 12)
            self.assertEqual((group['matches_with_blocks'], group['observed_hits'], group['observed_breaks']),
                             (60, 120, 0))
            measures = group['estimates']
            expected = 0.3 + (0.2 if sub == 'wall' else 0) + (0.1 if variant == 'endless_paint' else 0)
            self.assertAlmostEqual(measures['paint']['mean'], expected)
            self.assertAlmostEqual(measures['kills']['mean'], 1.5)
            self.assertAlmostEqual(measures['deaths']['mean'], 1.5)
            # Seat order varies, yet the seed's paired mean is identical each time.
            self.assertIsNone(measures['paint']['half_width'])

    def test_interval_keeps_the_same_seed_together_across_stages(self):
        rows = [row for cell in umbrella.cells() for row in cell_rows(cell)]
        for row in rows:
            total = row['paint'] + row['opposing_paint']
            shift = (1 if row['seed'] % 2 else -1) * (total // 100)
            row['paint'] += shift
            row['opposing_paint'] -= shift
        for group in umbrella.summarize(rows):
            # Six of the 12 shared seeds are at each sign, regardless of stage.
            # Pooling a seed's stages gives SE=0.01/sqrt(11) and 11 degrees of freedom.
            self.assertAlmostEqual(group['estimates']['paint']['half_width'], 0.006636, places=6)
            if group['sub'] == 'wall':
                # Matched wall and grenade rows share the same seed effect exactly.
                for measure, difference in [('paint', 0.2), ('kills', 0), ('deaths', 0)]:
                    delta = group['vs_grenade'][measure]
                    self.assertAlmostEqual(delta['mean'], difference)
                    self.assertIsNone(delta['half_width'])
            else:
                self.assertNotIn('vs_grenade', group)


if __name__ == '__main__':
    unittest.main()
