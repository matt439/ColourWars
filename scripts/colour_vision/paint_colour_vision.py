"""Measure painted terrain in a capture under simulated colour vision (REV#34).

The `team_palette` acceptance case judges each team pair as two flat colours
against the surface colours the levels author. This script asks the same
question of real frames: it finds the paint in a capture (the picture is drawn
unfiltered, so paint pixels are exactly the team colours or the wet band's and
shadows' mixes of them with white or black), and measures, under
normal vision and the Machado, Oliveira and Fernandes (2009) full-severity
protan, deutan and tritan simulations, CIEDE2000 between

- the two teams (the `team_palette` pair bar is 40), and
- each team's paint and every pixel it touches on screen (the surround bar is
  15), weighted by how many edges they share.

`--as-pair` recolours the captured paint with another shipped pair before
measuring, so all six pairs can be judged on the same terrain. The colour math
is a copy of `scripts/acceptance/team_palette.cpp`; its test pins the two
against each other. Standard library, plus Pillow to read the captures (the
colour math and its test need only the standard library); nothing is written unless
`--json` names a file.
"""

import argparse
import json
import math
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
PRESENTATION = REPO / "game/content/presentation.json"

VISIONS = ("normal", "protan", "deutan", "tritan")
MATRICES = {
    "protan": ((0.152286, 1.052583, -0.204868), (0.114503, 0.786281, 0.099216), (-0.003882, -0.048116, 1.051998)),
    "deutan": ((0.367322, 0.860646, -0.227968), (0.280085, 0.672501, 0.047413), (-0.011820, 0.042940, 0.968881)),
    "tritan": ((1.255528, -0.076749, -0.178779), (-0.078411, 0.930809, 0.147602), (0.004733, 0.691367, 0.303900)),
}
PAIR_BAR = 40.0
SURROUND_BAR = 15.0
# A run of one team colour this solid and this large is a swatch or a card,
# not a strip of paint: strips are at most a few pixels deep.
BLOCK_SIDE = 8
BLOCK_FILL = 0.8
# PaintTile's wet band lifts the tint toward white by at most wet_highlight
# (0.45 in tuning.json); shadows over paint scale it toward black.
MAX_HIGHLIGHT = 0.6
MIN_SHADOW = 0.5


def _linear(channel):
    c = channel / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def perceived(rgb, vision):
    """CIELAB (D65) of an 8-bit sRGB colour as seen under `vision`."""
    lin = [_linear(c) for c in rgb]
    if vision != "normal":
        m = MATRICES[vision]
        lin = [min(1.0, max(0.0, m[r][0] * lin[0] + m[r][1] * lin[1] + m[r][2] * lin[2])) for r in range(3)]
    x = (0.4124564 * lin[0] + 0.3575761 * lin[1] + 0.1804375 * lin[2]) / 0.95047
    y = 0.2126729 * lin[0] + 0.7151522 * lin[1] + 0.0721750 * lin[2]
    z = (0.0193339 * lin[0] + 0.1191920 * lin[1] + 0.9503041 * lin[2]) / 1.08883

    def f(t):
        return t ** (1.0 / 3.0) if t > 216.0 / 24389.0 else (24389.0 / 27.0 * t + 16.0) / 116.0

    return (116.0 * f(y) - 16.0, 500.0 * (f(x) - f(y)), 200.0 * (f(y) - f(z)))


