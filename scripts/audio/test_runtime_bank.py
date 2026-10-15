"""Exercise publication integrity, optional fallback and incremental staging."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

import audio_pipeline as pipeline
import publish_runtime_bank as publication


ROOT = pipeline.REPOSITORY


class RuntimeBankChecks(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory()
        self.folder = Path(self.scratch.name)
        self.bank = self.folder / "paint_exe.xwb"
        self.bank.write_bytes(b"accepted fixture bank")
        self.receipt = self.folder / "receipt.json"
        self.receipt.write_text(json.dumps({"bytes": self.bank.stat().st_size,
            "sha256": pipeline.sha256(self.bank), "waves": ["ui.confirm"]}))
        self.definition = {"waves": ["ui.confirm"], "create_effect_instance_for_each_wave": False,
                           "sound_effect_instances": [{"name": "probe", "wave": "ui.confirm"}]}
        self.write_definition()

    def tearDown(self) -> None:
        self.scratch.cleanup()

    def write_definition(self) -> None:
        (self.folder / "paint_exe.json").write_text(json.dumps(self.definition))

    def check(self, *, require: bool = False) -> subprocess.CompletedProcess:
        return subprocess.run(["cmake", f"-DBANK_DIR={self.folder}", f"-DRECEIPT={self.receipt}",
            f"-DREQUIRE_BANK={'ON' if require else 'OFF'}", "-P", str(ROOT / "cmake/check_wave_bank.cmake")],
            text=True, capture_output=True)

    def test_hash_and_definition_are_checked(self) -> None:
        self.assertEqual(self.check().returncode, 0)
        self.bank.write_bytes(b"different bank")
        result = self.check()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("differs from its accepted build receipt", result.stderr)
        self.definition["waves"] = ["misspelt.confirm"]
        self.write_definition()
        result = self.check()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("differs from accepted bank entry", result.stderr)

    def test_implicit_and_invalid_effects_are_rejected(self) -> None:
        self.definition["create_effect_instance_for_each_wave"] = True
        self.write_definition()
        self.assertIn("must be named explicitly", self.check().stderr)
        self.definition["create_effect_instance_for_each_wave"] = False
        self.definition["sound_effect_instances"][0]["wave"] = "missing.wave"
        self.write_definition()
        self.assertIn("invalid or duplicate effect", self.check().stderr)
        self.definition["sound_effect_instances"] = [{"name": "probe", "wave": "ui.confirm"}] * 2
        self.write_definition()
        self.assertIn("invalid or duplicate effect", self.check().stderr)

    def test_optional_missing_bank_passes_but_audible_release_requires_it(self) -> None:
        self.bank.unlink()
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("optional silent fallback", result.stdout)
        self.assertIn("audible release requires", self.check(require=True).stderr)

    def test_incremental_staging_removes_legacy_and_absent_bank(self) -> None:
        stage = self.folder / "stage"
        (stage / "unused").mkdir(parents=True)
        for name in ("sound_bank_1.xwb", "sound_bank_1.json", "sound list.txt", "unused/wave_bank_1.json"):
            (stage / name).write_bytes(b"old")
        command = ["cmake", f"-DBANK_FILE={self.bank}", f"-DSTAGE_DIR={stage}",
                   "-P", str(ROOT / "cmake/stage_wave_bank.cmake")]
        subprocess.run(command, check=True, capture_output=True)
        self.assertEqual((stage / self.bank.name).read_bytes(), self.bank.read_bytes())
        self.assertEqual([path.name for path in stage.rglob("*") if path.is_file()], [self.bank.name])
        self.bank.unlink()
        subprocess.run(command, check=True, capture_output=True)
        self.assertFalse((stage / self.bank.name).exists())

    def test_publisher_requires_the_exact_accepted_master_set(self) -> None:
        manifest = {"cues": [{"id": "ui.confirm"}]}
        report = {"cues": [{"id": "ui.confirm", "sha256": "accepted"}]}
        acceptance = {"status": "accepted", "master_sha256": {"ui.confirm": "accepted"}}
        self.assertEqual(publication.accepted_masters(manifest, report, acceptance), {"ui.confirm": "accepted"})
        report["cues"][0]["sha256"] = "changed"
        with self.assertRaisesRegex(pipeline.InvalidAudio, "accepted master hashes differ"):
            publication.accepted_masters(manifest, report, acceptance)

    def test_tracked_bank_names_and_hash_match_accepted_receipt(self) -> None:
        receipt = json.loads((ROOT / "scripts/audio/runtime-bank.json").read_text())
        acceptance = json.loads((ROOT / "game/audio/paint_exe/acceptance.json").read_text())
        bank = (ROOT / "game/content/sounds/paint_exe.xwb").read_bytes()
        self.assertEqual(hashlib.sha256(bank).hexdigest(), receipt["sha256"])
        self.assertEqual(len(bank), receipt["bytes"])
        self.assertEqual(receipt["accepted_master_sha256"], acceptance["master_sha256"])
        self.assertEqual(struct.unpack_from("<3I", bank), (0x444E4257, 46, 44))
        metadata, _ = struct.unpack_from("<II", bank, 12)
        flags, count, _, entry_size, name_size, _, _, _ = struct.unpack_from("<II64sIIIIQ", bank, metadata)
        self.assertEqual((flags, count, entry_size, name_size), (65536, 103, 24, 64))
        names_at, names_size = struct.unpack_from("<II", bank, 36)
        self.assertEqual(names_size, count * name_size)
        names = [bank[names_at + i * name_size:names_at + (i + 1) * name_size].rstrip(b"\0").decode("ascii")
                 for i in range(count)]
        self.assertEqual(names, receipt["waves"])
        entries_at, _ = struct.unpack_from("<II", bank, 20)
        for index, name in enumerate(names):
            if not name.startswith("music."):
                continue
            _, fmt, _, _, start, length = struct.unpack_from("<6I", bank, entries_at + index * entry_size)
            self.assertEqual(fmt & 3, 2, name)
            self.assertEqual((fmt >> 2) & 7, 2, name)
            self.assertEqual(((fmt >> 23) & 255) + 22, 262, name)
            self.assertEqual(start % 512, 0, name)
            self.assertEqual(length % 512, 0, name)


if __name__ == "__main__":
    unittest.main()
