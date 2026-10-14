"""Render the physical sound candidates from the retained, licensed recordings.

Requires Python 3, NumPy, SciPy and FFmpeg (only the chair MP3 decoder).
Run from any directory. No downloads, synthesis, randomness or runtime changes.
The JSON recipes are the editable production session; times refer to originals.
"""
from __future__ import annotations

import hashlib
import io
import json
import math
from pathlib import Path
import struct
import subprocess
import warnings

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, resample_poly, sosfiltfilt

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
RATE = 44100


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_source(row):
    path = REPO / row['path']
    if digest(path) != row['sha256']:
        raise ValueError(f"Source changed: {path}")
    if path.suffix == '.mp3':
        data = subprocess.run(['ffmpeg', '-v', 'error', '-i', str(path),
            '-f', 'wav', '-acodec', 'pcm_s16le', '-'], check=True,
            capture_output=True).stdout
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', wavfile.WavFileWarning)
            rate, raw = wavfile.read(io.BytesIO(data))
    else:
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', wavfile.WavFileWarning)
            rate, raw = wavfile.read(path)
    samples = raw.astype(np.float64)
    if raw.dtype.kind in 'iu':
        samples /= float(2 ** (raw.dtype.itemsize * 8 - 1))
    if samples.ndim > 1:
        samples = samples.mean(axis=1)
    return rate, samples


