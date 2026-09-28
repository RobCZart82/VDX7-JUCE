"""Release-manifest tests; checksum files contain only portable basenames."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "write_sha256_manifest", Path(__file__).parents[1] / "scripts/write_sha256_manifest.py")
checksums = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checksums)


class ReleaseChecksumTests(unittest.TestCase):
    def test_manifest_hashes_sorted_files_by_basename(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            first = root / "b.zip"
            second = root / "a.zip"
            first.write_bytes(b"package b")
            second.write_bytes(b"package a")
            output = root / "SHA256SUMS.txt"
            checksums.write_manifest([first, second], output)
            self.assertEqual(output.read_text(encoding="utf-8").splitlines(), [
                f"{hashlib.sha256(b'package a').hexdigest()}  a.zip",
                f"{hashlib.sha256(b'package b').hexdigest()}  b.zip",
            ])

    def test_duplicate_basenames_and_symlinks_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            one = root / "one"
            two = root / "two"
            one.mkdir()
            two.mkdir()
            first, second = one / "same.zip", two / "same.zip"
            first.write_bytes(b"one")
            second.write_bytes(b"two")
            with self.assertRaises(ValueError):
                checksums.write_manifest([first, second], root / "duplicates.txt")
            link = root / "link.zip"
            link.symlink_to(first)
            with self.assertRaises(ValueError):
                checksums.write_manifest([link], root / "link-check.txt")


if __name__ == "__main__":
    unittest.main()
