"""Validate the POL#22 sources and build an isolated audition wave bank.

    python scripts/audio/audio_pipeline.py validate
    python scripts/audio/audio_pipeline.py build

Python standard library only; build also needs ffmpeg and the checked-in
XWBTool. The live content bank and game are deliberately not changed here.
"""

from __future__ import annotations

import argparse
from array import array
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys


REPOSITORY = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = REPOSITORY / "game/audio/paint_exe/cues.json"
MAX_PEAK = 10 ** (-1 / 20)
SILENCE_THRESHOLD = 10 ** (-60 / 20)
# XAudio2 requires power-of-two sample counts per ADPCM block. FFmpeg requires
# power-of-two *byte* blocks instead, so encode overlapping windows then retain
# the first 512 samples of each independently predicted block.
# https://learn.microsoft.com/en-us/windows/win32/xaudio2/adpcm-overview
ADPCM_FRAMES_PER_BLOCK = 512


class InvalidAudio(ValueError):
    """An authoring input does not meet the declared production contract."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise InvalidAudio(message)


def sha256(path: Path) -> str:
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def repository_file(value: str, root: Path = REPOSITORY) -> Path:
    require(isinstance(value, str) and bool(value), "expected a repository-relative path")
    relative = Path(value)
    require(not relative.is_absolute() and not relative.drive,
            f"path must be repository-relative: {value}")
    resolved = (root / relative).resolve()
    require(resolved.is_relative_to(root.resolve()), f"path escapes repository: {value}")
    require(resolved.is_file(), f"file is missing: {value}")
    return resolved


def riff_chunks(data: bytes) -> dict[bytes, bytes]:
    require(len(data) >= 12 and data[:4] == b"RIFF" and data[8:12] == b"WAVE",
            "expected a RIFF WAVE file")
    require(struct.unpack_from("<I", data, 4)[0] + 8 == len(data),
            "RIFF length does not match file length")
    found = {}
    offset = 12
    while offset < len(data):
        require(offset + 8 <= len(data), "truncated RIFF chunk header")
        tag, length = struct.unpack_from("<4sI", data, offset)
        end = offset + 8 + length
        require(end + (length & 1) <= len(data), "truncated RIFF chunk")
        require(tag not in found, f"duplicate RIFF chunk: {tag!r}")
        found[tag] = data[offset + 8:end]
        offset = end + (length & 1)
    require(b"fmt " in found and b"data" in found, "missing WAV format or samples")
    return found


def loop_region(chunks: dict[bytes, bytes], frames: int) -> tuple[int, int] | None:
    if b"smpl" not in chunks:
        return None
    body = chunks[b"smpl"]
    require(len(body) >= 36, "truncated smpl header")
    count = struct.unpack_from("<I", body, 28)[0]
    require(count == 1 and len(body) >= 60, "expected exactly one authored smpl loop")
    _, kind, start, end, fraction, play_count = struct.unpack_from("<6I", body, 36)
    require(kind == 0 and fraction == 0 and play_count == 0,
            "expected an infinite forward loop on whole sample frames")
    require(start <= end < frames, "smpl loop falls outside the wave")
    # RIFF's end is inclusive; XWB uses a start plus a frame count.
    return start, end - start + 1


def inspect_pcm(path: Path, looping: bool) -> dict:
    chunks = riff_chunks(path.read_bytes())
    require(len(chunks[b"fmt "]) >= 16, "truncated PCM format")
    tag, channels, rate, byte_rate, alignment, bits = struct.unpack_from(
        "<HHIIHH", chunks[b"fmt "])
    require((tag, rate, bits) == (1, 44100, 16),
            f"expected integer PCM 44100 Hz / 16-bit, got {tag=} {rate=} {bits=}")
    require(channels in (1, 2) and alignment == channels * 2 and byte_rate == rate * alignment,
            "invalid PCM channel count or byte alignment")
    payload = chunks[b"data"]
    require(bool(payload) and len(payload) % alignment == 0, "empty or partial PCM frame")
    frames = len(payload) // alignment
    region = loop_region(chunks, frames)
    require((region is not None) == looping, "loop flag and authored smpl region disagree")
    samples = array("h")
    samples.frombytes(payload)
    if sys.byteorder != "little":
        samples.byteswap()
    peak = max(abs(value) for value in samples) / 32768
    require(0 < peak <= MAX_PEAK, f"peak {20 * math.log10(peak) if peak else '-inf'} dBFS exceeds -1 dBFS or wave is silent")
    threshold = 32768 * SILENCE_THRESHOLD
    first = next((i for i, value in enumerate(samples) if abs(value) >= threshold), None)
    require(first is not None, "wave is entirely below -60 dBFS")
    last = len(samples) - 1 - next(i for i, value in enumerate(reversed(samples)) if abs(value) >= threshold)
    onset_ms = (first // channels) * 1000 / rate
    tail_ms = (frames - 1 - last // channels) * 1000 / rate
    rms = math.sqrt(sum(value * value for value in samples) / len(samples)) / 32768
    warnings = []
    if onset_ms >= 10:
        warnings.append("onset is at least 10 ms below -60 dBFS; trim or audition the intentional attack")
    if not looping and tail_ms > 20:
        warnings.append("tail has more than 20 ms below -60 dBFS; check for unnecessary padding")
    seam = None
    if region:
        start, length = region
        seam = max(abs(samples[start * channels + c] - samples[(start + length - 1) * channels + c])
                   for c in range(channels)) / 32768
        if seam > 0.01:
            warnings.append("loop seam step exceeds 1% full scale; audition repeated joins")
    return {"channels": channels, "rate": rate, "frames": frames,
            "seconds": frames / rate, "peak_dbfs": 20 * math.log10(peak),
            "rms_dbfs": 20 * math.log10(rms), "onset_ms": onset_ms, "tail_ms": tail_ms,
            "loop_start": region[0] if region else 0,
            "loop_length": region[1] if region else 0,
            "loop_seam_step": seam, "sha256": sha256(path), "warnings": warnings}


def load_sources(path: Path, root: Path) -> dict[str, dict]:
    raw = json.loads(path.read_text(encoding="utf-8-sig"))
    require(isinstance(raw.get("sources"), list), "sources.json must contain a sources list")
    sources = {}
    for source in raw["sources"]:
        identifier = source.get("id")
        require(isinstance(identifier, str) and identifier and identifier not in sources,
                f"missing or duplicate source id: {identifier}")
        for field in ("creator", "url", "license", "license_url", "obtained", "attribution"):
            require(isinstance(source.get(field), str) and bool(source[field].strip()),
                    f"source {identifier}: missing {field}")
        require(source.get("redistribution") is True and source.get("release_use") is True,
                f"source {identifier}: source redistribution and release use must be explicitly permitted")
        original = repository_file(source.get("path"), root)
        require(source.get("sha256") == sha256(original), f"source {identifier}: SHA-256 differs")
        sources[identifier] = source
    return sources


def validate(manifest_path: Path, sources_path: Path, root: Path = REPOSITORY) -> tuple[dict, dict]:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    require(manifest.get("version") == 1 and isinstance(manifest.get("cues"), list),
            "expected version 1 manifest with cues list")
    require(bool(manifest["cues"]), "cue manifest is empty")
    sources = load_sources(sources_path, root)
    result = {"manifest": str(manifest_path), "source_count": len(sources), "cues": [],
              "coverage": {}, "warnings": [], "audition": "Technical checks do not establish listening or gameplay acceptance."}
    ids = set()
    groups = set()
    used_sources = set()
    for cue in manifest["cues"]:
        identifier = cue.get("id")
        require(isinstance(identifier, str) and re.fullmatch(r"[a-z0-9_.]+", identifier) is not None
                and len(identifier.encode("ascii")) < 64 and identifier not in ids,
                f"missing, duplicate, or invalid bank cue id: {identifier}")
        ids.add(identifier)
        require(cue.get("tier") in (1, 2, 3) and type(cue.get("loop")) is bool,
                f"{identifier}: tier must be 1-3 and loop must be boolean")
        require(bool(cue.get("description")), f"{identifier}: missing description")
        source_ids = cue.get("source_ids")
        require(isinstance(source_ids, list) and bool(source_ids), f"{identifier}: missing source ids")
        require(all(source in sources for source in source_ids), f"{identifier}: unknown source id")
        used_sources.update(source_ids)
        group = cue.get("cue_id", re.sub(r"\.\d+$", "", identifier))
        require(isinstance(group, str) and bool(group), f"{identifier}: invalid cue_id")
        groups.add(group)
        try:
            report = inspect_pcm(repository_file(cue.get("path"), root), cue["loop"])
        except InvalidAudio as error:
            raise InvalidAudio(f"{identifier}: {error}") from error
        if "sha256" in cue:
            require(cue["sha256"] == report["sha256"], f"{identifier}: derivative SHA-256 differs")
        if identifier.startswith("music.") and cue["loop"]:
            require(report["channels"] == 2, f"{identifier}: music must be stereo")
            if not 60 <= report["seconds"] <= 120:
                report["warnings"].append("loop is outside the proposed 60-120 second music target")
        if identifier in ("match.time_up", "match.glass_out"):
            require(report["seconds"] <= 2, f"{identifier}: must fit the two-second reveal boundary")
        result["cues"].append({"id": identifier, "cue_id": group, "tier": cue["tier"], **report})
        result["warnings"].extend(f"{identifier}: {warning}" for warning in report["warnings"])
    required = manifest.get("required_cues", [])
    require(isinstance(required, list) and all(isinstance(cue, str) for cue in required),
            "required_cues must be a list of cue ids")
    missing = sorted(set(required) - groups - ids)
    require(not missing, f"required cues are missing: {', '.join(missing)}")
    for cue in ("player.step", "player.land", "paint.splash"):
        count = sum(report["cue_id"] == cue for report in result["cues"])
        if cue in required or count:
            require(4 <= count <= 6, f"{cue}: expected 4-6 distinct variants, got {count}")
    result["coverage"] = {"required": len(required), "groups": len(groups),
                          "tier_counts": {str(tier): sum(cue["tier"] == tier for cue in manifest["cues"]) for tier in (1, 2, 3)},
                          "unused_sources": sorted(sources.keys() - used_sources)}
    return manifest, result


def copy_loop(source: Path, target: Path) -> None:
    original = riff_chunks(source.read_bytes())
    encoded = riff_chunks(target.read_bytes())
    if b"smpl" in original:
        encoded[b"smpl"] = original[b"smpl"]
    payload = b"WAVE" + b"".join(tag + struct.pack("<I", len(body)) + body + b"\0" * (len(body) & 1)
                                  for tag, body in encoded.items())
    target.write_bytes(b"RIFF" + struct.pack("<I", len(payload)) + payload)


def inspect_adpcm(path: Path, source: dict, looping: bool) -> dict:
    chunks = riff_chunks(path.read_bytes())
    fmt = chunks[b"fmt "]
    require(len(fmt) >= 50, "truncated ADPCM format")
    tag, channels, rate, _, alignment, bits, extension, block_frames, coefficients = struct.unpack_from("<HHIIHHHHH", fmt)
    require((tag, channels, rate, bits, extension, block_frames, coefficients) ==
            (2, source["channels"], 44100, 4, 32, ADPCM_FRAMES_PER_BLOCK, 7),
            "unexpected MS-ADPCM format; XAudio2 needs 512-frame blocks, never use FFmpeg's oversized default block")
    require(alignment == 262 * channels and len(chunks[b"data"]) % alignment == 0,
            "ADPCM must contain whole representable XWB blocks")
    require(list(struct.iter_unpack("<hh", fmt[22:50])) ==
            [(256, 0), (512, -256), (0, 0), (192, 64), (240, 0), (460, -208), (392, -232)],
            "non-standard ADPCM coefficient table")
    require(all(predictor < coefficients
                for offset in range(0, len(chunks[b"data"]), alignment)
                for predictor in chunks[b"data"][offset:offset + channels]),
            "ADPCM block has an invalid predictor index")
    frames = len(chunks[b"data"]) // alignment * block_frames
    require(source["frames"] <= frames < source["frames"] + block_frames,
            "ADPCM duration differs by more than final block padding")
    region = loop_region(chunks, frames)
    if looping:
        require((source["loop_start"], source["loop_length"]) == (0, source["frames"]),
                "music must author a full-file loop before final block padding")
    require(region == ((0, frames) if looping else None),
            "ADPCM conversion lost the full-file loop or its final block padding")
    return {"encoding": "ms-adpcm", "frames": frames, "data_bytes": len(chunks[b"data"]),
            "block_bytes": alignment, "samples_per_block": block_frames,
            "loop_start": region[0] if region else 0, "loop_length": region[1] if region else 0}


def encode_xaudio_adpcm(source: Path, target: Path, ffmpeg: str, measured: dict, looping: bool) -> None:
    """Keep FFmpeg's predictor/quantizer while exporting XAudio2-compatible blocks.

    A stereo FFmpeg 1024-byte block describes 1012 frames. Each input window
    starts 512 frames after the previous window; retaining its first 524 bytes
    keeps exactly 512 frames. The next block has its own predictor/header. The
    last block adds at most 511 frames, holding the boundary sample for loops
    and silence for stings; accepted PCM masters are intact.
    """
    original = riff_chunks(source.read_bytes())
    channels = measured["channels"]
    frame_bytes = channels * 2
    pcm = original[b"data"]
    block_count = (measured["frames"] + ADPCM_FRAMES_PER_BLOCK - 1) // ADPCM_FRAMES_PER_BLOCK
    raw = target.with_suffix(".encoder-input.pcm")
    oversized = target.with_suffix(".encoder-output.wav")
    window_frames = 1012
    with raw.open("wb") as output:
        for block in range(block_count):
            start = block * ADPCM_FRAMES_PER_BLOCK * frame_bytes
            window = pcm[start:start + window_frames * frame_bytes]
            output.write(window)
            pad_frame = pcm[:frame_bytes] if looping else b"\0" * frame_bytes
            output.write(pad_frame * (window_frames - len(window) // frame_bytes))
    subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "error", "-y",
                    "-f", "s16le", "-ar", "44100", "-ac", str(channels), "-i", str(raw),
                    "-map_metadata", "-1", "-c:a", "adpcm_ms", "-block_size", str(512 * channels),
                    str(oversized)], check=True)
    chunks = riff_chunks(oversized.read_bytes())
    encoded_bytes = 512 * channels
    retained_bytes = 262 * channels
    require(len(chunks[b"data"]) == block_count * encoded_bytes, "unexpected FFmpeg block count")
    payload = b"".join(chunks[b"data"][offset:offset + retained_bytes]
                       for offset in range(0, len(chunks[b"data"]), encoded_bytes))
    frames = block_count * ADPCM_FRAMES_PER_BLOCK
    fmt = bytearray(chunks[b"fmt "])
    struct.pack_into("<I", fmt, 8, 44100 * retained_bytes // ADPCM_FRAMES_PER_BLOCK)
    struct.pack_into("<H", fmt, 12, retained_bytes)
    struct.pack_into("<H", fmt, 18, ADPCM_FRAMES_PER_BLOCK)
    exported = {b"fmt ": bytes(fmt), b"fact": struct.pack("<I", frames), b"data": payload}
    if looping:
        require((measured["loop_start"], measured["loop_length"]) == (0, measured["frames"]),
                "music must author a full-file loop")
        smpl = bytearray(original[b"smpl"])
        struct.pack_into("<I", smpl, 48, frames - 1)
        exported[b"smpl"] = bytes(smpl)
    body = b"WAVE" + b"".join(tag + struct.pack("<I", len(data)) + data + b"\0" * (len(data) & 1)
                                  for tag, data in exported.items())
    target.write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)
    raw.unlink()
    oversized.unlink()


def inspect_bank(path: Path, exports: list[dict]) -> dict:
    data = path.read_bytes()
    require(len(data) >= 52, "truncated XWB header")
    require(struct.unpack_from("<3I", data) == (0x444E4257, 46, 44), "unexpected XWB version")
    regions = list(zip(*[iter(struct.unpack_from("<10I", data, 12))] * 2))
    require(all(offset + size <= len(data) for offset, size in regions), "XWB segment outside file")
    require(regions[0][1] == 96, "unexpected XWB bank metadata")
    flags, count, _, entry_size, name_size, alignment, _, _ = struct.unpack_from("<II64sIIIIQ", data, regions[0][0])
    require((flags, count, entry_size, name_size, alignment) == (65536, len(exports), 24, 64, 4),
            "expected named, non-compact, in-memory XWB with every cue")
    require(regions[1][1] == count * entry_size and regions[3][1] == count * name_size,
            "XWB entry/name region length differs")
    for index, exported in enumerate(exports):
        name = data[regions[3][0] + index * name_size:regions[3][0] + (index + 1) * name_size].rstrip(b"\0").decode("ascii")
        require(name == exported["id"], f"bank entry {index}: name differs")
        duration, fmt, offset, size, start, length = struct.unpack_from("<6I", data, regions[1][0] + index * entry_size)
        channels = (fmt >> 2) & 7
        require(channels == exported["channels"] and ((fmt >> 5) & 0x3ffff) == 44100,
                f"{name}: bank rate or channels differs")
        require((start, length) == (exported["loop_start"], exported["loop_length"]),
                f"{name}: bank changed authored loop")
        require(duration >> 4 == exported["export_frames"], f"{name}: bank duration differs")
        require(offset % alignment == 0 and offset + size <= regions[4][1], f"{name}: bank samples outside data")
        expected_tag = 2 if exported["encoding"] == "ms-adpcm" else 0
        require(fmt & 3 == expected_tag, f"{name}: bank encoding differs")
        expected_alignment = 240 if expected_tag == 2 else channels * 2
        require(((fmt >> 23) & 255) == expected_alignment, f"{name}: XWB truncated block alignment")
        wave_payload = riff_chunks(Path(exported["export_path"]).read_bytes())[b"data"]
        require(data[regions[4][0] + offset:regions[4][0] + offset + size] == wave_payload,
                f"{name}: bank samples differ from exported WAV")
    require(len(data) <= 40 * 1024 * 1024, "production bank exceeds the 40 MiB target")
    return {"path": str(path), "bytes": len(data), "entries": count, "sha256": sha256(path)}


def build(manifest: dict, report: dict, output: Path, root: Path = REPOSITORY) -> dict:
    require(output.resolve().is_relative_to((root / "out").resolve()),
            "derived output must stay under repository out/; this task does not replace the live bank")
    ffmpeg = shutil.which("ffmpeg")
    require(ffmpeg is not None, "ffmpeg is required to encode music")
    xwbtool = root / "scripts/audio/tools/XWBTool.exe"
    require(xwbtool.is_file(), "checked-in XWBTool.exe is missing")
    waves = output / "waves"
    waves.mkdir(parents=True, exist_ok=True)
    exports = []
    for cue, measured in zip(manifest["cues"], report["cues"], strict=True):
        source = repository_file(cue["path"], root)
        target = waves / f"{cue['id']}.wav"
        exported = {**measured, "export_path": str(target), "encoding": "pcm16", "export_frames": measured["frames"]}
        if cue["id"].startswith("music."):
            encode_xaudio_adpcm(source, target, ffmpeg, measured, cue["loop"])
            encoded = inspect_adpcm(target, measured, cue["loop"])
            exported.update(encoding=encoded["encoding"], export_frames=encoded["frames"],
                            loop_start=encoded["loop_start"], loop_length=encoded["loop_length"],
                            padding_frames=encoded["frames"] - measured["frames"])
            # Lossy encoding can introduce a louder sample or a different join.
            # Audit what is actually heard as well as the editable PCM master.
            decoded = output / "decoded" / target.name
            decoded.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run([ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-i", str(target),
                            "-map_metadata", "-1", "-c:a", "pcm_s16le", str(decoded)], check=True)
            copy_loop(target, decoded)
            decoded_report = inspect_pcm(decoded, cue["loop"])
            require(decoded_report["frames"] == encoded["frames"], f"{cue['id']}: decoded ADPCM duration differs")
            exported["decoded"] = decoded_report
        else:
            shutil.copyfile(source, target)
        exported["export_sha256"] = sha256(target)
        exports.append(exported)
    bank = output / "paint_exe.xwb"
    listing = output / "waves.txt"
    listing.write_text("\n".join(exported["export_path"] for exported in exports) + "\n", encoding="utf-8")
    subprocess.run([str(xwbtool), "-nologo", "-nc", "-f", "-y", "-o", str(bank), "-flist", str(listing)], check=True)
    bank_report = inspect_bank(bank, exports)
    definition = {"waves": [cue["id"] for cue in manifest["cues"]],
                  "create_effect_instance_for_each_wave": False, "sound_effect_instances": []}
    (output / "paint_exe.json").write_text(json.dumps(definition, indent=2) + "\n", encoding="utf-8")
    decoded_warnings = [f"{exported['id']} (decoded MS-ADPCM): {warning}"
                        for exported in exports for warning in exported.get("decoded", {}).get("warnings", [])]
    return {**report, "warnings": report["warnings"] + decoded_warnings,
            "exports": exports, "bank": bank_report,
            "tools": {"ffmpeg": subprocess.run([ffmpeg, "-version"], capture_output=True, text=True, check=True).stdout.splitlines()[0],
                      "xwbtool_sha256": sha256(xwbtool)}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("validate", "build"))
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--sources", type=Path)
    parser.add_argument("--output", type=Path, default=REPOSITORY / "out/audio-production")
    args = parser.parse_args()
    try:
        manifest, report = validate(args.manifest.resolve(), (args.sources or args.manifest.with_name("sources.json")).resolve())
        require(args.output.resolve().is_relative_to((REPOSITORY / "out").resolve()), "reports must stay under repository out/")
        if args.command == "build":
            report = build(manifest, report, args.output.resolve())
        args.output.mkdir(parents=True, exist_ok=True)
        report_path = args.output / f"{args.command}-report.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"Validated {len(report['cues'])} waves and {report['source_count']} source records.")
        print(f"Technical listening flags: {len(report['warnings'])}; report: {report_path}")
        if "bank" in report:
            print(f"Isolated bank: {report['bank']['bytes']:,} bytes; {report['bank']['entries']} named entries.")
        return 0
    except (InvalidAudio, OSError, json.JSONDecodeError, subprocess.CalledProcessError) as error:
        print(f"Audio production failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