def piece(layer, cache):
    rate, source = cache[layer['source']]
    a, b = layer['seconds']
    if a < 0 or b > len(source) / rate + .001 or a >= b:
        raise ValueError(f'Invalid source excerpt {layer}')
    samples = source[round(a * rate):round(b * rate)].copy()
    samples -= samples.mean()
    # A speed change also changes pitch, as on a tape machine. No oscillator.
    speed = layer.get('speed', 1.0)
    effective_rate = round(rate * speed)
    common = math.gcd(effective_rate, RATE)
    samples = resample_poly(samples, RATE // common, effective_rate // common)
    for kind, cutoff in [('highpass', layer.get('highpass', 70)),
                         ('lowpass', layer.get('lowpass', 16000))]:
        if cutoff:
            samples = sosfiltfilt(butter(2, cutoff, btype=kind, fs=RATE,
                                       output='sos'), samples)
    if layer.get('reverse', False):
        samples = samples[::-1].copy()
    # Relative layer levels are independent of recording microphone gain.
    rms = np.sqrt(np.mean(samples ** 2))
    samples *= 10 ** (layer.get('level_db', -18) / 20) / max(rms, 1e-12)
    return samples


def trim(samples):
    # Find the actual gesture, leaving 1 ms ahead of it and 4 ms of decay.
    threshold = max(np.max(np.abs(samples)) * .006, .00001)
    active = np.flatnonzero(np.abs(samples) > threshold)
    if not len(active):
        raise ValueError('Silent candidate')
    return samples[max(0, active[0] - 44):min(len(samples), active[-1] + 177)]


def join_loop(samples, seconds):
    """Overlap tail/head; retain a continuous region instead of a silent seam."""
    overlap = min(round(seconds * RATE), len(samples) // 3)
    w = .5 - .5 * np.cos(np.linspace(0, np.pi, overlap))
    seam = samples[-overlap:] * (1 - w) + samples[:overlap] * w
    result = np.concatenate((samples[overlap:-overlap], seam))
    # Move the seam to the interior. The output boundary is consecutive samples
    # from the source, not unrelated ends; match the endpoint by a tiny taper.
    result = np.roll(result, len(result) // 2)
    n = min(128, len(result) // 10)
    delta = result[-1] - result[0]
    result[-n:] -= delta * np.linspace(0, 1, n) ** 2
    return result


def write_wave(path, samples, loop, channels=1):
    pcm = np.rint(samples * 32767).astype('<i2')
    if channels == 2:
        pcm = np.column_stack((pcm, pcm))
    fmt = struct.pack('<HHIIHH', 1, channels, RATE, RATE * 2 * channels, 2 * channels, 16)
    chunks = [(b'fmt ', fmt), (b'data', pcm.tobytes())]
    if loop:
        # WAV smpl uses an inclusive loop end. Forward loop, infinite repeat.
        smpl = struct.pack('<9I', 0, 0, round(1e9 / RATE), 60, 0, 0, 0, 1, 0)
        smpl += struct.pack('<6I', 0, 0, 0, len(pcm) - 1, 0, 0)
        chunks.append((b'smpl', smpl))
    payload = b'WAVE' + b''.join(tag + struct.pack('<I', len(data)) + data +
        (b'\0' if len(data) % 2 else b'') for tag, data in chunks)
    path.write_bytes(b'RIFF' + struct.pack('<I', len(payload)) + payload)
    return pcm


def main():
    sources = json.loads((HERE / 'sources.json').read_text())['sources']
    recipes = json.loads((HERE / 'recipes.json').read_text())['recipes']
    needed = {layer['source'] for recipe in recipes for layer in recipe['layers']}
    cache = {row['id']: read_source(row) for row in sources if row['id'] in needed}
    cues, provenance = [], []
    (HERE / 'masters').mkdir(exist_ok=True)
    for recipe in recipes:
        layers = [(round(layer.get('offset', 0) * RATE), piece(layer, cache))
                  for layer in recipe['layers']]
        samples = np.zeros(max(offset + len(data) for offset, data in layers))
        for offset, data in layers:
            samples[offset:offset + len(data)] += data
        loop = recipe.get('loop', False)
        if loop:
            samples = join_loop(samples, recipe.get('crossfade', .15))
        else:
            samples = trim(samples)
            n = min(44, len(samples) // 10)
            samples[:n] *= np.linspace(0, 1, n)
            n = min(round(recipe.get('tail_fade', .016) * RATE), len(samples) // 4)
            samples[-n:] *= np.linspace(1, 0, n) ** 2
        rms = np.sqrt(np.mean(samples ** 2))
        target = 10 ** (recipe.get('rms_db', -20) / 20)
        peak = 10 ** (recipe.get('peak_db', -3) / 20)
        samples *= min(target / max(rms, 1e-12), peak / np.max(np.abs(samples)))
        if not loop and 'trim_final_tail_dbfs' in recipe:
            threshold = 10 ** (recipe['trim_final_tail_dbfs'] / 20)
            active = np.flatnonzero(np.abs(samples) >= threshold)
            if len(active):
                samples = samples[:min(len(samples), active[-1] + 89)]
                n = min(88, len(samples) // 10)
                samples[-n:] *= np.linspace(1, 0, n)
        path = HERE / 'masters' / (recipe['id'].replace('.', '_') + '.wav')
        channels = recipe.get('channels', 1)
        pcm = write_wave(path, samples, loop, channels)
        source_ids = sorted({layer['source'] for layer in recipe['layers']})
        cue = {key: recipe[key] for key in ['id', 'cue_id', 'tier', 'description']}
        cue.update(path=path.relative_to(REPO).as_posix(), loop=loop,
                   source_ids=source_ids, recommended_gain=recipe.get('gain', 1.0),
                   production_source='game/audio/paint_exe/foley/recipes.json')
        cues.append(cue)
        provenance.append(dict(**cue, creator='ColourWars edit from credited recordings',
            license='CC0-1.0', release_use=True, redistribution=True,
            attribution='None required; creators credited voluntarily in sources.json',
            obtained='2026-10-14',
            recipe=recipe, modifications='Mono fold-down, source-time excerpt, recorded-layer mix, '
                'speed/filter edits as listed, DC removal, polyphase resampling, RMS/peak mastering; '
                + ('continuous overlap loop and smpl region' if loop else 'onset trim and short edge fades'),
            export=f'PCM signed 16-bit little-endian {channels}-channel WAV, 44100 Hz',
            sha256=digest(path), frames=len(pcm), duration_seconds=len(pcm)/RATE,
            peak_dbfs=20 * math.log10(max(np.max(np.abs(pcm.astype(float))) / 32768, 1e-12)),
            rms_dbfs=20 * math.log10(max(np.sqrt(np.mean((pcm.astype(float)/32768)**2)), 1e-12)),
            loop_start=0 if loop else None, loop_end_inclusive=len(pcm)-1 if loop else None,
            review_status='Technical production record; current author acceptance is in ../acceptance.json; in-game acceptance remains ROAD#3'))
    (HERE / 'cues.json').write_text(json.dumps({'version': 1, 'cues': cues}, indent=2)+'\n')
    (HERE / 'PROVENANCE.json').write_text(json.dumps({'version': 1, 'assets': provenance}, indent=2)+'\n')
    print(f'Rendered {len(cues)} foley candidates from {len(needed)} retained recordings.')


if __name__ == '__main__':
    main()
