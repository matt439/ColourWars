"""Run ROAD#13's fixed Release balance design against the real umbrella acceptance probe."""
import argparse
import concurrent.futures
import csv
from dataclasses import asdict, dataclass
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time

import analyze
import matrix
from job_process import start_process


STAGES = ('crossfeed', 'close_quarters', 'press_tower', 'turbulence_slab_trial', 'sluice_works')
OPPONENTS = ('sprayer', 'sniper', 'roller', 'mister', 'bucket')
VARIANTS = ('standard', 'endless_paint')
SEEDS = range(1000, 1012)
AREAS = ('paint', 'opposing_paint', 'paintable')
COUNTS = ('seed', 'umbrella_seat', 'ticks', 'kills', 'deaths', 'open', 'alive', 'hits', 'breaks')


@dataclass(frozen=True)
class Cell:
    stage: str
    opponent: str
    variant: str
    sub: str

    @property
    def name(self):
        return f'{self.variant}-{self.stage}-{self.opponent}-{self.sub}'


def cells():
    profiles = [(tool, 'grenade') for tool in OPPONENTS] + [('sprayer', 'wall')]
    return [Cell(stage, opponent, variant, sub)
            for variant in VARIANTS for stage in STAGES for opponent, sub in profiles]


def read_rows(log_path, cell):
    expected = {(seed, seat) for seed in SEEDS for seat in (1, 2)}
    metadata = asdict(cell)
    fields = set(metadata) | set(AREAS) | set(COUNTS)
    rows = {}
    for index, line in enumerate(log_path.read_text(encoding='utf-8-sig').splitlines(), 1):
        if not line.startswith('CW-CANOPY '):
            continue
        try:
            tokens = line.split()[1:]
            row = dict(token.split('=', 1) for token in tokens)
            if len(tokens) != len(row) or set(row) != fields:
                raise ValueError('duplicate, missing or unknown measurement field')
            if any(row[k] != v for k, v in metadata.items()):
                raise ValueError('cell identity differs from the requested game')
            for key in COUNTS:
                row[key] = int(row[key])
                if row[key] < 0:
                    raise ValueError(f'negative {key}')
            for key in AREAS:
                row[key] = float(row[key])
                if not math.isfinite(row[key]) or row[key] < 0:
                    raise ValueError(f'invalid {key}')
            painted = row['paint'] + row['opposing_paint']
            if not 0 < painted <= row['paintable'] * (1 + 1e-6):
                raise ValueError('paint needs a positive scored denominator and must fit the arena')
            if not 3599 <= row['ticks'] <= 3601 or not row['open'] <= row['alive'] <= row['ticks']:
                raise ValueError('invalid one-minute round or canopy uptime')
            if row['breaks'] > row['hits']:
                raise ValueError('more observed breaks than canopy hit-point losses')
            identity = row['seed'], row['umbrella_seat']
            if identity not in expected or identity in rows:
                raise ValueError(f'unexpected or repeated seed/seat {identity}')
            rows[identity] = row
        except ValueError as error:
            raise ValueError(f'{log_path}:{index}: {error}') from error
    missing = expected - rows.keys()
    if missing:
        raise ValueError(f'{log_path}: missing seed/seat matches {sorted(missing)}')
    return [rows[k] for k in sorted(rows)]


def seed_means(matches):
    # read_rows required both seat orders for every stage. Keep the same seed
    # together across the five fixed arenas, including for the sub comparison.
    samples = {seed: [] for seed in SEEDS}
    for row in matches:
        samples[row['seed']].append({
            'paint': row['paint'] / (row['paint'] + row['opposing_paint']),
            'kills': row['kills'] * 3600 / row['ticks'],
            'deaths': row['deaths'] * 3600 / row['ticks'],
        })
    if any(len(rows) != len(STAGES) * 2 for rows in samples.values()):
        raise ValueError('summary needs both seat orders on all five stages per seed')
    return {seed: {name: analyze.mean([r[name] for r in rows]) for name in analyze.MEASURES}
            for seed, rows in samples.items()}


def estimates(means, baseline=None):
    units = [{'tested': means[seed]} for seed in SEEDS]
    weights = {'tested': 1.0}
    if baseline is not None:
        for unit, seed in zip(units, SEEDS):
            unit['baseline'] = baseline[seed]
        weights['baseline'] = -1.0
    return {name: analyze.contrast({'seeds': units}, name, weights) for name in analyze.MEASURES}


