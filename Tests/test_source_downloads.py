"""Separate durable-source staging, not installer or publication acceptance."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
import source_package_fixture as fixture

SCRIPTS = Path(__file__).parents[1] / "scripts"
spec = importlib.util.spec_from_file_location("source_downloads", SCRIPTS / "stage_source_downloads.py")
source_downloads = importlib.util.module_from_spec(spec)
sys.path.insert(0, str(SCRIPTS))
try:
    spec.loader.exec_module(source_downloads)
finally:
    sys.path.pop(0)
p = source_downloads.package_source


class SourceDownloadTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.assets, self.output = self.root / "assets", self.root / "source-downloads"
        self.assets.mkdir()
        self.source, self.tool, self.approval = "a" * 40, "b" * 40, "c" * 40
        self.name = f"VDX7-1.0.1-{self.source}-corresponding-source.zip"
        self.data = b"// synthetic source fixture\n"
        self.payload = {**fixture.files(), "Source/example.cpp": (self.data, 0o644)}
        self.manifest = {
            "schema": 1, "kind": "stable-release-preparation-corresponding-source",
            "package_label": "1.0.1", "source_commit": self.source,
            "packager_commit": self.tool, "release_accepted": False,
            "dependencies": {"JUCE": p.JUCE_SHA, "Retromulator": p.CORE_SHA},
            "files": fixture.entries(self.payload),
        }
        self.write_archive()
        (self.assets / "VDX7-1.0.1-Windows-x64-Setup.exe").write_bytes(b"retained binary")

    def write_archive(self):
        archive = self.assets / self.name
        if archive.exists():
            archive.unlink()
        p.write_zip(archive, self.payload, self.manifest)
        source_downloads.write_manifest([archive], self.assets / "SHA256SUMS.txt")

    def stage(self, **changes):
        args = dict(assets=self.assets, output=self.output, label="1.0.1",
                    source_commit=self.source, packager_commit=self.tool,
                    approval_commit=self.approval, accepted=False)
        args.update(changes)
        return source_downloads.stage(**args)

    def test_exact_two_files_keep_validation_and_binary_assets_unchanged(self):
        before = {path.name: path.read_bytes() for path in self.assets.iterdir()}
        result = self.stage()
        self.assertEqual({path.name for path in self.output.iterdir()}, {self.name, "SHA256SUMS.txt"})
        self.assertEqual({path.name: path.read_bytes() for path in self.assets.iterdir()}, before)
        self.assertEqual((self.output / self.name).read_bytes(), before[self.name])
        self.assertEqual(source_downloads.read_checksums(self.output / "SHA256SUMS.txt"), result)

    def test_hash_failure_creates_no_output(self):
        with (self.assets / self.name).open("ab") as stream:
            stream.write(b"tampered")
        with self.assertRaises(ValueError):
            self.stage()
        self.assertFalse(self.output.exists())

    def test_required_license_removed_and_rehashed_creates_no_output(self):
        del self.payload["LICENSE.txt"]
        self.manifest["files"] = fixture.entries(self.payload)
        self.write_archive()
        with self.assertRaises(ValueError):
            self.stage()
        self.assertFalse(self.output.exists())

    def test_manifest_payload_failure_even_after_archive_rehash(self):
        self.manifest["files"][0]["sha256"] = "0" * 64
        self.write_archive()
        with self.assertRaises(ValueError):
            self.stage()
        self.assertFalse(self.output.exists())

    def test_tuple_mismatch_after_valid_rehash_is_rejected(self):
        for field, value in (("source_commit", "d" * 40), ("packager_commit", "d" * 40)):
            with self.subTest(field=field):
                old = self.manifest[field]
                self.manifest[field] = value
                self.write_archive()
                with self.assertRaises(ValueError):
                    self.stage()
                self.assertFalse(self.output.exists())
                self.manifest[field] = old

    def test_accepted_requires_exact_approval_proof_and_staging_status(self):
        self.manifest.update(kind="stable-release-corresponding-source", release_accepted=True)
        self.write_archive()
        with self.assertRaises(ValueError):
            self.stage(accepted=True)
        self.manifest["release_approval"] = {
            "source_commit": self.source, "packager_commit": self.tool,
            "approval_commit": self.approval, "package_label": "1.0.1",
            "workflow_ref": p.APPROVED_WORKFLOW_REF, "workflow_git_ref": p.APPROVED_GIT_REF,
            "policy_sha256": "d" * 64,
        }
        self.write_archive()
        with self.assertRaises(ValueError):
            self.stage()
        with self.assertRaises(ValueError):
            self.stage(accepted=True, approval_commit="e" * 40)
        self.assertFalse(self.output.exists())
        self.stage(accepted=True)

    def test_invalid_identities_and_prereleases_create_no_output(self):
        for changes in ({"source_commit": "../escape"}, {"packager_commit": None},
                        {"approval_commit": "main"}, {"label": "1.0.1-dev"},
                        {"label": "1.0.2"}, {"accepted": "false"}):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                self.stage(**changes)
        self.assertFalse(self.output.exists())

    def test_old_output_and_validation_directory_are_preserved(self):
        self.output.mkdir()
        marker = self.output / "keep.txt"
        marker.write_bytes(b"keep")
        with self.assertRaises(ValueError):
            self.stage()
        self.assertEqual(marker.read_bytes(), b"keep")
        with self.assertRaises(ValueError):
            self.stage(output=self.assets / "nested")
        self.assertFalse((self.assets / "nested").exists())

    def test_duplicate_checksum_entries_are_rejected(self):
        checksum = self.assets / "SHA256SUMS.txt"
        checksum.write_text(checksum.read_text() * 2)
        with self.assertRaises(ValueError):
            self.stage()
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()
