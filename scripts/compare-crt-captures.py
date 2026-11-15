"""Compare the focused 720p CRT captures. Requires Pillow and numpy.
Usage: python scripts/compare-crt-captures.py OFF/captures ON/captures
"""
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image

off, on = map(Path, sys.argv[1:3])
expected = {f"crt-{players}p-{beat}-{mode}.ppm" for players in (2, 4)
            for beat in ("opening", "active", "reveal", "result")
            for mode in ("normal", "reduced")}
assert {p.name for p in off.glob('crt-*.ppm')} == expected
assert {p.name for p in on.glob('crt-*.ppm')} == expected
assert (off / 'crt-trial.csv').read_bytes() == (on / 'crt-trial.csv').read_bytes()
look = json.loads(Path('game/content/presentation.json').read_text())['opening']['glass']
y, x = np.mgrid[:720, :1280]
glass = ((x + .5 >= look['x'] * 1280) & (x + .5 <= (look['x'] + look['width']) * 1280)
         & (y + .5 >= look['y'] * 720) & (y + .5 <= (look['y'] + look['height']) * 720))
rows = []
for name in sorted(expected):
    a, b = (np.array(Image.open(path / name)) for path in (off, on))
    assert a.shape == b.shape == (720, 1280, 3), name
    changed = np.any(a != b, axis=2)
    count = int(changed.sum())
    # Independent 720p interiors spanning every score glyph, not a scene-only crop.
    if '-result-' in name:
        for left, right in ((200, 565), (710, 1055)):
            assert np.array_equal(a[175:245, left:right], b[175:245, left:right]), name + ': score card changed' 
    neutral = '-active-' in name or '-reduced' in name
    assert count == 0 if neutral else count > 0, (name, count)
    assert not np.any(changed & ~glass), f'{name}: effect escaped glass'
    ys, xs = np.where(changed)
    rows.append(dict(frame=name, changed=count, outside_glass=0,
        bounds=[int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())] if count else None))
print(json.dumps(dict(outcome='passed', frames=rows), indent=2))
