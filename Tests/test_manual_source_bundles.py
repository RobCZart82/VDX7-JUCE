"""Regression tests for source archives nested in candidate Manual ZIPs."""
import hashlib
from pathlib import Path
import tempfile
import unittest
import zipfile

from scripts.verify_manual_source_bundles import verify_manual_source_bundles


SOURCE_NAME = "VDX7-1.2.0-rc2-0123456789abcdef0123456789abcdef01234567-corresponding-source.zip"
SOURCE_BYTES = b"matching source archive bytes"


class ManualSourceBundleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=Path(__file__).parents[1])
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write_manual_zip(self, filename, source_bytes=SOURCE_BYTES, source_name=SOURCE_NAME,
                         checksum=None, include_plugin=True, platform="macos",
                         include_windows_binary=True):
        path = self.root / filename
        digest = checksum or hashlib.sha256(source_bytes).hexdigest()
        with zipfile.ZipFile(path, "w") as archive:
            if include_plugin:
                if platform == "windows":
                    archive.writestr("VDX7.vst3/Contents/Resources/moduleinfo.json", b"{}")
                    if include_windows_binary:
                        archive.writestr("VDX7.vst3/Contents/x86_64-win/VDX7.vst3", b"plugin binary")
                else:
                    archive.writestr("VDX7.vst3/Contents/Info.plist", b"plugin marker")
            archive.writestr(f"Source/{source_name}", source_bytes)
            archive.writestr("Source/SHA256SUMS.txt", f"{digest}  {source_name}\n")
        return path

    def test_matching_source_is_verified_in_both_manual_archives(self):
        windows = self.write_manual_zip(
            "VDX7-1.2.0-rc2-Windows-x64-Manual.zip", platform="windows"
        )
        macos = self.write_manual_zip("VDX7-1.2.0-rc2-macOS-universal-Manual.zip")

        digest = verify_manual_source_bundles([windows, macos], SOURCE_NAME)

        self.assertEqual(digest, hashlib.sha256(SOURCE_BYTES).hexdigest())

    def test_rejects_a_source_checksum_mismatch(self):
        manual = self.write_manual_zip(
            "VDX7-1.2.0-rc2-macOS-universal-Manual.zip", checksum="0" * 64
        )

        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            verify_manual_source_bundles([manual], SOURCE_NAME)

    def test_rejects_different_source_archives_between_platforms(self):
        windows = self.write_manual_zip(
            "VDX7-1.2.0-rc2-Windows-x64-Manual.zip", platform="windows"
        )
        macos = self.write_manual_zip(
            "VDX7-1.2.0-rc2-macOS-universal-Manual.zip", source_bytes=b"different source"
        )

        with self.assertRaisesRegex(ValueError, "different corresponding-source"):
            verify_manual_source_bundles([windows, macos], SOURCE_NAME)

    def test_rejects_missing_plugin_or_source_bundle(self):
        manual = self.write_manual_zip(
            "VDX7-1.2.0-rc2-macOS-universal-Manual.zip", include_plugin=False
        )

        with self.assertRaisesRegex(ValueError, "VST3 bundle marker"):
            verify_manual_source_bundles([manual], SOURCE_NAME)

    def test_rejects_windows_bundle_without_platform_binary(self):
        manual = self.write_manual_zip(
            "VDX7-1.2.0-rc2-Windows-x64-Manual.zip",
            platform="windows",
            include_windows_binary=False,
        )

        with self.assertRaisesRegex(ValueError, "VST3 bundle marker"):
            verify_manual_source_bundles([manual], SOURCE_NAME)

    def test_rejects_wrong_platform_bundle_even_when_other_platform_marker_exists(self):
        windows_named_macos_bundle = self.write_manual_zip(
            "VDX7-1.2.0-rc2-Windows-x64-Manual.zip", platform="macos"
        )
        macos_named_windows_bundle = self.write_manual_zip(
            "VDX7-1.2.0-rc2-macOS-universal-Manual.zip", platform="windows"
        )

        for manual in (windows_named_macos_bundle, macos_named_windows_bundle):
            with self.subTest(manual=manual.name):
                with self.assertRaisesRegex(ValueError, "Platform-specific VST3 bundle marker"):
                    verify_manual_source_bundles([manual], SOURCE_NAME)

    def test_rejects_path_traversal_members(self):
        manual = self.root / "unsafe.zip"
        with zipfile.ZipFile(manual, "w") as archive:
            archive.writestr("../Source/escape", b"not allowed")

        with self.assertRaisesRegex(ValueError, "Unsafe ZIP member path"):
            verify_manual_source_bundles([manual], SOURCE_NAME)


if __name__ == "__main__":
    unittest.main()