def summarize(rows):
    groups = {}
    for row in rows:
        key = row['variant'], row['opponent'], row['sub']
        groups.setdefault(key, []).append(row)
    means = {key: seed_means(matches) for key, matches in groups.items()}
    results = []
    for key, matches in groups.items():
        variant, opponent, sub = key
        results.append({
            'variant': variant, 'opponent': opponent, 'sub': sub,
            'matches': len(matches), 'pairs': len(matches) // 2, 'independent_seeds': len(SEEDS),
            'estimates': estimates(means[key]), 'matches_with_blocks': sum(r['hits'] > 0 for r in matches),
            'observed_hits': sum(r['hits'] for r in matches),
            'observed_breaks': sum(r['breaks'] for r in matches),
            'open_ticks': sum(r['open'] for r in matches), 'alive_ticks': sum(r['alive'] for r in matches),
        })
        if sub == 'wall':
            results[-1]['vs_grenade'] = estimates(means[key], means[variant, opponent, 'grenade'])
    return results


def pin_inputs(exe, content):
    repo = matrix.REPO
    policy = repo / 'cmake/runtime_assets.cmake'
    source_root = repo / 'game/content'
    source = matrix.runtime_pins(source_root, policy)
    staged = matrix.runtime_pins(content, policy)
    for path, expected in source.items():
        counterpart = str((content / Path(path).relative_to(source_root)).resolve())
        if staged[counterpart] != expected:
            raise ValueError(f'Staged content differs from source: {counterpart}')

    manifest = json.loads((content / 'manifest.json').read_text(encoding='utf-8-sig'))
    levels = [name for entry in manifest['assets'] if entry['kind'] == 'level' for name in entry['names']]
    random_pool = [stage for stage in levels if
                   json.loads((content / 'levels' / (stage + '.json')).read_text(encoding='utf-8-sig'))
                   .get('include_in_random', True)]
    if sorted(random_pool) != sorted(STAGES):
        raise ValueError(f'Random stage membership differs from the qualification design: {random_pool}')

    build_info = content.parent / 'generated/build-info.txt'
    if 'Configuration: Release' not in build_info.read_text(encoding='utf-8-sig').splitlines():
        raise ValueError('This qualification requires Release game content and libraries')
    sources = repo / 'scripts/acceptance'
    inputs = [exe, content / 'ColourWars.exe', content / 'colourwars_game.lib', build_info, policy,
              Path(__file__), Path(matrix.__file__), Path(analyze.__file__), Path(__file__).with_name('job_process.py'),
              sources / 'umbrella.cpp', *sorted((sources / 'umbrella').glob('*.*')),
              sources / 'bot_navigation/fixtures.h', sources / 'paint_surfaces/fixtures.h',
              *sorted((sources / 'support').glob('*.h')), *sorted(content.glob('*.dll'))]
    for suffix in ('*.cpp', '*.h'):
        inputs.extend(sorted((repo / 'game').rglob(suffix)))
    return source | staged | {str(path.resolve()): matrix.digest(path) for path in inputs}


def run_cell(cell, directory, exe, content, timeout, control):
    directory.mkdir()
    args = [str(exe), str(content), '--balance', cell.stage, cell.opponent, cell.variant,
            cell.sub, str(SEEDS.start), str(len(SEEDS))]
    info = {'arguments': args, 'outcome': 'running'}
    receipt = directory / 'job.json'
    matrix.write_json(receipt, info)
    started = time.monotonic()
    environment = os.environ.copy()
    environment['PATH'] = str(content) + os.pathsep + environment.get('PATH', '')
    environment['COLOURWARS_MUTE'] = '1'
    log_path = directory / 'run.log'
    try:
        with log_path.open('xb') as log:
            with control.lock:
                if control.cancelled.is_set():
                    raise RuntimeError('Cancelled before starting job')
                process = start_process(args, log, environment)
                control.active.add(process)
            try:
                info['exit_code'] = process.wait(timeout=timeout)
            except BaseException:
                process.terminate()
                process.wait()
                raise
            finally:
                with control.lock:
                    control.active.discard(process)
        if info['exit_code'] != 0:
            raise RuntimeError(f'{cell.name} failed; inspect {log_path}')
        rows = read_rows(log_path, cell)
        info.update(outcome='passed', log_sha256=matrix.digest(log_path), matches=len(rows))
        return rows
    except BaseException as error:
        info.update(outcome='failed', reason=str(error))
        raise
    finally:
        info['seconds'] = round(time.monotonic() - started, 3)
        matrix.write_json(receipt, info)


def run_cells(out, exe, content, parallel, timeout):
    control = matrix.RunControl()
    results = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=parallel) as pool:
        pending = {pool.submit(run_cell, cell, out / cell.name, exe, content, timeout, control): cell
                   for cell in cells()}
        try:
            for future in concurrent.futures.as_completed(pending):
                cell = pending[future]
                results[cell] = future.result()
                print(f'[{len(results)}/{len(pending)}] {cell.name}', flush=True)
        except BaseException:
            control.cancel()
            for future in pending:
                future.cancel()
            raise
    first = cells()[0]
    repeated = run_cell(first, out / 'identity-repeat', exe, content, timeout, control)
    if repeated != results[first]:
        raise ValueError(f'Identity repeat changed a match on {first.name}')
    return [row for cell in cells() for row in results[cell]]


