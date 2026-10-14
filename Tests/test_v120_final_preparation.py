"""Final display identity is independent of publication authorization."""
from pathlib import Path
import unittest


class FinalPreparationTests(unittest.TestCase):
    def test_final_packages_are_explicit_and_non_publishing(self):
        root = Path(__file__).resolve().parents[1]
        text = (root / '.github/workflows/prepare-v1.2.0-candidate.yml').read_text()
        for token in ('default: candidate', '- final', "PACKAGE_LABEL=1.2.0",
                      "echo 'CONFIGURE_RELEASE=ON'", "echo 'CONFIGURE_RELEASE=OFF'",
                      '-DVDX7_RELEASE_BUILD=${CONFIGURE_RELEASE}',
                      'Resolve final-version assembly label', 'contents: read',
                      'not accepted or approved for publication.'):
            self.assertIn(token, text)
        self.assertEqual(text.count("echo 'PACKAGE_LABEL=1.2.0'"), 2)
        for forbidden in ('contents: write', 'gh release', 'git tag', '--release-accepted'):
            self.assertNotIn(forbidden, text)
