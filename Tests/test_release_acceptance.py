"""ROM-free approval guards tested against real, disposable Git objects."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile


SCRIPT = Path(__file__).parents[1] / "scripts/package_source.py"
spec = importlib.util.spec_from_file_location("release_acceptance_packager", SCRIPT)
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)


class ReleaseAcceptanceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.repo = self.root / "repository"
        self.repo.mkdir()
        self.env = dict(os.environ, GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
        for key in ("GIT_DIR", "GIT_WORK_TREE", "GIT_INDEX_FILE", "GIT_OBJECT_DIRECTORY",
                    "GIT_ALTERNATE_OBJECT_DIRECTORIES", "GIT_NO_REPLACE_OBJECTS"):
            self.env.pop(key, None)
        self.git("init", "-b", "main")
        self.git("config", "user.name", "Release Guard Fixture")
        self.git("config", "user.email", "fixture@example.invalid")
        self.git("config", "commit.gpgSign", "false")
        self.git("config", "core.autocrlf", "false")
        self.old_checker = b"raise SystemExit('old product checker is intentionally obsolete')\n"
        self.write("CMakeLists.txt", ("project(VDX7_JUCE VERSION 1.0.1 LANGUAGES C CXX)\n"
                                    + p.JUCE_SHA + "\n" + p.CORE_SHA + "\n").encode())
        self.write("installer/windows/VDX7.iss", b'#define AppVersion "1.0.1"\n')
        for name in ("LICENSE.txt", "NOTICE.md", "THIRD_PARTY.md"):
            self.write(name, b"Synthetic fixture notice\n")
        self.write("scripts/package_source.py", self.old_checker)
        self.write_policy({"schema": 1, "approved_release": None})
        self.source = self.commit("A: exact old product, no approval")
        self.write("scripts/package_source.py", SCRIPT.read_bytes().replace(b"\r\n", b"\n"))
        self.tool = self.commit("B: independently frozen current packager")
        self.approved = {
            "package_label": "1.0.0", "source_commit": self.source,
            "packager_commit": self.tool, "workflow_ref": p.APPROVED_WORKFLOW_REF,
        }
        self.approval = self.approve()

    def git(self, *args, input=None):
        return subprocess.check_output(
            ["git", "-c", "core.hooksPath=" + str(self.root / "no-hooks"),
             "-C", str(self.repo), *args], input=input, env=self.env,
            stderr=subprocess.STDOUT)

    def write(self, name, data):
        destination = self.repo / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)

    def write_policy(self, policy):
        data = (json.dumps(policy, sort_keys=True, indent=2) + "\n").encode()
        self.write(p.APPROVAL_PATH, data)
        return data

    def commit(self, message):
        self.git("add", "-A")
        self.git("commit", "--allow-empty", "-m", message)
        return self.git("rev-parse", "HEAD").decode().strip()

    def approve(self, changes=None):
        record = {**self.approved, **(changes or {})}
        self.write_policy({"schema": 1, "approved_release": record})
        return self.commit("C: separate explicit approval")

    def authorize(self, **changes):
        arguments = {
            "repo": self.repo, "approval_commit": self.approval,
            "source_commit": self.source, "package_label": "1.0.0",
            "workflow_ref": p.APPROVED_WORKFLOW_REF,
            "workflow_git_ref": p.APPROVED_GIT_REF, "accepted": True,
        }
        arguments.update(changes)
        return p.release_authorization(**arguments)

    def test_exact_tuple_resolves_frozen_tool_and_policy_digest(self):
        actual = self.authorize()
        policy = self.git("show", self.approval + ":" + p.APPROVAL_PATH)
        expected = {
            **self.approved, "approval_commit": self.approval,
            "workflow_git_ref": p.APPROVED_GIT_REF, "release_accepted": True,
            "policy_sha256": hashlib.sha256(policy).hexdigest(),
        }
        self.assertEqual(actual, expected)
        self.assertEqual(self.authorize(packager_commit=self.tool), expected)
        self.assertNotEqual(self.tool, self.approval)

    def test_101_approval_requires_its_own_exact_tuple(self):
        self.approval = self.approve({"package_label": "1.0.1"})
        actual = self.authorize(package_label="1.0.1")
        self.assertEqual(actual["package_label"], "1.0.1")
        self.assertEqual(actual["packager_commit"], self.tool)
        with self.assertRaises(ValueError):
            self.authorize(package_label="1.0.0")

    def test_101_preparation_rejects_old_project_or_installer_before_outputs(self):
        for path, content in (
                ("CMakeLists.txt", b"project(VDX7_JUCE VERSION 1.0.0 LANGUAGES C CXX)\n"),
                ("installer/windows/VDX7.iss", b'#define AppVersion "1.0.0"\n')):
            with self.subTest(path=path):
                self.git("checkout", self.approval, "--", "CMakeLists.txt", "installer/windows/VDX7.iss")
                self.write(path, content)
                bad = self.commit("wrong source version")
                with self.assertRaisesRegex(ValueError, "versions must both match"):
                    self.authorize(approval_commit=bad, source_commit=bad,
                                   package_label="1.0.1", accepted=False)

    def test_preparation_has_no_policy_read_or_acceptance(self):
        workflow_path = p.APPROVED_WORKFLOW_REF.rsplit("@", 1)[0]
        cases = [(git_ref, workflow_path + "@" + git_ref) for git_ref in (
            p.APPROVED_GIT_REF, "refs/heads/rehearsal", "refs/tags/prep-test")]
        cases.append(("refs/heads/rehearsal",
                      "fixture-user/VDX7-JUCE/.github/workflows/prepare-stable-package.yml@refs/heads/rehearsal"))
        for git_ref, workflow_ref in cases:
            with self.subTest(git_ref=git_ref, workflow_ref=workflow_ref), patch.object(
                    p, "snapshot", side_effect=AssertionError("preparation must not read approval")):
                context = self.authorize(
                    accepted=False, workflow_git_ref=git_ref, workflow_ref=workflow_ref)
                self.assertIs(context["release_accepted"], False)
                self.assertEqual(context["packager_commit"], self.approval)
                self.assertNotIn("policy_sha256", context)

    def test_requested_tuple_mismatches_are_rejected(self):
        cases = {
            "product": {"source_commit": self.tool},
            "tool": {"packager_commit": self.source},
            "dev": {"package_label": "1.0.0-dev"},
            "rc": {"package_label": "1.0.0-rc1"},
            "version": {"package_label": "1.0.1"},
            "workflow": {"workflow_ref": p.APPROVED_WORKFLOW_REF.replace(
                "prepare-stable-package.yml", "other.yml")},
            "repository": {"workflow_ref": p.APPROVED_WORKFLOW_REF.replace(
                "RobCZart82/VDX7-JUCE", "other/VDX7-JUCE")},
            "branch": {"workflow_git_ref": "refs/heads/rehearsal", "workflow_ref":
                p.APPROVED_WORKFLOW_REF.replace("refs/heads/main", "refs/heads/rehearsal")},
            "tag": {"workflow_git_ref": "refs/tags/v1.0.0", "workflow_ref":
                p.APPROVED_WORKFLOW_REF.replace("refs/heads/main", "refs/tags/v1.0.0")},
            "injected-output": {"workflow_git_ref": "refs/heads/main\nrelease_accepted=true"},
            "self-tool": {"packager_commit": self.approval},
            "wrong-head": {"approval_commit": self.tool},
        }
        for name, changes in cases.items():
            with self.subTest(name=name), self.assertRaises(ValueError):
                self.authorize(**changes)

    def test_changed_policy_fields_and_unknown_keys_are_rejected(self):
        for changes in ({"source_commit": self.tool}, {"packager_commit": self.source},
                        {"package_label": "1.0.1"}, {"workflow_ref": "other/workflow@refs/heads/main"},
                        {"extra": "unreviewed"}):
            self.approval = self.approve(changes)
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                self.authorize(packager_commit=self.tool)

    def test_null_missing_and_dirty_policy_fail_closed(self):
        self.write_policy({"schema": 1, "approved_release": None})
        self.approval = self.commit("null authority")
        # Neither a dirty fixed-path record nor a different untracked policy can
        # authorize the original committed null-policy generation.
        self.write_policy({"schema": 1, "approved_release": self.approved})
        self.write("other-policy.json", json.dumps(self.approved).encode())
        with self.assertRaises(ValueError):
            self.authorize()
        self.assertIsNone(json.loads(p.snapshot(
            self.repo, self.approval, (p.APPROVAL_PATH,))[p.APPROVAL_PATH][0])["approved_release"])
        (self.repo / p.APPROVAL_PATH).unlink()
        self.approval = self.commit("fixed policy path absent")
        with self.assertRaises(ValueError):
            self.authorize()

    def test_dirty_policy_does_not_revoke_committed_valid_tuple(self):
        self.write_policy({"schema": 1, "approved_release": None})
        self.assertIs(self.authorize()["release_accepted"], True)

    def test_policy_json_requires_strict_schema_and_unique_keys(self):
        policy = {"schema": 1, "approved_release": self.approved}
        malformed = [b"not JSON", b"null", b"[]", b"{}",
                     json.dumps({**policy, "schema": True}).encode(),
                     json.dumps({**policy, "schema": "1"}).encode(),
                     json.dumps({**policy, "schema": 1.0}).encode(),
                     json.dumps({**policy, "extra": 1}).encode(),
                     json.dumps({"schema": 1, "approved_release": []}).encode(),
                     json.dumps({"schema": 1, "approved_release": True}).encode(),
                     b'{"schema":1,"schema":1,"approved_release":' + json.dumps(self.approved).encode() + b'}',
                     b'{"schema":1,"approved_release":null,"approved_release":'
                     + json.dumps(self.approved).encode() + b'}']
        duplicate_record = json.dumps(self.approved).replace(
            '"package_label": "1.0.0"', '"package_label": "1.0.0", "package_label": "1.0.0"')
        malformed.append(('{"schema":1,"approved_release":' + duplicate_record + '}').encode())
        for index, data in enumerate(malformed):
            self.write(p.APPROVAL_PATH, data)
            self.approval = self.commit("invalid policy " + str(index))
            with self.subTest(index=index), self.assertRaises(ValueError):
                self.authorize()

    def test_acceptance_boolean_and_sha_types_are_strict(self):
        for value in (1, 0, "true", "false", None):
            with self.subTest(accepted=value), self.assertRaises(ValueError):
                self.authorize(accepted=value)
        for field in ("approval_commit", "source_commit", "packager_commit"):
            for value in ("HEAD", "a" * 39, "A" * 40, 1, self.source + "\n"):
                with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                    self.authorize(**{field: value})

    def test_off_branch_source_and_tool_are_not_authorized(self):
        orphan = self.git("commit-tree", self.tool + "^{tree}", input=b"unrelated root\n").decode().strip()
        for field in ("source_commit", "packager_commit"):
            self.approval = self.approve({field: orphan})
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "ancestors"):
                self.authorize(**{field: orphan})
            # A replacement can forge a second parent while keeping the policy
            # tree and reported approval SHA unchanged. Real-object ancestry
            # must still reject the unrelated source/tool after this graft.
            parent = self.git("rev-parse", self.approval + "^").decode().strip()
            self.git("replace", "--graft", self.approval, parent, orphan)
            self.git("merge-base", "--is-ancestor", orphan, self.approval)
            with self.subTest(field=field, forged_ancestry=True), self.assertRaisesRegex(ValueError, "ancestors"):
                self.authorize(**{field: orphan})
            self.git("replace", "-d", self.approval)

    def test_same_checker_bytes_do_not_authorize_different_tool_commit(self):
        self.write("same-tool-marker.txt", b"different Git identity; identical script\n")
        other_tool = self.commit("same tool bytes, different commit")
        self.assertEqual(p.snapshot(self.repo, self.tool, ("scripts/package_source.py",)),
                         p.snapshot(self.repo, other_tool, ("scripts/package_source.py",)))
        self.approval = self.approve()
        with self.assertRaisesRegex(ValueError, "Packager commit"):
            self.authorize(packager_commit=other_tool)

    def test_git_replacement_cannot_swap_null_policy_or_blob(self):
        null_bytes = self.git("show", self.source + ":" + p.APPROVAL_PATH)
        approved_bytes = self.git("show", self.approval + ":" + p.APPROVAL_PATH)
        null_blob = self.git("rev-parse", self.source + ":" + p.APPROVAL_PATH).decode().strip()
        approved_blob = self.git("rev-parse", self.approval + ":" + p.APPROVAL_PATH).decode().strip()
        self.git("replace", self.source, self.approval)
        self.assertEqual(self.git("show", self.source + ":" + p.APPROVAL_PATH), approved_bytes)
        self.assertEqual(p.snapshot(self.repo, self.source, (p.APPROVAL_PATH,))[p.APPROVAL_PATH][0], null_bytes)
        self.git("replace", "-d", self.source)
        # Test cat-file's direct blob reads too, not only commit/tree selection.
        self.git("replace", null_blob, approved_blob)
        self.assertEqual(p.snapshot(self.repo, self.source, (p.APPROVAL_PATH,))[p.APPROVAL_PATH][0], null_bytes)
        self.git("replace", "-d", null_blob)
        self.git("update-ref", "refs/heads/main", self.source)
        self.git("replace", self.source, self.approval)
        with self.assertRaises(ValueError):
            self.authorize(approval_commit=self.source)

    def cli(self, output, **changes):
        fields = {"repo": self.repo, "approval-commit": self.approval,
                  "commit": self.source, "package-label": "1.0.0",
                  "workflow-ref": p.APPROVED_WORKFLOW_REF, "workflow-git-ref": p.APPROVED_GIT_REF}
        fields.update(changes)
        command = [sys.executable, str(SCRIPT), "authorize", "--release-accepted"]
        for key, value in fields.items():
            command.extend(["--" + key, str(value)])
        command.extend(["--github-output", str(output)])
        return subprocess.run(command, env=self.env, capture_output=True, text=True)

    def test_cli_writes_outputs_only_after_successful_guard(self):
        output = self.root / "github-output.txt"
        passed = self.cli(output)
        self.assertEqual(passed.returncode, 0, passed.stderr)
        actual = dict(line.split("=", 1) for line in output.read_text().splitlines())
        self.assertEqual(actual["release_accepted"], "true")
        self.assertEqual(actual["packager_commit"], self.tool)
        self.assertEqual(actual["approval_commit"], self.approval)
        previous = output.read_bytes()
        failed = self.cli(output, commit=self.tool)
        self.assertNotEqual(failed.returncode, 0)
        self.assertEqual(output.read_bytes(), previous)
        absent = self.root / "failure-created.txt"
        self.assertNotEqual(self.cli(absent, commit=self.tool).returncode, 0)
        self.assertFalse(absent.exists())

    def test_accepted_create_rejects_before_package_output(self):
        self.write_policy({"schema": 1, "approved_release": None})
        self.approval = self.commit("unapproved create")
        output = self.root / "should-not-exist"
        with self.assertRaises(ValueError):
            p.package(self.repo, "unused-juce", "unused-core", self.source, output,
                      "1.0.0", True, self.tool, approval_repo=self.repo,
                      approval_commit=self.approval, workflow_ref=p.APPROVED_WORKFLOW_REF,
                      workflow_git_ref=p.APPROVED_GIT_REF)
        self.assertFalse(output.exists())

    def test_create_rejects_non_boolean_flags_before_output(self):
        # Library callers, not just argparse, must not turn truthy strings or
        # integers into accepted metadata and fail only at later verification.
        for index, accepted in enumerate((1, 0, "true", "false", None)):
            output = self.root / ("invalid-flag-" + str(index))
            with self.subTest(accepted=accepted), self.assertRaises(ValueError):
                p.package(self.repo, "unused-juce", "unused-core", self.source, output,
                          "1.0.0", accepted, self.tool, approval_repo=self.repo,
                          approval_commit=self.approval, workflow_ref=p.APPROVED_WORKFLOW_REF,
                          workflow_git_ref=p.APPROVED_GIT_REF)
            self.assertFalse(output.exists())

    def make_accepted_archive(self, label="1.0.0"):
        original_snapshot = p.snapshot
        def minimal_dependencies(repo, sha, paths=()):
            if repo == "fixture-juce":
                self.assertEqual(sha, p.JUCE_SHA)
                return {"LICENSE.md": (b"Synthetic JUCE notice\n", 0o644)}
            if repo == "fixture-core":
                self.assertEqual(sha, p.CORE_SHA)
                return {"source/dx7Lib/dx7.cpp": (b"// synthetic core\n", 0o644),
                        "LICENSE.txt": (b"Synthetic core notice\n", 0o644)}
            return original_snapshot(repo, sha, paths)
        with patch.object(p, "snapshot", side_effect=minimal_dependencies):
            return p.package(self.repo, "fixture-juce", "fixture-core", self.source,
                             self.root / "accepted-package", label, True, self.tool,
                             approval_repo=self.repo, approval_commit=self.approval,
                             workflow_ref=p.APPROVED_WORKFLOW_REF, workflow_git_ref=p.APPROVED_GIT_REF)

    def test_real_approval_package_keeps_old_product_and_git_free_bundled_verifier(self):
        archive = self.make_accepted_archive()
        extracted = self.root / "extracted"
        with zipfile.ZipFile(archive) as contents:
            manifest = json.loads(contents.read(p.MANIFEST))
            self.assertEqual(contents.read("scripts/package_source.py"), self.old_checker)
            self.assertEqual(manifest["release_approval"], {
                key: value for key, value in self.authorize().items() if key != "release_accepted"})
            contents.extractall(extracted)
        self.assertFalse((extracted / ".git").exists())
        no_git_env = dict(self.env, PATH=str(self.root / "no-executables"))
        verified = subprocess.run([sys.executable, str(extracted / p.VERIFIER), "verify", str(archive)],
                                  cwd=extracted, env=no_git_env, capture_output=True, text=True)
        self.assertEqual(verified.returncode, 0, verified.stderr)
        self.assertIn("not publisher authentication", verified.stdout)

    def test_101_approved_archive_and_git_free_bundled_verifier(self):
        self.approval = self.approve({"package_label": "1.0.1"})
        archive = self.make_accepted_archive("1.0.1")
        extracted = self.root / "extracted"
        with zipfile.ZipFile(archive) as contents:
            manifest = json.loads(contents.read(p.MANIFEST))
            self.assertEqual(manifest["package_label"], "1.0.1")
            self.assertEqual(manifest["release_approval"]["packager_commit"], self.tool)
            contents.extractall(extracted)
        no_git_env = dict(self.env, PATH=str(self.root / "no-executables"))
        verified = subprocess.run([sys.executable, str(extracted / p.VERIFIER), "verify", str(archive)],
                                  cwd=extracted, env=no_git_env, capture_output=True, text=True)
        self.assertEqual(verified.returncode, 0, verified.stderr)
        self.assertIn("not publisher authentication", verified.stdout)

    def test_approval_proof_tampering_is_rejected_after_rehash(self):
        archive = self.make_accepted_archive()
        with zipfile.ZipFile(archive) as contents:
            baseline = json.loads(contents.read(p.MANIFEST))
            files = {entry["path"]: (contents.read(entry["path"]), entry["mode"])
                     for entry in baseline["files"]}
        cases = {
            "product": {"source_commit": self.tool},
            "tool": {"packager_commit": self.source},
            "version": {"package_label": "1.0.0-rc1"},
            "workflow": {"workflow_ref": p.APPROVED_WORKFLOW_REF.replace("main", "other")},
            "tag": {"workflow_git_ref": "refs/tags/v1.0.0"},
            "digest": {"policy_sha256": "not-a-sha256"},
            "self-reference": {"approval_commit": self.tool},
            "unknown": {"extra": True},
        }
        for name, changes in cases.items():
            candidate = json.loads(json.dumps(baseline))
            candidate["release_approval"].update(changes)
            target = self.root / (name + ".zip")
            p.write_zip(target, files, candidate)
            with self.subTest(name=name), self.assertRaises(ValueError):
                p.verify(target)
        candidate = json.loads(json.dumps(baseline))
        del candidate["release_approval"]["policy_sha256"]
        target = self.root / "missing-proof-field.zip"
        p.write_zip(target, files, candidate)
        with self.assertRaises(ValueError):
            p.verify(target)


if __name__ == "__main__":
    unittest.main()