def ciede2000(first, second):
    l1, a1_, b1 = first
    l2, a2_, b2 = second
    pow7 = lambda v: v ** 7
    mean_c = (math.hypot(a1_, b1) + math.hypot(a2_, b2)) / 2.0
    g = 0.5 * (1.0 - math.sqrt(pow7(mean_c) / (pow7(mean_c) + pow7(25.0))))
    a1, a2 = (1.0 + g) * a1_, (1.0 + g) * a2_
    c1, c2 = math.hypot(a1, b1), math.hypot(a2, b2)

    def hue(a, b):
        h = math.degrees(math.atan2(b, a))
        return h + 360.0 if h < 0.0 else h

    h1, h2 = hue(a1, b1), hue(a2, b2)
    dh = 0.0
    if c1 * c2 != 0.0:
        dh = h2 - h1
        if dh > 180.0:
            dh -= 360.0
        elif dh < -180.0:
            dh += 360.0
    d_l, d_c = l2 - l1, c2 - c1
    d_h = 2.0 * math.sqrt(c1 * c2) * math.sin(math.radians(dh / 2.0))
    mean_l, mean_cp = (l1 + l2) / 2.0, (c1 + c2) / 2.0
    mean_h = h1 + h2
    if c1 * c2 != 0.0:
        if abs(h1 - h2) <= 180.0:
            mean_h = (h1 + h2) / 2.0
        else:
            mean_h = (h1 + h2 + 360.0) / 2.0 if h1 + h2 < 360.0 else (h1 + h2 - 360.0) / 2.0
    t = (1.0 - 0.17 * math.cos(math.radians(mean_h - 30.0)) + 0.24 * math.cos(math.radians(2.0 * mean_h))
         + 0.32 * math.cos(math.radians(3.0 * mean_h + 6.0)) - 0.20 * math.cos(math.radians(4.0 * mean_h - 63.0)))
    d_theta = 30.0 * math.exp(-(((mean_h - 275.0) / 25.0) ** 2))
    r_c = 2.0 * math.sqrt(pow7(mean_cp) / (pow7(mean_cp) + pow7(25.0)))
    s_l = 1.0 + 0.015 * (mean_l - 50.0) ** 2 / math.sqrt(20.0 + (mean_l - 50.0) ** 2)
    s_c, s_h = 1.0 + 0.045 * mean_cp, 1.0 + 0.015 * mean_cp * t
    r_t = -math.sin(math.radians(2.0 * d_theta)) * r_c
    return math.sqrt((d_l / s_l) ** 2 + (d_c / s_c) ** 2 + (d_h / s_h) ** 2 + r_t * (d_c / s_c) * (d_h / s_h))


def difference(first, second, vision):
    return ciede2000(perceived(first, vision), perceived(second, vision))


def hex_rgb(text):
    text = text.lstrip("#")
    return tuple(int(text[i:i + 2], 16) for i in (0, 2, 4))


def shipped_pairs(path=PRESENTATION):
    """{key: (team_a_rgb, team_b_rgb)} from presentation.json."""
    palette = json.loads(path.read_text(encoding="utf-8"))["team_palette"]
    named = {entry["name"]: hex_rgb(entry["colour"]) for entry in palette["colours"]}
    return {pair["key"]: (named[pair["team_a"]], named[pair["team_b"]]) for pair in palette["pairs"]}


def _components(mask_points):
    """Four-connected components of a set of (x, y) points."""
    remaining = set(mask_points)
    while remaining:
        seed = remaining.pop()
        stack, component = [seed], [seed]
        while stack:
            x, y = stack.pop()
            for nxt in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if nxt in remaining:
                    remaining.remove(nxt)
                    stack.append(nxt)
                    component.append(nxt)
        yield component


def shade_of(team, colour):
    """The paint's own shades: a wet-band highlight (toward white) or a shadow
    (toward black) of `team`, as `team` mixed with white by `s` or scaled by
    `k`. Returns the (white, s) or (black, k) mix, or None for anything else."""
    if colour == team:
        return ("white", 0.0)
    gap = max(range(3), key=lambda i: 255 - team[i])
    if 255 - team[gap] > 0:
        s = (colour[gap] - team[gap]) / (255 - team[gap])
        if 0.0 < s <= MAX_HIGHLIGHT and all(abs(colour[i] - (team[i] + (255 - team[i]) * s)) <= 2 for i in range(3)):
            return ("white", s)
    top = max(range(3), key=lambda i: team[i])
    if team[top] > 0:
        k = colour[top] / team[top]
        if MIN_SHADOW <= k < 1.0 and all(abs(colour[i] - team[i] * k) <= 2 for i in range(3)):
            return ("black", k)
    return None


def apply_shade(team, shade):
    kind, amount = shade
    if kind == "white":
        return tuple(round(c + (255 - c) * amount) for c in team)
    return tuple(round(c * amount) for c in team)


