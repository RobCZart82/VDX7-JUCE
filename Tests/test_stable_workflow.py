"""ROM-free static contracts for stable packaging's authorization boundary."""
from pathlib import Path
import re
import unittest


WORKFLOW = Path(__file__).parents[1] / ".github/workflows/prepare-stable-package.yml"


def jobs(text):
    """Split fixed-format top-level job blocks without a YAML dependency."""
    body = text.split("\njobs:\n", 1)[1]
    starts = list(re.finditer(r"^  ([a-z][a-z0-9-]*):\n", body, re.MULTILINE))
    return {match.group(1): body[match.start():starts[index + 1].start()
                                if index + 1 < len(starts) else len(body)]
            for index, match in enumerate(starts)}


class StableWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.text = WORKFLOW.read_text(encoding="utf-8")
        self.jobs = jobs(self.text)

    def test_trusted_authorization_precedes_platform_jobs(self):
        self.assertIn("authorize-package", self.jobs)
        guard = self.jobs["authorize-package"]
        self.assertLess(self.text.index("  authorize-package:\n"), self.text.index("  package:\n"))
        for token in ("ref: ${{ github.sha }}", "path: approval-tools", "fetch-depth: 0",
                      "persist-credentials: false", "WORKFLOW_COMMIT: ${{ github.sha }}",
                      "WORKFLOW_GIT_REF: ${{ github.ref }}", "WORKFLOW_REF: ${{ github.workflow_ref }}",
                      'git --no-replace-objects -C approval-tools rev-parse HEAD',
                      'git --no-replace-objects -C approval-tools merge-base --is-ancestor "$SOURCE_COMMIT" origin/main',
                      "python3 approval-tools/scripts/package_source.py authorize"):
            self.assertIn(token, guard)
        self.assertNotIn("cmake", guard)
        self.assertNotIn("release-assets", guard)
        self.assertIn("    needs: authorize-package\n", self.jobs["package"])
        self.assertIn("    needs: [authorize-package, package]\n", self.jobs["assemble-release-assets"])

    def test_authorize_interface_and_normalized_outputs(self):
        self.assertIn("authorize-package", self.jobs)
        guard = self.jobs["authorize-package"]
        for token in ('--repo approval-tools', '--approval-commit "$WORKFLOW_COMMIT"',
                      '--commit "$SOURCE_COMMIT"', '--package-label 1.0.0',
                      '--workflow-ref "$WORKFLOW_REF"', '--workflow-git-ref "$WORKFLOW_GIT_REF"',
                      '--github-output "$GITHUB_OUTPUT"'):
            self.assertIn(token, guard)
        for name in ("source_commit", "packager_commit", "approval_commit", "package_label",
                     "workflow_ref", "workflow_git_ref", "release_accepted"):
            self.assertIn(f"{name}: ${{{{ steps.authorization.outputs.{name} }}}}", guard)

    def test_downstream_never_consumes_raw_dispatch_acceptance(self):
        for name in ("package", "assemble-release-assets"):
            with self.subTest(job=name):
                job = self.jobs[name]
                self.assertNotIn("${{ inputs.", job)
                self.assertNotIn("${{ github.sha }}", job)
                self.assertIn("needs.authorize-package.outputs.source_commit", job)
                self.assertIn("needs.authorize-package.outputs.release_accepted == 'true'", job)
        self.assertEqual(self.text.count("${{ inputs."), 2)

    def test_matrix_uses_frozen_packager_and_committed_approval(self):
        package = self.jobs["package"]
        for token in ("ref: ${{ needs.authorize-package.outputs.source_commit }}",
                      "ref: ${{ needs.authorize-package.outputs.packager_commit }}",
                      "ref: ${{ needs.authorize-package.outputs.approval_commit }}",
                      "path: release-tools", "path: approval-tools",
                      "python release-tools/scripts/package_source.py create",
                      '--packager-commit "$PACKAGER_COMMIT"',
                      '--approval-repo approval-tools', '--approval-commit "$APPROVAL_COMMIT"',
                      '--workflow-ref "$WORKFLOW_REF"', '--workflow-git-ref "$WORKFLOW_GIT_REF"',
                      '--package-label "$PACKAGE_LABEL"'):
            self.assertIn(token, package)
        self.assertIn('[[ "$ACCEPTED_FOR_PUBLICATION" == "true" ]]', package)
        self.assertIn('package_args+=(--release-accepted)', package)
        for label in ("Source commit:", "Release packager commit:", "Approval/workflow commit:",
                      "Workflow ref:", "Workflow Git ref:"):
            self.assertEqual(package.count(label), 2, label)

    def test_assembly_uses_guarded_checksum_tooling_and_read_only_permissions(self):
        assembly = self.jobs["assemble-release-assets"]
        self.assertIn("ref: ${{ needs.authorize-package.outputs.approval_commit }}", assembly)
        self.assertIn("path: approval-tools", assembly)
        self.assertIn("python3 approval-tools/scripts/write_sha256_manifest.py", assembly)
        self.assertNotIn("python scripts/write_sha256_manifest.py", assembly)
        self.assertIn("permissions:\n  contents: read\n", self.text)
        self.assertNotIn("contents: write", self.text)
        self.assertNotIn("gh release", self.text)
        self.assertIsNone(re.search(r"\bgit\s+(?!\-\-no-replace-objects\b)", self.text))

    def test_existing_vst3_installer_and_test_scope_is_retained(self):
        package = self.jobs["package"]
        for token in ("-DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\"", "-A x64",
                      "--target VDX7_VST3 vdx7_ci_checks", "--no-tests=error",
                      "lipo", "codesign --verify --deep --strict", "pkgutil --payload-files",
                      "Uninstaller smoke test failed",
                      "VDX7-1.0.0-macOS-universal.pkg", "VDX7-1.0.0-Windows-x64-Setup.exe"):
            self.assertIn(token, package)
        self.assertNotIn("--target VDX7_AU", package)

    def test_packaging_records_observed_toolchain_and_checks_pinned_inno(self):
        package = self.jobs["package"]
        for token in ('INNO_SETUP_VERSION: "6.7.1"',
                      'choco install innosetup --version=$env:INNO_SETUP_VERSION --yes --no-progress',
                      'Inno Setup installation failed', 'Inno Setup version mismatch',
                      'python release-tools/scripts/write_build_provenance.py',
                      '--platform Windows-x64', '--platform macOS-universal',
                      '--inno "$iscc" --inno-version "$innoVersion"',
                      'BUILD-INFO-Windows-x64.txt', 'BUILD-INFO-macOS-universal.txt'):
            self.assertIn(token, package)
        self.assertEqual(package.count('write_build_provenance.py'), 2)
        self.assertEqual(package.count('--juce build/_deps/juce-src --core build/_deps/retromulator-src'), 3)
        self.assertIn('Build provenance capture failed', package)


if __name__ == "__main__":
    unittest.main()
