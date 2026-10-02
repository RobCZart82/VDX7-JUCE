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

    def test_product_installer_and_workflow_versions_are_coherent(self):
        root = WORKFLOW.parents[2]
        cmake = (root / "CMakeLists.txt").read_text()
        installer = (root / "installer/windows/VDX7.iss").read_text()
        candidate = (root / ".github/workflows/release-candidate.yml").read_text()
        self.assertRegex(cmake, r"project\(VDX7_JUCE\s+VERSION\s+1\.0\.1\s")
        self.assertIn('#define AppVersion "1.0.1"', installer)
        self.assertIn('OutputBaseFilename=VDX7-{#AppVersion}-Windows-x64-Setup', installer)
        self.assertIn('AppId={{9F5E28E9-1D9A-4B25-99CA-1F0B08C6C087}', installer)
        self.assertIn('--version 1.0.1', self.text)
        self.assertIn('--package-label "1.0.1-$CANDIDATE_LABEL"', candidate)
        self.assertNotIn("1.0.0", self.text)
        self.assertNotIn("1.0.0", candidate)

    def test_active_candidate_and_source_instructions_match_101(self):
        root = WORKFLOW.parents[2]
        guide = (root / "docs/release/CANDIDATE_README_HU_EN.md").read_text()
        notice = (root / "NOTICE.md").read_text()
        source = (root / "docs/release/SOURCE_PACKAGING_1.0.md").read_text()
        for text in (guide, notice, source):
            self.assertIn("1.0.1-dev", text)
            self.assertNotIn("1.0.0-dev", text)
            self.assertNotIn("1.0.0-rcN", text)
        self.assertIn("--package-label 1.0.1-dev", source)
        self.assertEqual(guide.count("1.0.1-rcN"), 2)

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
                      '--commit "$SOURCE_COMMIT"', '--package-label 1.0.1',
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
                      "VDX7-1.0.1-macOS-universal.pkg", "VDX7-1.0.1-Windows-x64-Setup.exe"):
            self.assertIn(token, package)
        self.assertNotIn("--target VDX7_AU", package)

    def test_packaging_records_observed_toolchain_and_checks_pinned_inno(self):
        package = self.jobs["package"]
        for token in ('INNO_SETUP_VERSION: "6.7.1"',
                      'choco install innosetup --version=$env:INNO_SETUP_VERSION --yes --no-progress',
                      'Inno Setup installation failed', 'Inno Setup version verification failed',
                      'python release-tools/scripts/verify_inno_version.py --inno "$iscc" --expected "$env:INNO_SETUP_VERSION"',
                      'python release-tools/scripts/write_build_provenance.py',
                      '--platform Windows-x64', '--platform macOS-universal',
                      '--inno "$iscc" --inno-version "$innoVersion"',
                      'BUILD-INFO-Windows-x64.txt', 'BUILD-INFO-macOS-universal.txt'):
            self.assertIn(token, package)
        self.assertEqual(package.count('write_build_provenance.py'), 2)
        self.assertEqual(package.count('--juce build/_deps/juce-src --core build/_deps/retromulator-src'), 3)
        self.assertIn('Build provenance capture failed', package)
        self.assertNotIn('ProductMajorPart', package)
        self.assertLess(package.index('verify_inno_version.py'), package.index('& $iscc'))

    def test_only_four_user_downloads_are_staged_after_checksum_verification(self):
        assembly = self.jobs["assemble-release-assets"]
        self.assertIn('approval-tools/scripts/stage_release_downloads.py', assembly)
        self.assertLess(assembly.index('sha256sum -c SHA256SUMS.txt'),
                        assembly.index('stage_release_downloads.py'))
        self.assertIn('--assets release-assets --output publishable-downloads', assembly)
        self.assertIn('--package-label "$PACKAGE_LABEL"', assembly)
        # Internal provenance/source evidence must not be dropped to enforce the public count.
        self.assertIn('path: release-assets/*', assembly)
        public = assembly.split('- name: Upload four user downloads', 1)[1]
        for suffix in ('Windows-x64-Setup.exe', 'Windows-x64-Manual.zip',
                       'macOS-universal.pkg', 'macOS-universal-Manual.zip'):
            self.assertIn('publishable-downloads/VDX7-${{ needs.authorize-package.outputs.package_label }}-' + suffix, public)
        self.assertEqual(public.count('publishable-downloads/'), 4)
        self.assertNotIn('corresponding-source.zip', public)
        self.assertNotIn('BUILD-INFO', public)
        self.assertNotIn('SHA256SUMS', public)
        self.assertNotIn('publishable-downloads/*', public)

    def test_separate_source_allowlist_uses_guarded_tuple_and_checksum_validation(self):
        assembly = self.jobs["assemble-release-assets"]
        self.assertIn("approval-tools/scripts/stage_source_downloads.py", assembly)
        self.assertLess(assembly.index("sha256sum -c SHA256SUMS.txt"),
                        assembly.index("stage_source_downloads.py"))
        for token in ('--package-label "$PACKAGE_LABEL"', '--source-commit "$SOURCE_COMMIT"',
                      '--packager-commit "$PACKAGER_COMMIT"', '--approval-commit "$APPROVAL_COMMIT"',
                      'source_args+=(--release-accepted)'):
            self.assertIn(token, assembly)
        upload = assembly.split("- name: Upload separate source downloads", 1)[1].split(
            "- name: Upload four user downloads", 1)[0]
        self.assertEqual(upload.count("source-downloads/"), 2)
        self.assertIn("source-downloads/SHA256SUMS.txt", upload)
        self.assertNotIn("source-downloads/*", upload)
        self.assertNotIn("Setup.exe", upload)
        self.assertNotIn("gh release", self.text)

    def test_upgrade_smoke_is_hosted_only_and_preserves_published_layout(self):
        root = WORKFLOW.parents[2]
        script = (root / "scripts/smoke_windows_upgrade.ps1").read_text()
        for token in ('$env:GITHUB_ACTIONS -ne "true"', '$env:RUNNER_OS -ne "Windows"',
                      '$env:RUNNER_ENVIRONMENT -ne "github-hosted"',
                      "670b18160fd67c3534c5ca2914353762a730e324765189750434fd8e202206e3",
                      'Assert-RegisteredVersion "1.0.0"', 'Assert-RegisteredVersion "1.0.1"',
                      "Assert-PreservedUserFiles", "Upgraded plugin payload differs",
                      "Upgrade smoke refuses existing installation"):
            self.assertIn(token, script)
        self.assertLess(script.index("Published 1.0.0 installer digest mismatch"),
                        script.index("Invoke-Installer $oldSetup"))
        self.assertNotIn("Remove-Item", script)
        package = self.jobs["package"]
        self.assertIn("& release-tools/scripts/smoke_windows_upgrade.ps1", package)
        self.assertIn("Get-Content -LiteralPath $upgradeEvidence", package)
        windows = (root / ".github/workflows/build-windows.yml").read_text()
        self.assertIn("[System.Management.Automation.Language.Parser]::ParseFile", windows)
        self.assertNotIn("Invoke-Installer", windows)
        installer = (root / "installer/windows/VDX7.iss").read_text()
        self.assertNotIn("UninstallFilesDir=", installer)


if __name__ == "__main__":
    unittest.main()
