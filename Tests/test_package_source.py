"""ROM-free packaging positive and negative controls; run with unittest."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import sys
import zipfile

spec = importlib.util.spec_from_file_location("package_source", Path(__file__).parents[1] / "scripts/package_source.py")
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)


class SourcePackageTests(unittest.TestCase):
    def test_reserved_tooling_and_false_packager_identity_rejected(self):
        minimal = {"CMakeLists.txt": ((p.JUCE_SHA + p.CORE_SHA).encode(), 0o644)}
        for name in (p.VERIFIER, p.PACKAGE_README, p.MANIFEST):
            with patch.object(p, "snapshot", return_value={**minimal, name: (b"collision", 0o644)}):
                with self.assertRaisesRegex(ValueError, "Reserved"):
                    p.package("repo", "juce", "core", "a" * 40, Path("unused"))
        with patch.object(p, "snapshot", side_effect=[minimal, {"scripts/package_source.py": (b"wrong", 0o644)}]):
            with self.assertRaisesRegex(ValueError, "Running packager differs"):
                p.package("repo", "juce", "core", "a" * 40, Path("unused"))

    def test_old_product_new_packager_bundled_verifier(self):
        old_checker = b"raise SystemExit('old product checker rejects accepted format')\n"
        wrapper = {"CMakeLists.txt": ((p.JUCE_SHA + p.CORE_SHA).encode(), 0o644),
                   "scripts/package_source.py": (old_checker, 0o644),
                   **{name: (b"notice\n", 0o644) for name in ("LICENSE.txt", "NOTICE.md", "THIRD_PARTY.md")}}
        checker = Path(p.__file__).read_bytes().replace(b"\r\n", b"\n")
        def snapshot(repo, sha, paths=()):
            if sha == "b" * 40:
                return {"scripts/package_source.py": (checker, 0o644)}
            if repo == "wrapper": return dict(wrapper)
            if repo == "juce": return {"LICENSE.md": (b"notice\n", 0o644)}
            return {"source/dx7Lib/dx7.cpp": (b"// core\n", 0o644), "LICENSE.txt": (b"notice\n", 0o644)}
        with tempfile.TemporaryDirectory() as folder, patch.object(p, "snapshot", side_effect=snapshot):
            root = Path(folder)
            for accepted in (False, True):
                archive = p.package("wrapper", "juce", "core", "a" * 40,
                                    root / str(accepted), "1.0.0", accepted, "b" * 40)
                p.verify(archive)
                with zipfile.ZipFile(archive) as z:
                    self.assertEqual(z.read("scripts/package_source.py"), old_checker)
                    bundled = root / (str(accepted) + "-verify.py")
                    bundled.write_bytes(z.read(".vdx7-source-tools/package_source.py"))
                    self.assertIn(b".vdx7-source-tools/package_source.py", z.read("SOURCE_PACKAGE_README.txt"))
                subprocess.run([sys.executable, str(bundled), "verify", str(archive)], check=True)

    def fixture(self):
        data = b"example source\n"
        return {"Source/example.cpp": (data, 0o644)}, {
            "schema": 1, "kind": "development-corresponding-source",
            "package_label": "1.0.0-dev", "source_commit": "a" * 40,
            "release_accepted": False,
            "dependencies": {"JUCE": p.JUCE_SHA, "Retromulator": p.CORE_SHA},
            "files": [{"path": "Source/example.cpp", "size": len(data), "mode": 0o644,
                       "sha256": hashlib.sha256(data).hexdigest()}],
        }

    def test_deterministic_archive_and_verification(self):
        with tempfile.TemporaryDirectory() as folder:
            a, b = Path(folder)/"a.zip", Path(folder)/"b.zip"
            files, manifest = self.fixture()
            p.write_zip(a, files, manifest)
            p.write_zip(b, files, manifest)
            self.assertEqual(a.read_bytes(), b.read_bytes())
            p.verify(a)
            with self.assertRaises(FileExistsError):
                p.write_zip(a, files, manifest)

    def test_unsafe_paths(self):
        for name in ("../escape", "/absolute", "C:/drive", "a\\b", "a/../b", "a//b"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                p.safe_path(name)

    def test_forbidden_payloads(self):
        for name in ("ROM/firmware.dat", "dx7.bin", "bank.syx", "build/cache.txt", ".git/config",
                     "state.zip", ".env.local", "credential.pem", "file.obj"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                p.inspect_payload(name, b"not a real ROM or credential")
        p.inspect_payload("ROM/PUT_YOUR_ROM_HERE.txt", b"Instructions only")
        p.inspect_payload("extras/Build/tool.cpp", b"source", upstream=True)
        p.inspect_payload("examples/Assets/teapot.obj", b"mesh", upstream=True)
        for data in (("ghp_" + "x" * 36).encode(), ("/Users/" + "example-person/private").encode()):
            with self.assertRaises(ValueError):
                p.inspect_payload("Source/test.cpp", data)

    def test_tampering_and_extra_entry(self):
        files, manifest = self.fixture()
        with tempfile.TemporaryDirectory() as folder:
            for label, altered in (("tampered", {"Source/example.cpp": (b"changed", 0o644)}),
                                   ("extra", {**files, "extra.txt": (b"extra", 0o644)})):
                path = Path(folder)/(label + ".zip")
                p.write_zip(path, altered, manifest)
                with self.assertRaises(ValueError):
                    p.verify(path)

    def test_modes_and_manifest_identity(self):
        files, manifest = self.fixture()
        with tempfile.TemporaryDirectory() as folder:
            for label in ("mode", "identity", "label"):
                changed = json.loads(json.dumps(manifest))
                if label == "mode":
                    changed["files"][0]["mode"] = 0o755
                elif label == "identity":
                    changed["release_accepted"] = True
                else:
                    changed["package_label"] = "1.0.0"
                    changed["kind"] = None
                path = Path(folder)/(label + ".zip")
                p.write_zip(path, files, changed)
                with self.assertRaises(ValueError):
                    p.verify(path)

    def test_prepublication_source_package_labels(self):
        files, manifest = self.fixture()
        labels = {
            "1.0.0-dev": "development-corresponding-source",
            "1.0.0-rc2": "release-candidate-corresponding-source",
            "1.0.0": "stable-release-preparation-corresponding-source",
        }
        with tempfile.TemporaryDirectory() as folder:
            for index, (label, kind) in enumerate(labels.items()):
                with self.subTest(label=label):
                    candidate = json.loads(json.dumps(manifest))
                    candidate["package_label"] = label
                    candidate["kind"] = kind
                    path = Path(folder) / f"package-{index}.zip"
                    p.write_zip(path, files, candidate)
                    p.verify(path)

    def test_explicit_stable_publication_acceptance(self):
        files, manifest = self.fixture()
        manifest["package_label"] = "1.0.0"
        manifest["kind"] = "stable-release-corresponding-source"
        manifest["release_accepted"] = True
        manifest["packager_commit"] = "b" * 40
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "accepted-stable.zip"
            p.write_zip(path, files, manifest)
            p.verify(path)

        manifest["package_label"] = "1.0.0-rc1"
        manifest["kind"] = "release-candidate-corresponding-source"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "accepted-rc.zip"
            p.write_zip(path, files, manifest)
            with self.assertRaises(ValueError):
                p.verify(path)

    def test_snapshot_ignores_dirty_and_untracked_files(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            def run(*args):
                return subprocess.check_output(["git", "-C", folder, *args], stderr=subprocess.STDOUT)
            run("init")
            (root/"source.txt").write_bytes(b"committed\n")
            run("add", "source.txt")
            run("-c", "user.name=Package Test", "-c", "user.email=fixture@example.invalid",
                "commit", "-m", "fixture")
            sha = run("rev-parse", "HEAD").decode().strip()
            (root/"source.txt").write_bytes(b"dirty\n")
            (root/"untracked.bin").write_bytes(b"synthetic, not firmware")
            self.assertEqual(p.snapshot(root, sha)["source.txt"][0], b"committed\n")
            self.assertNotIn("untracked.bin", p.snapshot(root, sha))
            with self.assertRaises(ValueError):
                p.snapshot(root, "HEAD")


if __name__ == "__main__":
    unittest.main()
