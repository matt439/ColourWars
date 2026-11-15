"""Compare the focused 720p and optional 1080p CRT captures. Requires Pillow and numpy.
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
if list(off.glob('crt-1080-*.ppm')):
    expected |= {name.replace('crt-', 'crt-1080-', 1) for name in list(expected)}
assert {p.name for p in off.glob('crt-*.ppm')} == expected
assert {p.name for p in on.glob('crt-*.ppm')} == expected
assert (off / 'crt-trial.csv').read_bytes() == (on / 'crt-trial.csv').read_bytes()
look = json.loads(Path('game/content/presentation.json').read_text())['opening']['glass']
rows = []
for name in sorted(expected):
    a, b = (np.array(Image.open(path / name)) for path in (off, on))
    height, width = (1080, 1920) if 'crt-1080-' in name else (720, 1280)
    assert a.shape == b.shape == (height, width, 3), name
    y, x = np.mgrid[:height, :width]
    glass = ((x + .5 >= look['x'] * width) & (x + .5 <= (look['x'] + look['width']) * width)
             & (y + .5 >= look['y'] * height) & (y + .5 <= (look['y'] + look['height']) * height))
    changed = np.any(a != b, axis=2)
    count = int(changed.sum())
    # Independent score-card interiors, scaled from authored 720p geometry.
    if '-result-' in name:
        for left, right in ((200, 565), (710, 1055)):
            region = np.s_[int(175*height/720):int(245*height/720),
                           int(left*width/1280):int(right*width/1280)]
            assert np.array_equal(a[region], b[region]), name + ': score card changed'
    neutral = '-active-' in name or '-reduced' in name
    assert count == 0 if neutral else count > 0, (name, count)
    assert not np.any(changed & ~glass), f'{name}: effect escaped glass'
    ys, xs = np.where(changed)
    rows.append(dict(frame=name, changed=count, outside_glass=0,
        bounds=[int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())] if count else None))
print(json.dumps(dict(outcome='passed', frames=rows), indent=2))
