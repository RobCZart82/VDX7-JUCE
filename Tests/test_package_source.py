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
import source_package_fixture as fixture

spec = importlib.util.spec_from_file_location("package_source", Path(__file__).parents[1] / "scripts/package_source.py")
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)


class SourcePackageTests(unittest.TestCase):
    def test_new_pin_verifier_retains_historical_901_archive_integrity(self):
        # Frozen 1.0.1 packagers must remain verifiable by newer approval tools.
        files, manifest = self.fixture()
        manifest["dependencies"]["JUCE"] = "e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8"
        with tempfile.TemporaryDirectory() as folder:
            archive = Path(folder) / "historical.zip"
            p.write_zip(archive, files, manifest)
            with patch.object(p, "JUCE_SHA", "be29c81492b6151c8ea8d14c840e1311963b3a83"):
                p.verify(archive)

    def test_historical_verification_does_not_allow_unknown_pins_or_creation(self):
        files, manifest = self.fixture()
        with tempfile.TemporaryDirectory() as folder:
            for dependency in ("JUCE", "Retromulator"):
                changed = json.loads(json.dumps(manifest))
                changed["dependencies"][dependency] = "f" * 40
                archive = Path(folder) / (dependency + ".zip")
                p.write_zip(archive, files, changed)
                with self.assertRaisesRegex(ValueError, "package identity"):
                    p.verify(archive)
            output = Path(folder) / "no-output"
            cmake = (p.HISTORICAL_JUCE_SHA + p.CORE_SHA).encode()
            with patch.object(p, "JUCE_SHA", "be29c81492b6151c8ea8d14c840e1311963b3a83"), \
                    patch.object(p, "snapshot", return_value={"CMakeLists.txt": (cmake, 0o644)}):
                with self.assertRaisesRegex(ValueError, "pins changed"):
                    p.package("repo", "juce", "core", "a" * 40, output, "1.0.0-dev")
            self.assertFalse(output.exists())

    def test_reserved_tooling_and_false_packager_identity_rejected(self):
        minimal = {"CMakeLists.txt": ((p.JUCE_SHA + p.CORE_SHA).encode(), 0o644)}
        for name in (p.VERIFIER, p.PACKAGE_README, p.MANIFEST):
            with patch.object(p, "snapshot", return_value={**minimal, name: (b"collision", 0o644)}):
                with self.assertRaisesRegex(ValueError, "Reserved"):
                    p.package("repo", "juce", "core", "a" * 40, Path("unused"), "1.0.0-dev")
        with patch.object(p, "snapshot", side_effect=[minimal, {"scripts/package_source.py": (b"wrong", 0o644)}]):
            with self.assertRaisesRegex(ValueError, "Running packager differs"):
                p.package("repo", "juce", "core", "a" * 40, Path("unused"), "1.0.0-dev")

    def test_101_creation_rejects_version_mismatch_before_output(self):
        files = {"CMakeLists.txt": (b"project(VDX7_JUCE VERSION 1.0.0 LANGUAGES C CXX)\n", 0o644),
                 "installer/windows/VDX7.iss": (b'#define AppVersion "1.0.1"\n', 0o644)}
        with tempfile.TemporaryDirectory() as folder, patch.object(p, "snapshot", return_value=files):
            output = Path(folder) / "rejected"
            with self.assertRaisesRegex(ValueError, "versions must both match"):
                p.package("repo", "juce", "core", "a" * 40, output)
            self.assertFalse(output.exists())

    def test_supported_labels_and_negative_controls(self):
        for version in ("1.0.0", "1.0.1"):
            for suffix in ("", "-dev", "-rc1", "-rc12"):
                p.package_identity(version + suffix, False)
            p.package_identity(version, True)
            for suffix in ("-dev", "-rc1"):
                with self.assertRaises(ValueError):
                    p.package_identity(version + suffix, True)
        for label in ("1.0.2", "1.0.1-rc0", "1.0.1-rc01", "1.0.1\n", "../1.0.1", None):
            with self.assertRaises(ValueError):
                p.package_identity(label, False)

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
        authorization = {
            "source_commit": "a" * 40, "packager_commit": "b" * 40,
            "approval_commit": "c" * 40, "package_label": "1.0.0",
            "workflow_ref": p.APPROVED_WORKFLOW_REF, "workflow_git_ref": p.APPROVED_GIT_REF,
            "release_accepted": True, "policy_sha256": "d" * 64,
        }
        # This case isolates old-product/new-tool packaging. Real Git-policy
        # authorization and CLI negatives live in test_release_acceptance.py.
        with tempfile.TemporaryDirectory() as folder, patch.object(p, "snapshot", side_effect=snapshot), \
                patch.object(p, "release_authorization", return_value=authorization) as guard:
            root = Path(folder)
            for accepted in (False, True):
                archive = p.package("wrapper", "juce", "core", "a" * 40,
                                    root / str(accepted), "1.0.0", accepted, "b" * 40,
                                    approval_repo="wrapper", approval_commit="c" * 40,
                                    workflow_ref=p.APPROVED_WORKFLOW_REF, workflow_git_ref=p.APPROVED_GIT_REF)
                p.verify(archive)
                with zipfile.ZipFile(archive) as z:
                    self.assertEqual(z.read("scripts/package_source.py"), old_checker)
                    bundled = root / (str(accepted) + "-verify.py")
                    bundled.write_bytes(z.read(".vdx7-source-tools/package_source.py"))
                    self.assertIn(b".vdx7-source-tools/package_source.py", z.read("SOURCE_PACKAGE_README.txt"))
                subprocess.run([sys.executable, str(bundled), "verify", str(archive)], check=True)
            guard.assert_called_once_with("wrapper", "c" * 40, "a" * 40, "1.0.0",
                                          p.APPROVED_WORKFLOW_REF, p.APPROVED_GIT_REF, True, "b" * 40)

    def fixture(self):
        data = b"example source\n"
        payload = {**fixture.files(), "Source/example.cpp": (data, 0o644)}
        return payload, {
            "schema": 1, "kind": "development-corresponding-source",
            "package_label": "1.0.0-dev", "source_commit": "a" * 40,
            "release_accepted": False,
            "dependencies": {"JUCE": p.JUCE_SHA, "Retromulator": p.CORE_SHA},
            "files": fixture.entries(payload),
        }

    def test_required_file_removed_with_manifest_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            for missing in fixture.REQUIRED:
                with self.subTest(missing=missing):
                    payload, manifest = self.fixture()
                    del payload[missing]
                    manifest["files"] = fixture.entries(payload)
                    archive = Path(folder) / (missing.replace("/", "_") + ".zip")
                    p.write_zip(archive, payload, manifest)
                    with self.assertRaises(ValueError):
                        p.verify(archive)

    def test_historical_100_without_generated_tools_remains_verifiable(self):
        payload, manifest = self.fixture()
        for name in (p.VERIFIER, p.PACKAGE_README):
            del payload[name]
        manifest["files"] = fixture.entries(payload)
        with tempfile.TemporaryDirectory() as folder:
            archive = Path(folder) / "historical.zip"
            p.write_zip(archive, payload, manifest)
            p.verify(archive)
            manifest.update(package_label="1.0.1-dev")
            modern = Path(folder) / "modern.zip"
            p.write_zip(modern, payload, manifest)
            with self.assertRaises(ValueError):
                p.verify(modern)

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
            "1.0.1-dev": "development-corresponding-source",
            "1.0.1-rc2": "release-candidate-corresponding-source",
            "1.0.1": "stable-release-preparation-corresponding-source",
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
        for version in ("1.0.0", "1.0.1"):
            manifest["package_label"] = version
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