def paint_contacts(pixels, size, team_colours):
    """Per team: paint pixel count and {neighbour: shared edges}.

    Paint is a team colour or one of its shades (see shade_of). A neighbour
    that is the other team's paint is keyed as ("team", index, shade) so a
    recolour can follow it; any other neighbour is keyed by its colour. Solid
    blocks (swatches, cards) are left out; see BLOCK_SIDE.
    """
    width, height = size
    classified = {}

    def classify(colour):
        if colour not in classified:
            classified[colour] = None
            for index, team in enumerate(team_colours):
                shade = shade_of(team, colour)
                if shade is not None:
                    classified[colour] = (index, shade)
                    break
        return classified[colour]

    found = [[] for _ in team_colours]
    for y in range(height):
        for x in range(width):
            hit = classify(pixels[x, y][:3])
            if hit is not None:
                found[hit[0]].append((x, y))
    result = []
    for index, points in enumerate(found):
        kept = []
        for component in _components(points):
            xs = [p[0] for p in component]
            ys = [p[1] for p in component]
            w, h = max(xs) - min(xs) + 1, max(ys) - min(ys) + 1
            if w >= BLOCK_SIDE and h >= BLOCK_SIDE and len(component) >= BLOCK_FILL * w * h:
                continue
            kept.extend(component)
        contacts = {}
        for x, y in kept:
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < width and 0 <= ny < height:
                    other = pixels[nx, ny][:3]
                    hit = classify(other)
                    if hit is not None and hit[0] == index:
                        continue
                    key = ("team", hit[0], hit[1]) if hit is not None else other
                    contacts[key] = contacts.get(key, 0) + 1
        result.append((len(kept), contacts))
    return result


def measure(capture_contacts, as_pair):
    """Measure captured paint (from paint_contacts) recoloured to `as_pair`."""
    report = {"visions": {}}
    for vision in VISIONS:
        entry = {"pair": difference(as_pair[0], as_pair[1], vision),
                 "lightness": abs(perceived(as_pair[0], vision)[0] - perceived(as_pair[1], vision)[0])}
        for index, label in enumerate(("team_a", "team_b")):
            team = as_pair[index]
            count, contacts = capture_contacts[index]
            total = sum(contacts.values())
            below, worst = 0, None
            for key, edges in contacts.items():
                neighbour = apply_shade(as_pair[key[1]], key[2]) if key[0] == "team" else key
                delta = difference(team, neighbour, vision)
                if delta < SURROUND_BAR:
                    below += edges
                if edges >= 0.01 * total and (worst is None or delta < worst[1]):
                    worst = ("%02X%02X%02X" % neighbour, delta, edges / total if total else 0.0)
            entry[label] = {"pixels": count, "edges": total,
                            "below_surround_bar": below / total if total else 0.0,
                            "nearest_common_neighbour": worst}
        report["visions"][vision] = entry
    worst_pair = min(report["visions"].items(), key=lambda item: item[1]["pair"])
    report["worst_pair"] = (worst_pair[0], worst_pair[1]["pair"])
    report["worst_below_surround_bar"] = max(
        report["visions"][v][t]["below_surround_bar"] for v in VISIONS for t in ("team_a", "team_b"))
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("captures", nargs="+", type=Path, help="unfiltered PNG frames of painted terrain")
    parser.add_argument("--pair", required=True, help="the shipped pair the captures were taken with")
    parser.add_argument("--as-pair", action="append", default=None,
                        help="recolour the paint to this pair before measuring (repeatable; 'all' for every pair)")
    parser.add_argument("--json", type=Path, help="also write the full report here")
    args = parser.parse_args(argv)

    from PIL import Image  # only the image half needs Pillow

    pairs = shipped_pairs()
    captured_pair = pairs[args.pair]
    targets = args.as_pair or [args.pair]
    if "all" in targets:
        targets = list(pairs)
    full = {}
    for capture in args.captures:
        with Image.open(capture) as image:
            rgb = image.convert("RGB")
            contacts = paint_contacts(rgb.load(), rgb.size, captured_pair)
        for key in targets:
            report = measure(contacts, pairs[key])
            full.setdefault(str(capture), {})[key] = report
            n = report["visions"]["normal"]
            print(f"{capture.name} as {key}: paint {n['team_a']['pixels']}+{n['team_b']['pixels']} px; "
                  f"pair worst {report['worst_pair'][1]:.1f} ({report['worst_pair'][0]}); "
                  f"edges below {SURROUND_BAR:.0f}: up to {100 * report['worst_below_surround_bar']:.1f}%")
            for vision in VISIONS:
                v = report["visions"][vision]
                parts = []
                for label in ("team_a", "team_b"):
                    near = v[label]["nearest_common_neighbour"]
                    near_text = f"{near[0]} {near[1]:.1f} ({100 * near[2]:.0f}% of edges)" if near else "-"
                    parts.append(f"{label[-1].upper()} below {100 * v[label]['below_surround_bar']:.1f}%, nearest {near_text}")
                print(f"  {vision:7} pair {v['pair']:5.1f} dL {v['lightness']:5.1f} | " + " | ".join(parts))
    if args.json:
        args.json.write_text(json.dumps(full, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