def interval(stat, scale=1):
    mean = stat['mean'] * scale
    if stat['half_width'] is None:
        return f'{mean:.2f} (no interval)'
    return f"{mean:.2f} ± {stat['half_width'] * scale:.2f}"


def write_report(out, results):
    lines = ['# Umbrella bot qualification', '',
             'The 1,440 one-minute Normal 1v1 rounds cover the five Random arenas. '
             'Each row has 120 rounds forming 60 side-swapped pairs. Within each of 12 seeds, '
             'the estimate averages both seat orders on all five fixed arenas. The unadjusted 95% '
             'Student intervals use variation among the 12 seed averages. Paint is the share of '
             'all painted area held at time up. Eliminations and deaths are per live match minute.',
             '', 'Canopy hits and breaks are observed lower bounds from simulation steps. '
             'Open time uses the umbrella player’s living ticks. Both players carry grenades '
             'except in the dedicated paint wall comparison.', '']
    for heading, sub in [('Umbrella with a grenade', 'grenade'), ('Umbrella with a paint wall', 'wall')]:
        lines.extend([f'## {heading}', '',
                      '| Paint rule | Opponent | Paint % | Eliminations/min | Deaths/min | Rounds with a block / 120 | Observed hits | Observed breaks | Open time % |',
                      '|---|---|---:|---:|---:|---:|---:|---:|---:|'])
        for result in results:
            if result['sub'] != sub:
                continue
            stats = result['estimates']
            uptime = (f"{100 * result['open_ticks'] / result['alive_ticks']:.1f}"
                      if result['alive_ticks'] else '—')
            lines.append(f"| {result['variant']} | {result['opponent']} | {interval(stats['paint'], 100)} | "
                         f"{interval(stats['kills'])} | {interval(stats['deaths'])} | "
                         f"{result['matches_with_blocks']} | {result['observed_hits']} | "
                         f"{result['observed_breaks']} | {uptime} |")
        lines.append('')
    lines.extend(['The following differences use the same 12 seeds for each loadout against '
                  'Sprayer + Grenade. Each seed contributes one difference after averaging over '
                  'the same five arenas and both seats. Positive values are higher with the paint wall.',
                  '', '| Paint rule | Paint difference (percentage points) | Eliminations/min difference | Deaths/min difference |',
                  '|---|---:|---:|---:|'])
    for group in results:
        if group['sub'] == 'wall':
            change = group['vs_grenade']
            lines.append(f"| {group['variant']} | {interval(change['paint'], 100)} | "
                         f"{interval(change['kills'])} | {interval(change['deaths'])} |")
    lines.extend(['', 'The trial measures these bots under these rules. '
                      'Player balance and sound remain attended judgments.'])
    (out / 'summary.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, required=True, help='Release umbrella acceptance executable')
    parser.add_argument('--content', type=Path, required=True, help='staged x64-release/game directory')
    parser.add_argument('--output', type=Path, required=True, help='new evidence directory under out/')
    parser.add_argument('--parallel', type=int, choices=range(1, 17), default=4)
    parser.add_argument('--timeout', type=int, default=600, help='deadline per 24-match cell in seconds')
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    exe, content = args.runner.resolve(), args.content.resolve()
    out = matrix.output_path(args.output)
    pins = pin_inputs(exe, content)
    out.mkdir(parents=True, exist_ok=False)
    receipt = out / 'run-info.json'
    info = {'outcome': 'running', 'cells': [asdict(cell) for cell in cells()], 'seeds': list(SEEDS),
            'duration_seconds': 60, 'difficulty': 'normal', 'parallel': args.parallel, 'timeout': args.timeout,
            'device_free': True, 'human_acceptance': False, 'input_pins': pins,
            'started_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
    matrix.write_json(receipt, info)
    try:
        rows = run_cells(out, exe, content, args.parallel, args.timeout)
        matrix.verify_pins(pins)
        results = summarize(rows)
        with (out / 'matches.csv').open('x', encoding='utf-8', newline='') as output:
            writer = csv.DictWriter(output, fieldnames=[*asdict(cells()[0]), *COUNTS, *AREAS])
            writer.writeheader()
            writer.writerows(rows)
        matrix.write_json(out / 'summary.json', results)
        write_report(out, results)
        info.update(outcome='passed', matches=len(rows), identity_repeat=cells()[0].name,
                    outputs={name: matrix.digest(out / name) for name in ('matches.csv', 'summary.json', 'summary.md')})
    except BaseException as error:
        info.update(outcome='failed', reason=str(error))
        raise
    finally:
        info['completed_utc'] = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())
        matrix.write_json(receipt, info)
    print(f"PASS {len(rows)} matches; {out / 'summary.md'}")
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        sys.exit(str(error))
