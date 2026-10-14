"""Render the editable ColourWars acoustic score; no oscillators or synth presets.

Requires Python 3.11+, NumPy and SciPy. Run from any working directory.
Every sounding event is a pitched/trimmed recording named in sources.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
from functools import lru_cache
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parent
RATE = 44100
PITCHES = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5,
           "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}


def midi(note: str) -> int:
    match = re.fullmatch(r"([A-G]#?)(-?\d+)", note)
    if not match:
        raise ValueError(f"Invalid scientific pitch: {note}")
    return 12 * (int(match[2]) + 1) + PITCHES[match[1]]


def read_sample(path: Path) -> np.ndarray:
    rate, raw = wavfile.read(path)
    scale = float(2 ** (np.iinfo(raw.dtype).bits - 1))
    data = raw.astype(np.float64) / scale
    if data.ndim == 2:
        data = data.mean(axis=1)
    data -= np.mean(data)
    peak = float(np.max(np.abs(data)))
    # Trim the pre-attack microphone noise, not the instrument's attack.
    indices = np.flatnonzero(np.abs(data) > peak * 0.004)
    first = max(0, int(indices[0]) - round(rate * 0.001))
    data = data[first:]
    if rate != RATE:
        divisor = math.gcd(rate, RATE)
        data = resample_poly(data, RATE // divisor, rate // divisor)
    # A short de-click and normalized source level make velocities comparable.
    fade = min(44, len(data))
    data[:fade] *= np.linspace(0, 1, fade)
    data /= max(np.max(np.abs(data)), 1e-12)
    return data.astype(np.float32)


def bank() -> dict:
    result = {"uke": {}, "piano": {}, "percussion": {}}
    for path in sorted((ROOT / "sources/ukulele/samples").glob("*.wav")):
        if path.stem in ("body-tap", "chuck"):
            name = path.stem.replace("-", "_")
            result["percussion"][name] = (read_sample(path), f"music.ukulele.{name}")
        else:
            result["uke"][midi(path.stem)] = (read_sample(path), f"music.ukulele.{path.stem}")
    for path in sorted((ROOT / "sources/vcsl").glob("Upright*.wav")):
        name = path.stem.split("_")[2]
        # VCSL uses C3=middle C. Our score consistently uses scientific C4.
        result["piano"][midi(name) + 12] = (read_sample(path), f"music.vcsl.piano_{name}")
    percussion = {
        "cajon_low_1": "Cajon_hit1_mp_rr1.wav", "cajon_low_2": "Cajon_hit1_mp_rr2.wav",
        "cajon_high_1": "Cajon_hit2_mp_rr1.wav", "cajon_high_2": "Cajon_hit2_mp_rr2.wav",
        "clap_1": "Clap_rr1.wav", "clap_2": "Clap_rr2.wav", "clap_3": "Clap_rr3.wav",
        "shaker_down_1": "Mid_ShakerDouble_Down_rr1.wav", "shaker_down_2": "Mid_ShakerDouble_Down_rr2.wav",
        "shaker_up_1": "Mid_ShakerDouble_Up_rr1.wav", "shaker_up_2": "Mid_ShakerDouble_Up_rr2.wav",
        "bongo_1": "BongoH_Hit1_v2_rr1_Mid.wav", "bongo_2": "BongoH_Hit1_v2_rr2_Mid.wav",
        "claves": "Claves1_Hit_v1_rr1_Mid.wav",
    }
    for name, filename in percussion.items():
        result["percussion"][name] = (read_sample(ROOT / "sources/vcsl" / filename), f"music.vcsl.{name}")
    return result


BANK: dict = {}


@lru_cache(maxsize=1024)
def pitched(instrument: str, note: int, length_frames: int) -> tuple[np.ndarray, str]:
    candidates = BANK[instrument]
    base = min(candidates, key=lambda candidate: abs(candidate - note))
    data, source_id = candidates[base]
    ratio = 2 ** ((note - base) / 12)
    positions = np.arange(0, len(data), ratio)
    rendered = np.interp(positions, np.arange(len(data)), data).astype(np.float32)
    # Note-off emulates a hand damping the string/key; no artificial sustain.
    release = round((0.12 if instrument == "uke" else 0.22) * RATE)
    n = min(len(rendered), length_frames + release)
    rendered = rendered[:n].copy()
    if n > length_frames:
        rendered[length_frames:] *= np.linspace(1, 0, n - length_frames) ** 1.8
    fade = min(128, len(rendered))
    rendered[-fade:] *= np.linspace(1, 0, fade)
    return rendered, source_id


class Session:
    def __init__(self, frames: int, loop: bool, seed: int, beat_seconds: float):
        self.audio = np.zeros((frames, 2), dtype=np.float32)
        self.loop = loop
        self.rng = np.random.default_rng(seed)
        self.beat = beat_seconds
        self.events = []
        self.sources = set()

    def add(self, data: np.ndarray, time: float, gain: float, pan: float, source: str,
            role: str, note: str | None = None):
        start = round(time * RATE)
        pan_gains = np.array([math.cos((pan + 1) * math.pi / 4),
                              math.sin((pan + 1) * math.pi / 4)], dtype=np.float32)
        sample = data[:, None] * (gain * pan_gains[None, :])
        if self.loop:
            # Wrap full decays across the seam. No fade-out/fade-in gap in music.
            cursor = 0
            while cursor < len(sample):
                offset = (start + cursor) % len(self.audio)
                count = min(len(sample) - cursor, len(self.audio) - offset)
                self.audio[offset:offset + count] += sample[cursor:cursor + count]
                cursor += count
        else:
            offset = max(0, start)
            count = min(len(sample) - max(0, -start), len(self.audio) - offset)
            if count > 0:
                self.audio[offset:offset + count] += sample[max(0, -start):max(0, -start) + count]
        self.sources.add(source)
        self.events.append({"frame": start, "role": role, "note": note,
                            "gain": round(gain, 5), "pan": pan, "source_id": source})

    def note(self, instrument: str, note: str, beat: float, length: float,
             gain: float, pan: float = 0, human: bool = True, role: str = "melody"):
        data, source = pitched(instrument, midi(note), round(length * self.beat * RATE))
        # Keep the opening attack at zero; all later variations remain deterministic.
        jitter = self.rng.uniform(-0.009, 0.009) if human and beat > 0 else 0
        velocity = self.rng.uniform(0.92, 1.06) if human else 1
        self.add(data, beat * self.beat + jitter, gain * velocity, pan, source, role, note)

    def hit(self, instrument: str, beat: float, gain: float, pan: float = 0):
        data, source = BANK["percussion"][instrument]
        jitter = self.rng.uniform(-0.007, 0.007) if beat > 0 else 0
        self.add(data, beat * self.beat + jitter, gain * self.rng.uniform(0.88, 1.08),
                 pan, source, "percussion")


def melody(session: Session, bars: list[str], start_bar: int, instrument: str,
           gain: float, pan: float, variation: int):
    for bar_index, bar in enumerate(bars):
        beat = (start_bar + bar_index) * 4
        consumed = 0.0
        tokens = bar.split()
        for token in tokens:
            name, duration = token.split(":")
            length = float(duration)
            if name != "r":
                accent = 1.05 if consumed == 0 else 0.94
                # Reprises change voicing, phrasing and orchestration, not just level.
                if variation == 2 and bar_index in (3, 7) and consumed >= 2:
                    name = re.sub(r"\d+$", lambda m: str(int(m[0]) + 1), name)
                session.note(instrument, name, beat + consumed, length * 0.83,
                             gain * accent, pan)
            consumed += length
        if abs(consumed - 4) > 1e-9:
            raise ValueError(f"Melody bar must contain 4 beats: {bar!r} ({consumed})")


def accompaniment(session: Session, chord: dict, bar: int, style: str, intensity: float,
                  section: int):
    start = bar * 4
    notes = chord["notes"]
    bass = chord["bass"]
    # Distinct rhythmic identities: sparse desk, tight offbeats, broad stomp, cross-rhythm.
    if style == "desk":
        bass_beats, chord_beats = [0], [1.5, 3]
        bass_gain, chord_gain = 0.13, 0.065
    elif style == "dash":
        bass_beats, chord_beats = [0, 2, 3.5], [0.5, 1.5, 2.75, 3.5]
        bass_gain, chord_gain = 0.19, 0.086
    elif style == "crown":
        bass_beats, chord_beats = [0, 2], [1, 3]
        bass_gain, chord_gain = 0.22, 0.10
    else:
        bass_beats, chord_beats = [0, 1.5, 3], [0.75, 1.5, 2.5, 3.5]
        bass_gain, chord_gain = 0.19, 0.077
    for index, beat in enumerate(bass_beats):
        note = bass if index % 2 == 0 else chord.get("bass_fifth", bass)
        session.note("piano", note, start + beat, 0.8 if style != "desk" else 1.4,
                     bass_gain * intensity, -0.05, role="bass")
    for index, beat in enumerate(chord_beats):
        voiced = notes if index % 2 == 0 else list(reversed(notes))
        for string, note in enumerate(voiced):
            # A genuine three-string strum, rather than identical simultaneous attacks.
            session.note("uke", note, start + beat + string * 0.022 / session.beat,
                         0.37 if style != "desk" else 0.9,
                         chord_gain * intensity * (0.9 if index % 2 else 1), -0.33,
                         role="strum")
    rr = bar % 2 + 1
    if style == "desk":
        session.hit("body_tap", start, 0.085 * intensity, 0.12)
        if section > 0:
            session.hit("chuck", start + 2.5, 0.055 * intensity, -0.2)
        if bar % 4 == 3:
            session.hit("shaker_up_1", start + 3.5, 0.045, 0.35)
        return
    session.hit(f"cajon_low_{rr}", start, 0.18 * intensity, 0.06)
    session.hit(f"cajon_low_{3-rr}", start + 2, 0.14 * intensity, 0.06)
    for beat in (1, 3):
        name = f"clap_{bar % 3 + 1}" if style == "crown" else f"cajon_high_{rr}"
        session.hit(name, start + beat, (0.10 if style == "crown" else 0.15) * intensity, 0.17)
    for half_beat in range(8):
        name = f"shaker_{'down' if half_beat % 2 == 0 else 'up'}_{rr}"
        session.hit(name, start + half_beat / 2, (0.038 if half_beat % 2 else 0.055) * intensity, 0.42)
    if style == "fan":
        for beat in (0.75, 1.5, 2.75, 3.5):
            session.hit(f"bongo_{rr}", start + beat, 0.12 * intensity, -0.23)
        for beat in ([0, 1.5, 3] if bar % 2 == 0 else [1, 2]):
            session.hit("claves", start + beat, 0.045 * intensity, 0.35)
    elif style == "dash" and bar % 2:
        session.hit("chuck", start + 2.5, 0.1 * intensity, -0.25)
    if bar % 8 == 7:
        # A quiet hand-played turnaround distinguishes eight-bar phrases.
        for fill, beat in enumerate((2.5, 3, 3.5, 3.75)):
            session.hit(f"bongo_{fill % 2 + 1}", start + beat,
                        (0.065 + fill * 0.015) * intensity, -0.18)


def wave(path: Path, audio: np.ndarray, loop: bool):
    data = np.clip(np.rint(audio * 32767), -32768, 32767).astype("<i2")
    pcm = data.tobytes()
    fmt = struct.pack("<HHIIHH", 1, 2, RATE, RATE * 4, 4, 16)
    chunks = b"fmt " + struct.pack("<I", len(fmt)) + fmt
    if loop:
        # MIDI unity note 60; inclusive loop end per RIFF smpl specification.
        smpl = struct.pack("<9I", 0, 0, round(1e9 / RATE), 60, 0, 0, 0, 1, 0)
        smpl += struct.pack("<6I", 0, 0, 0, len(data) - 1, 0, 0)
        chunks += b"smpl" + struct.pack("<I", len(smpl)) + smpl
    chunks += b"data" + struct.pack("<I", len(pcm)) + pcm
    path.write_bytes(b"RIFF" + struct.pack("<I", len(chunks) + 4) + b"WAVE" + chunks)


def finish(session: Session, stem: str, target_rms_db: float) -> dict:
    audio = session.audio.astype(np.float64)
    # Small room reflections use the recordings themselves, never synthesized tones.
    original = audio.copy()
    for delay, level in ((0.031, 0.07), (0.053, 0.045)):
        offset = round(delay * RATE)
        if session.loop:
            audio += np.roll(original[:, ::-1], offset, axis=0) * level
        else:
            audio[offset:] += original[:-offset, ::-1] * level
    if session.loop:
        audio -= audio.mean(axis=0)
    rms = math.sqrt(float(np.mean(audio * audio)))
    gain = min(10 ** (target_rms_db / 20) / rms, 10 ** (-1.5 / 20) / np.max(np.abs(audio)))
    audio *= gain
    if session.loop:
        # Correct only the endpoint discrepancy across 1.45 ms, below a whole period
        # of the lowest note; tails already wrap naturally through the loop boundary.
        delta = audio[-1] - audio[0]
        audio[-64:] -= np.linspace(0, 1, 64)[:, None] * delta
    else:
        # Export only the natural recorded decay, not the padded working buffer.
        active = np.flatnonzero(np.max(np.abs(audio), axis=1) > 10 ** (-66 / 20))
        audio = audio[:min(len(audio), int(active[-1]) + 45)]
        audio[:44] *= np.linspace(0, 1, 44)[:, None]
        audio[-882:] *= np.linspace(1, 0, 882)[:, None] ** 2
        # The de-click itself can leave an inaudible tail. Keep just 1 ms beyond
        # the final -60 dBFS sample, then taper that final millisecond. This also
        # leaves room for one MS-ADPCM block's unavoidable decoder padding.
        audible = np.flatnonzero(np.max(np.abs(audio), axis=1) >= 0.001)
        audio = audio[:min(len(audio), int(audible[-1]) + 45)]
        audio[-44:] *= np.linspace(1, 0, 44)[:, None]
    peak = float(np.max(np.abs(audio)))
    if peak > 10 ** (-1 / 20):
        raise ValueError(f"Peak exceeds -1 dBFS: {stem}")
    path = ROOT / "waves" / f"{stem}.wav"
    wave(path, audio, session.loop)
    event_path = ROOT / "scores" / f"{stem}.events.json"
    event_path.write_text(json.dumps({"sample_rate": RATE, "events": session.events}, indent=2) + "\n")
    return {"frames": len(audio), "seconds": round(len(audio) / RATE, 6),
            "peak_dbfs": round(20 * math.log10(peak), 3),
            "rms_dbfs": round(20 * math.log10(math.sqrt(float(np.mean(audio * audio)))), 3),
            "loop_seam_max_step": float(np.max(np.abs(audio[-1] - audio[0]))) if session.loop else None,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "source_ids": sorted(session.sources),
            "events": len(session.events), "master_gain_db": round(20 * math.log10(gain), 3)}


def render_track(track: dict, chords: dict) -> tuple[dict, dict]:
    bars = sum(len(track["sections"][part["section"]]["chords"]) for part in track["form"])
    nominal_frames = bars * 4 * 60 / track["bpm"] * RATE
    frames = round(nominal_frames / 500) * 500
    beat = frames / RATE / (bars * 4)
    session = Session(frames, True, track["seed"], beat)
    start = 0
    for section_index, part in enumerate(track["form"]):
        section = track["sections"][part["section"]]
        for index, chord_name in enumerate(section["chords"]):
            accompaniment(session, chords[chord_name], start + index, track["style"],
                          part["intensity"], section_index)
        melody(session, section["melody"], start, part["lead"], part["melody_gain"],
               0.17 if part["lead"] == "piano" else -0.08, part.get("variation", 0))
        # A sparse answer voice enters only in the contrasting/return sections.
        if "answer" in section and part.get("answer", False):
            melody(session, section["answer"], start, "piano" if part["lead"] == "uke" else "uke",
                   0.07, 0.30, 0)
        start += len(section["chords"])
    measured = finish(session, track["id"].replace(".", "_"), track["rms_dbfs"])
    measured.update({"title": track["title"], "bars": bars,
                     "nominal_bpm": track["bpm"], "rendered_bpm": round(60 / beat, 8)})
    cue = {"id": track["id"], "tier": track["tier"],
           "path": f"game/audio/paint_exe/music/waves/{track['id'].replace('.', '_')}.wav",
           "loop": True, "source_ids": measured["source_ids"], "description": track["description"],
           "production_source": "game/audio/paint_exe/music/score.json"}
    return cue, measured


def render_sting(sting: dict) -> tuple[dict, dict]:
    session = Session(round(sting["seconds"] * RATE), False, sting["seed"], 60 / sting["bpm"])
    for event in sting["notes"]:
        instrument, note, beat, duration, gain, pan = event
        session.note(instrument, note, beat, duration, gain, pan, human=False)
    for instrument, beat, gain, pan in sting.get("hits", []):
        session.hit(instrument, beat, gain, pan)
    measured = finish(session, sting["id"].replace(".", "_"), sting.get("rms_dbfs", -21))
    cue = {"id": sting["id"], "tier": 2,
           "path": f"game/audio/paint_exe/music/waves/{sting['id'].replace('.', '_')}.wav",
           "loop": False, "source_ids": measured["source_ids"], "description": sting["description"],
           "production_source": "game/audio/paint_exe/music/score.json"}
    if sting["id"].startswith("music.end_sting."):
        cue["cue_id"] = "music.end_sting"
    return cue, measured


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", help="Render exactly one cue id; registry still requires a full render")
    args = parser.parse_args()
    global BANK
    BANK = bank()
    (ROOT / "scores").mkdir(exist_ok=True)
    (ROOT / "waves").mkdir(exist_ok=True)
    score = json.loads((ROOT / "score.json").read_text(encoding="utf-8"))
    cues, measurements = [], {}
    for track in score["tracks"]:
        if args.only and args.only != track["id"]:
            continue
        cue, measured = render_track(track, score["chords"])
        cues.append(cue)
        measurements[cue["id"]] = measured
        print(f"{cue['id']}: {measured['seconds']:.2f}s, {measured['peak_dbfs']:.2f} dBFS peak")
    for sting in score["stings"]:
        if args.only and args.only != sting["id"]:
            continue
        cue, measured = render_sting(sting)
        cues.append(cue)
        measurements[cue["id"]] = measured
        print(f"{cue['id']}: {measured['seconds']:.2f}s, {measured['peak_dbfs']:.2f} dBFS peak")
    if not args.only:
        (ROOT / "cues.json").write_text(json.dumps({"version": 1, "cues": cues}, indent=2) + "\n")
        (ROOT / "measurements.json").write_text(json.dumps(measurements, indent=2) + "\n")


if __name__ == "__main__":
    main()
