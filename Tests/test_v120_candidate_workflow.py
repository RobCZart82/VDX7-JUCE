"""Static safety contracts for the non-publishing v1.2.0 candidate packager."""
from pathlib import Path
import unittest


ROOT = Path(__file__).parents[1]
WORKFLOW = ROOT / ".github/workflows/prepare-v1.2.0-candidate.yml"


class V120CandidateWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.text = WORKFLOW.read_text(encoding="utf-8")

    def test_workflow_is_manual_exact_main_and_read_only(self):
        for token in ("workflow_dispatch:", "WORKFLOW_REF: ${{ github.ref }}",
                      '[[ "$WORKFLOW_REF" == "refs/heads/main" ]]',
                      '[[ "$SOURCE_COMMIT" == "$WORKFLOW_COMMIT" ]]',
                      'test "$(git --no-replace-objects rev-parse HEAD)" = "$SOURCE_COMMIT"',
                      "permissions:\n  contents: read"):
            self.assertIn(token, self.text)
        for forbidden in ("contents: write", "gh release", "git tag", "softprops/action-gh-release",
                          "actions/create-release", "release-accepted", "--release-accepted"):
            self.assertNotIn(forbidden, self.text)

    def test_candidate_label_controls_all_outputs_and_four_requested_formats(self):
        for token in ("^rc[1-9][0-9]*$", "PACKAGE_LABEL: 1.2.0-${{ inputs.candidate_label }}",
                      "VDX7-${PACKAGE_LABEL}-Windows-x64-Setup.exe",
                      "VDX7-${PACKAGE_LABEL}-Windows-x64-Manual.zip",
                      "VDX7-${PACKAGE_LABEL}-macOS-universal.pkg",
                      "VDX7-${PACKAGE_LABEL}-macOS-universal-Manual.zip",
                      "VDX7-${PACKAGE_LABEL}-${SOURCE_COMMIT}-corresponding-source.zip"):
            self.assertIn(token, self.text)
        self.assertIn("VDX7-${{ env.PACKAGE_LABEL }}-Four-Downloads-${{ env.SOURCE_COMMIT }}-PREPARATION", self.text)

    def test_both_platforms_build_verify_and_record_unsigned_scope(self):
        for token in ("macos-14", "windows-2022", "-DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\"",
                      "-A x64", 'unittest discover -s Tests -p "test_*.py"',
                      "VDX7_VST3 vdx7_ci_checks", "ctest --test-dir build -C Release",
                      "codesign --verify --deep --strict", "pkgutil --payload-files",
                      "Installer smoke test failed", "Uninstaller smoke test failed",
                      "PREPARATION ONLY; not accepted or approved for publication.",
                      "Not Developer ID signed or notarized", "unknown-publisher warning",
                      "Firmware: no Yamaha ROM or firmware included.",
                      "write_build_provenance.py", "write_sha256_manifest.py", "sha256sum -c SHA256SUMS.txt"):
            self.assertIn(token, self.text)

    def test_legacy_101_workflows_remain_separate_and_unchanged_in_scope(self):
        stable = (ROOT / ".github/workflows/prepare-stable-package.yml").read_text()
        legacy_candidate = (ROOT / ".github/workflows/release-candidate.yml").read_text()
        self.assertIn("--package-label 1.0.1", stable)
        self.assertIn('--package-label "1.0.1-$CANDIDATE_LABEL"', legacy_candidate)
        self.assertNotIn("prepare-stable-package.yml", self.text)


if __name__ == "__main__":
    unittest.main()
