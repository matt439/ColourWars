"""Render the editable interface/match arrangements using recorded CC0 samples.

Run from any directory: python game/audio/paint_exe/interface/render.py
Requires numpy, scipy and ffmpeg (for reading source WAV formats).
No oscillators, generated noise, legacy audio or runtime bank inputs are used.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import wave

import numpy as np
from scipy.signal import butter, sosfilt, resample_poly

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
RATE = 44100


def read_sample(path: str) -> np.ndarray:
    result = subprocess.run([
        "ffmpeg", "-v", "error", "-i", str(ROOT / path), "-f", "f32le",
        "-acodec", "pcm_f32le", "-ac", "1", "-ar", str(RATE), "-",
    ], check=True, capture_output=True)
    return np.frombuffer(result.stdout, dtype="<f4").astype(np.float64)


def excerpt(raw: np.ndarray, layer: dict) -> np.ndarray:
    start = round(layer.get("start", 0) * RATE)
    end = round(layer.get("end", len(raw) / RATE) * RATE)
    signal = raw[start:end].copy()
    if not len(signal):
        raise ValueError(f"Empty source excerpt: {layer}")
    active = np.flatnonzero(np.abs(signal) > max(1e-5, np.max(np.abs(signal)) * .006))
    if not len(active):
        raise ValueError(f"Silent source excerpt: {layer}")
    signal = signal[max(0, active[0] - 44):min(len(signal), active[-1] + 89)]
    ratio = 2 ** (layer.get("semitones", 0) / 12)
    signal = resample_poly(signal, 10000, round(10000 * ratio))
    signal -= np.mean(signal)
    if "lowpass" in layer:
        signal = sosfilt(butter(2, layer["lowpass"], fs=RATE, output="sos"), signal)
    if layer.get("reverse"):
        signal = signal[::-1].copy()
    signal /= max(np.max(np.abs(signal)), 1e-8)
    # A short attack removes edit clicks without erasing the recorded transient.
    fade_in = min(len(signal) // 2, 44)
    fade_out = min(len(signal) // 2, round(layer.get("fade_out", .018) * RATE))
    signal[:fade_in] *= np.linspace(0, 1, fade_in)
    signal[-fade_out:] *= np.linspace(1, 0, fade_out)
    return signal * layer.get("gain", 1)


def write_wave(path: Path, signal: np.ndarray, loop: bool) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = np.rint(np.clip(signal, -.99, .99) * 32767).astype("<i2")
    with wave.open(str(path), "wb") as output:
        output.setparams((1, 2, RATE, len(pcm), "NONE", "not compressed"))
        output.writeframes(pcm.tobytes())
    if loop:
        # RIFF smpl end is inclusive. Full-length cycle, infinite forward loop.
        payload = struct.pack("<15I", 0, 0, round(1e9 / RATE), 60, 0, 0, 0, 1, 0,
                              0, 0, 0, len(pcm) - 1, 0, 0)
        data = bytearray(path.read_bytes())
        data += b"smpl" + struct.pack("<I", len(payload)) + payload
        struct.pack_into("<I", data, 4, len(data) - 8)
        path.write_bytes(data)


def main() -> None:
    project = json.loads((HERE / "project.json").read_text(encoding="utf-8"))
    samples = {key: read_sample(value["path"]) for key, value in project["samples"].items()}
    cues = []
    for cue in project["cues"]:
        layers = [(layer, excerpt(samples[layer["sample"]], layer)) for layer in cue["layers"]]
        length = round(cue["duration"] * RATE) if cue.get("loop") else max(
            round(layer.get("at", 0) * RATE) + len(signal) for layer, signal in layers)
        output = np.zeros(length, dtype=np.float64)
        for layer, signal in layers:
            begin = round(layer.get("at", 0) * RATE)
            if cue.get("loop"):
                np.add.at(output, (np.arange(len(signal)) + begin) % length, signal)
            else:
                output[begin:begin + len(signal)] += signal
        if not cue.get("loop"):
            active = np.flatnonzero(np.abs(output) > np.max(np.abs(output)) * .001)
            output = output[max(0, active[0] - 44):min(len(output), active[-1] + 89)]
            n = min(88, len(output) // 2)
            output[:n] *= np.linspace(0, 1, n)
            output[-n:] *= np.linspace(1, 0, n)
        output *= 10 ** (cue["peak_dbfs"] / 20) / np.max(np.abs(output))
        if not cue.get("loop"):
            # Trim after mastering as well: very quiet decorative cues otherwise
            # retain an inaudible head/tail that was audible at the working gain.
            audible = np.flatnonzero(np.abs(output) >= .001)
            output = output[max(0, audible[0] - 44):min(len(output), audible[-1] + 89)]
            n = min(44, len(output) // 2)
            output[:n] *= np.linspace(0, 1, n)
            output[-n:] *= np.linspace(1, 0, n)
        path = HERE / "waves" / f"{cue['id']}.wav"
        write_wave(path, output, cue.get("loop", False))
        cues.append({
            "id": cue["id"], "cue_id": cue["id"], "tier": cue["tier"],
            "path": path.relative_to(ROOT).as_posix(), "loop": cue.get("loop", False),
            "source_ids": sorted({project["samples"][layer["sample"]]["source_id"]
                                  for layer in cue["layers"]}),
            "description": cue["description"],
            "production_source": "game/audio/paint_exe/interface/project.json",
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        })
    (HERE / "cues.json").write_text(json.dumps({"version": 1, "cues": cues}, indent=2) + "\n", encoding="utf-8")
    print(f"Rendered {len(cues)} interface/match waves")


if __name__ == "__main__":
    main()
