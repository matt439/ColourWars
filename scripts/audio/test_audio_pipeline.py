"""Small malformed-input and loop-metadata checks for the production pipeline."""

from array import array
import json
from pathlib import Path
import struct
import shutil
import sys
import tempfile
import unittest
import wave

import audio_pipeline as pipeline


def write_wave(path: Path, *, channels: int = 1, peak: int = 2000,
               frames: int = 1000, looping: bool = False) -> None:
    samples = array("h", [peak, -peak] * (frames * channels // 2))
    if sys.byteorder != "little":
        samples.byteswap()
    with wave.open(str(path), "wb") as output:
        output.setparams((channels, 2, 44100, 0, "NONE", "PCM"))
        output.writeframes(samples.tobytes())
    if looping:
        append_loop(path, 500, 999)


def append_loop(path: Path, start: int, inclusive_end: int) -> None:
    data = path.read_bytes()
    smpl = struct.pack("<15I", 0, 0, 22676, 60, 0, 0, 0, 1, 0,
                       0, 0, start, inclusive_end, 0, 0)
    data += b"smpl" + struct.pack("<I", len(smpl)) + smpl
    path.write_bytes(data[:4] + struct.pack("<I", len(data) - 8) + data[8:])


class ProductionChecks(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory()
        self.root = Path(self.scratch.name)
        self.wav = self.root / "source.wav"
        write_wave(self.wav)

    def tearDown(self) -> None:
        self.scratch.cleanup()

    def test_pcm_metrics_and_bounded_paths(self) -> None:
        measured = pipeline.inspect_pcm(self.wav, False)
        self.assertEqual((measured["frames"], measured["channels"], measured["onset_ms"]), (1000, 1, 0))
        self.assertEqual(pipeline.repository_file("source.wav", self.root), self.wav.resolve())
        with self.assertRaisesRegex(pipeline.InvalidAudio, "escapes"):
            pipeline.repository_file("../elsewhere.wav", self.root)
        with self.assertRaisesRegex(pipeline.InvalidAudio, "relative"):
            pipeline.repository_file(str(self.wav.resolve()), self.root)

    def test_full_scale_rejected(self) -> None:
        write_wave(self.wav, peak=32767)
        with self.assertRaisesRegex(pipeline.InvalidAudio, "peak"):
            pipeline.inspect_pcm(self.wav, False)

    def test_silent_wave_rejected(self) -> None:
        write_wave(self.wav, peak=0)
        with self.assertRaisesRegex(pipeline.InvalidAudio, "silent"):
            pipeline.inspect_pcm(self.wav, False)

    def test_smpl_end_is_inclusive_and_survives_copy(self) -> None:
        write_wave(self.wav, looping=True)
        measured = pipeline.inspect_pcm(self.wav, True)
        self.assertEqual((measured["loop_start"], measured["loop_length"]), (500, 500))
        exported = self.root / "export.wav"
        write_wave(exported)
        pipeline.copy_loop(self.wav, exported)
        self.assertEqual(pipeline.loop_region(pipeline.riff_chunks(exported.read_bytes()), 1000), (500, 500))

    def test_loop_contract_and_bounds_rejected(self) -> None:
        with self.assertRaisesRegex(pipeline.InvalidAudio, "disagree"):
            pipeline.inspect_pcm(self.wav, True)
        append_loop(self.wav, 500, 1000)
        with self.assertRaisesRegex(pipeline.InvalidAudio, "outside"):
            pipeline.inspect_pcm(self.wav, True)

    def test_truncated_wave_rejected(self) -> None:
        self.wav.write_bytes(self.wav.read_bytes()[:-1])
        with self.assertRaisesRegex(pipeline.InvalidAudio, "length"):
            pipeline.inspect_pcm(self.wav, False)

    def test_source_permission_hash_and_cue_coverage(self) -> None:
        source = {"id": "test-recording", "creator": "Fixture", "url": "fixture:original",
                  "license": "CC0-1.0", "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
                  "path": "source.wav", "sha256": pipeline.sha256(self.wav), "obtained": "2026-10-14",
                  "redistribution": True, "release_use": True, "attribution": "None required"}
        source_path = self.root / "sources.json"
        source_path.write_text(json.dumps({"sources": [source]}))
        cue = {"id": "ui.confirm", "tier": 1, "path": "source.wav", "loop": False,
               "source_ids": ["test-recording"], "description": "A fixture click"}
        manifest = {"version": 1, "required_cues": ["ui.confirm"], "cues": [cue]}
        manifest_path = self.root / "cues.json"
        manifest_path.write_text(json.dumps(manifest))
        _, report = pipeline.validate(manifest_path, source_path, self.root)
        self.assertEqual(report["coverage"]["tier_counts"], {"1": 1, "2": 0, "3": 0})
        cue["sha256"] = pipeline.sha256(self.wav)
        manifest_path.write_text(json.dumps(manifest))
        pipeline.validate(manifest_path, source_path, self.root)
        cue["sha256"] = "0" * 64
        manifest_path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(pipeline.InvalidAudio, "derivative SHA-256"):
            pipeline.validate(manifest_path, source_path, self.root)
        del cue["sha256"]
        manifest["required_cues"].append("ui.cancel")
        manifest_path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(pipeline.InvalidAudio, "required cues"):
            pipeline.validate(manifest_path, source_path, self.root)
        source["redistribution"] = False
        source_path.write_text(json.dumps({"sources": [source]}))
        with self.assertRaisesRegex(pipeline.InvalidAudio, "explicitly permitted"):
            pipeline.load_sources(source_path, self.root)
        source["redistribution"] = True
        source["sha256"] = "0" * 64
        source_path.write_text(json.dumps({"sources": [source]}))
        with self.assertRaisesRegex(pipeline.InvalidAudio, "SHA-256"):
            pipeline.load_sources(source_path, self.root)

    def test_output_cannot_replace_live_bank(self) -> None:
        with self.assertRaisesRegex(pipeline.InvalidAudio, "out/"):
            pipeline.build({}, {}, self.root / "game/content/sounds", self.root)

    def test_oversized_adpcm_blocks_rejected(self) -> None:
        # FFmpeg's 1024-byte default is accepted then truncated by XWBTool;
        # the authoring validator must reject it before that tool runs.
        coefficients = b"".join(struct.pack("<hh", *pair) for pair in
                                [(256, 0), (512, -256), (0, 0), (192, 64), (240, 0), (460, -208), (392, -232)])
        fmt = struct.pack("<HHIIHHHHH", 2, 1, 44100, 22180, 1024, 4, 32, 2036, 7) + coefficients
        body = b"WAVEfmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", 1024) + b"\0" * 1024
        self.wav.write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)
        with self.assertRaisesRegex(pipeline.InvalidAudio, "oversized default"):
            pipeline.inspect_adpcm(self.wav, {"channels": 1}, False)

    def test_corrupt_adpcm_predictor_rejected(self) -> None:
        coefficients = b"".join(struct.pack("<hh", *pair) for pair in
                                [(256, 0), (512, -256), (0, 0), (192, 64), (240, 0), (460, -208), (392, -232)])
        fmt = struct.pack("<HHIIHHHHH", 2, 1, 44100, 22579, 256, 4, 32, 500, 7) + coefficients
        # There are seven coefficient pairs, indexed 0-6; 7 is corrupt.
        block = b"\x07" + b"\0" * 255
        body = b"WAVEfmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", len(block)) + block
        self.wav.write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)
        with self.assertRaisesRegex(pipeline.InvalidAudio, "predictor"):
            pipeline.inspect_adpcm(self.wav, {"channels": 1}, False)

    @unittest.skipUnless(shutil.which("ffmpeg") and
                         (pipeline.REPOSITORY / "game/content/sounds/sound_bank_1/XWBTool.exe").is_file(),
                         "integration check requires ffmpeg and the bundled XWBTool")
    def test_real_adpcm_bank_preserves_loop_and_detects_corruption(self) -> None:
        scratch_parent = pipeline.REPOSITORY / "out/audio-pipeline-tests"
        scratch_parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch_parent) as scratch:
            folder = Path(scratch)
            source = folder / "loop.wav"
            write_wave(source, channels=2, looping=True)
            cue = {"id": "music.probe", "path": source.relative_to(pipeline.REPOSITORY).as_posix(), "loop": True}
            measured = {"id": cue["id"], **pipeline.inspect_pcm(source, True)}
            report = pipeline.build({"cues": [cue]}, {"cues": [measured], "warnings": []}, folder / "built")
            self.assertEqual(report["bank"]["entries"], 1)
            self.assertEqual(report["exports"][0]["decoded"]["frames"], 1000)
            bank_path = Path(report["bank"]["path"])
            bank = bytearray(bank_path.read_bytes())
            entries_at = struct.unpack_from("<I", bank, 20)[0]
            fmt = struct.unpack_from("<I", bank, entries_at + 4)[0]
            struct.pack_into("<I", bank, entries_at + 4, fmt ^ (1 << 23))
            bank_path.write_bytes(bank)
            with self.assertRaisesRegex(pipeline.InvalidAudio, "block alignment"):
                pipeline.inspect_bank(bank_path, report["exports"])


if __name__ == "__main__":
    unittest.main()
