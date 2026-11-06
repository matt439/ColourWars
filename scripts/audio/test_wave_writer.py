"""Authoring writer compatibility and clipping; no accepted assets are rewritten."""

import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave

import audio_pipeline as pipeline

try:
    import numpy as np
    from wave_writer import write_wave
except ImportError:
    np = None


@unittest.skipUnless(np is not None, "authoring writer checks require NumPy")
class WaveWriterChecks(unittest.TestCase):
    def test_accepted_masters_keep_their_bytes(self):
        manifest = json.loads(pipeline.DEFAULT_MANIFEST.read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as scratch:
            for cue in manifest["cues"]:
                with self.subTest(cue=cue["id"]):
                    original = (pipeline.REPOSITORY / cue["path"]).read_bytes()
                    chunks = pipeline.riff_chunks(original)
                    channels = struct.unpack_from("<H", chunks[b"fmt "], 2)[0]
                    samples = np.frombuffer(chunks[b"data"], dtype="<i2").astype(np.float64) / 32767
                    if channels == 2:
                        samples = samples.reshape((-1, channels))
                    order = list(chunks)
                    before = cue["loop"] and order.index(b"smpl") < order.index(b"data")
                    path = Path(scratch) / "master.wav"
                    write_wave(path, samples, cue["loop"], channels=channels, loop_before_data=before)
                    self.assertEqual(path.read_bytes(), original)

    def test_saturation_and_full_file_loop(self):
        with tempfile.TemporaryDirectory() as scratch:
            path = Path(scratch) / "stereo.wav"
            pcm = write_wave(path, np.array([-2., -.5, .5, 2.]), True, channels=2)
            self.assertEqual(pcm.tolist(), [[-32768] * 2, [-16384] * 2,
                                          [16384] * 2, [32767] * 2])
            chunks = pipeline.riff_chunks(path.read_bytes())
            self.assertEqual(pipeline.loop_region(chunks, 4), (0, 4))
            with wave.open(str(path), "rb") as source:
                self.assertEqual(source.getparams()[:4], (2, 2, 44100, 4))

    def test_accepted_range_preserves_each_projects_container(self):
        samples = np.array([0., .1, -.2, .3])
        with tempfile.TemporaryDirectory() as scratch:
            for channels, before in ((1, False), (2, False), (2, True)):
                with self.subTest(channels=channels, loop_before_data=before):
                    pcm = np.rint(samples * 32767).astype("<i2")
                    if channels == 2:
                        pcm = np.column_stack((pcm, pcm))
                    fmt = struct.pack("<HHIIHH", 1, channels, 44100, 44100 * channels * 2,
                                      channels * 2, 16)
                    smpl = struct.pack("<15I", 0, 0, round(1e9 / 44100), 60, 0, 0, 0, 1, 0,
                                       0, 0, 0, len(pcm) - 1, 0, 0)
                    chunks = [(b"fmt ", fmt), (b"data", pcm.tobytes())]
                    chunks.insert(1 if before else 2, (b"smpl", smpl))
                    body = b"WAVE" + b"".join(tag + struct.pack("<I", len(data)) + data
                                               for tag, data in chunks)
                    old = b"RIFF" + struct.pack("<I", len(body)) + body
                    path = Path(scratch) / "render.wav"
                    write_wave(path, samples, True, channels=channels, loop_before_data=before)
                    self.assertEqual(path.read_bytes(), old)

    def test_refuses_nonfinite_audio_and_channel_mismatch(self):
        with tempfile.TemporaryDirectory() as scratch:
            path = Path(scratch) / "bad.wav"
            for samples in (np.array([float("nan")]), np.array([float("inf")]),
                            np.array([[0., 0.]])):
                with self.subTest(samples=samples):
                    with self.assertRaises(ValueError):
                        write_wave(path, samples, False)
                    self.assertFalse(path.exists())

    @unittest.skipUnless(importlib.util.find_spec("scipy"), "music CLI imports SciPy")
    def test_unknown_music_cue_is_a_cli_error(self):
        result = subprocess.run([sys.executable, str(pipeline.REPOSITORY /
            "game/audio/paint_exe/music/render.py"), "--only", "music.does_not_exist"],
            text=True, capture_output=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("unknown cue id for --only: music.does_not_exist", result.stderr)


if __name__ == "__main__":
    unittest.main()
