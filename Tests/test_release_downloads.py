"""Four public download assets, distinct from retained validation/source evidence."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "stage_downloads", Path(__file__).parents[1] / "scripts/stage_release_downloads.py")
downloads = importlib.util.module_from_spec(spec)
spec.loader.exec_module(downloads)


class ReleaseDownloadTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.assets, self.output = self.root / "assets", self.root / "public"
        self.assets.mkdir()
        self.names = downloads.download_names("1.0.1")
        for name in self.names:
            (self.assets / name).write_bytes(b"synthetic asset: " + name.encode())
        for name in ("BUILD-INFO-Windows-x64.txt", "BUILD-INFO-macOS-universal.txt",
                     "VDX7-1.0.1-fixture-corresponding-source.zip", "unexpected.txt"):
            (self.assets / name).write_bytes(b"retained internal evidence")
        self.manifest = self.assets / "SHA256SUMS.txt"
        self.manifest.write_text("".join(
            f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n"
            for p in sorted(self.assets.iterdir())), encoding="utf-8")

    def test_exactly_four_matching_copies_and_internal_evidence_preserved(self):
        before = {p.name: p.read_bytes() for p in self.assets.iterdir()}
        result = downloads.stage(self.assets, self.output, "1.0.1")
        self.assertEqual(set(result), set(self.names))
        self.assertEqual({p.name for p in self.output.iterdir()}, set(self.names))
        self.assertEqual({p.name: p.read_bytes() for p in self.assets.iterdir()}, before)
        for name in self.names:
            self.assertEqual((self.output / name).read_bytes(), before[name])

    def test_missing_asset_or_tampering_rejected_before_output(self):
        target = self.assets / self.names[-1]
        original = target.read_bytes()
        target.unlink()
        with self.assertRaises(ValueError):
            downloads.stage(self.assets, self.output, "1.0.1")
        self.assertFalse(self.output.exists())
        target.write_bytes(original + b"tampered")
        with self.assertRaises(ValueError):
            downloads.stage(self.assets, self.output, "1.0.1")
        self.assertFalse(self.output.exists())

    def test_existing_output_is_never_overwritten_or_extended(self):
        self.output.mkdir()
        marker = self.output / "old.zip"
        marker.write_bytes(b"keep")
        with self.assertRaises(ValueError):
            downloads.stage(self.assets, self.output, "1.0.1")
        self.assertEqual(marker.read_bytes(), b"keep")
        self.assertEqual(list(self.output.iterdir()), [marker])

    def test_missing_duplicate_and_unsafe_manifest_entries_rejected(self):
        original = self.manifest.read_text()
        cases = ("", original + original.splitlines()[0] + "\n",
                 "a" * 64 + "  ../escape.zip\n", "not a manifest\n")
        for text in cases:
            with self.subTest(text=text[:30]):
                self.manifest.write_text(text)
                with self.assertRaises(ValueError):
                    downloads.stage(self.assets, self.output, "1.0.1")
                self.assertFalse(self.output.exists())

    def test_invalid_labels_cannot_select_another_path_or_prerelease(self):
        for label in ("../1.0.1", "1.0.1-dev", "1.0.1-rc1", "", None):
            with self.subTest(label=label):
                with self.assertRaises(ValueError):
                    downloads.download_names(label)
        self.assertEqual(downloads.download_names("2.3.4")[0], "VDX7-2.3.4-Windows-x64-Setup.exe")

    def test_wrong_version_cannot_stage_old_assets(self):
        with self.assertRaises(ValueError):
            downloads.stage(self.assets, self.output, "1.0.2")
        self.assertFalse(self.output.exists())

    def test_output_inside_validation_directory_rejected(self):
        output = self.assets / "public"
        with self.assertRaises(ValueError):
            downloads.stage(self.assets, output, "1.0.1")
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
