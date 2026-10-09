"""Comparator controls use tiny synthetic floats, not private audio or ROM."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("compare_audio", Path(__file__).parents[1] / "scripts/compare_dependency_audio.py")
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)


class DependencyAudioTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.old, self.new = (Path(self.temp.name) / name for name in ("old", "new"))
        for folder in (self.old, self.new):
            folder.mkdir()
            rows = []
            for name in sorted(p.CASES):
                rate, block, mode = name.removesuffix(".f32").split("-")
                data = struct.pack("<ffff", 0.0, 0.0, 0.5, -0.5)
                (folder / name).write_bytes(data)
                rows.append(dict(file=name, rate=int(rate), block=int(block), corrected=mode == "corrected",
                                 samples=4, sha256=hashlib.sha256(data).hexdigest(), stateSha256="a" * 64,
                                 recallSha256="b" * 64, peak=0.5, rms=0.25, silentBlocks=1))
            (folder / "fingerprint.json").write_text(json.dumps(rows))

    def rows(self):
        return json.loads((self.new / "fingerprint.json").read_text())

    def write(self, rows):
        (self.new / "fingerprint.json").write_text(json.dumps(rows))

    def test_equal_control_and_first_sample_difference(self):
        result = p.compare(self.old, self.new)
        self.assertTrue(result["pass"])
        self.assertEqual(result["sampleValues"], 120)
        rows = self.rows()
        row = rows[0]
        data = struct.pack("<ffff", 0.0, 0.0, 0.75, -0.5)
        (self.new / row["file"]).write_bytes(data)
        row["sha256"] = hashlib.sha256(data).hexdigest()
        self.write(rows)
        result = p.compare(self.old, self.new)
        self.assertFalse(result["pass"])
        self.assertEqual(result["differentSampleValues"], 1)
        self.assertEqual(result["maxAbsDifference"], 0.25)
        self.assertEqual(result["firstDifference"]["frame"], 1)
        self.assertEqual(result["firstDifference"]["channel"], 0)

    def test_project_only_and_metric_only_changes_fail(self):
        for field, changed in (("stateSha256", "c" * 64), ("recallSha256", "c" * 64), ("peak", 0.75)):
            rows = self.rows()
            original = rows[0][field]
            rows[0][field] = changed
            self.write(rows)
            self.assertFalse(p.compare(self.old, self.new)["pass"])
            rows[0][field] = original
            self.write(rows)

    def test_missing_duplicate_unsafe_and_wrong_hash_fail_closed(self):
        original = self.rows()
        for rows in (original[:-1], [original[0]] * 30,
                     [{**original[0], "file": "../escape.f32"}, *original[1:]],
                     [{**original[0], "sha256": "0" * 64}, *original[1:]]):
            self.write(rows)
            with self.assertRaises(ValueError):
                p.compare(self.old, self.new)

    def test_nonfinite_pcm_rejected_even_with_matching_hash(self):
        rows = self.rows()
        data = struct.pack("<ffff", float("nan"), 0.0, 0.5, -0.5)
        (self.new / rows[0]["file"]).write_bytes(data)
        rows[0]["sha256"] = hashlib.sha256(data).hexdigest()
        self.write(rows)
        with self.assertRaisesRegex(ValueError, "Nonfinite"):
            p.compare(self.old, self.new)
