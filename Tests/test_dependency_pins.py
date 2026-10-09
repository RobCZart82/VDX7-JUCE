"""Keep active dependency creation and public build instructions coherent."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).parents[1]


class DependencyPinTests(unittest.TestCase):
    def test_current_creation_pin_matches_cmake_and_active_guides(self):
        cmake = (ROOT / "CMakeLists.txt").read_text()
        declaration = re.search(r"FetchContent_Declare\(\s*JUCE\b(.*?)\)", cmake, re.S).group(1)
        pin = re.search(r"\bGIT_TAG\s+([0-9a-f]{40})\b", declaration).group(1)
        script = (ROOT / "scripts/package_source.py").read_text()
        self.assertEqual(re.search(r'^JUCE_SHA = "([0-9a-f]{40})"$', script, re.M).group(1), pin)
        guide = (ROOT / "docs/guides/SOURCE_DEPENDENCIES.md").read_text()
        self.assertIn("Revision: " + pin + " (JUCE ", guide)
        self.assertIn("Exact revision: " + pin + ".", (ROOT / "THIRD_PARTY.md").read_text())

    def test_private_capture_is_not_a_public_ctest_or_workflow_command(self):
        self.assertNotIn("--compatibility-fingerprint", (ROOT / "CMakeLists.txt").read_text())
        for path in (ROOT / ".github/workflows").glob("*.yml"):
            self.assertNotIn("--compatibility-fingerprint", path.read_text(), path.name)
