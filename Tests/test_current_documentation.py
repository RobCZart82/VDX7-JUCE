"""Guard current navigation; dated historical evidence remains untouched."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CurrentDocumentationTests(unittest.TestCase):
    def test_bilingual_downloads_target_current_release(self):
        for name in ('README.md', 'README_HU.md', 'docs/guides/GUIDE_EN.md',
                     'docs/guides/GUIDE_HU.md'):
            with self.subTest(name=name):
                text = (ROOT / name).read_text()
                self.assertIn('/releases/tag/v1.2.0', text)
                self.assertIn('Source/', text)
                self.assertNotIn('/releases/tag/v1.0.1-source', text)

    def test_notice_and_dependency_guide_identify_published_source(self):
        for name in ('NOTICE.md', 'docs/guides/SOURCE_DEPENDENCIES.md'):
            text = (ROOT / name).read_text()
            self.assertIn('7e2271352d0f05fdad39bae26fdec764064fa1e1', text)
            self.assertNotIn('stabil 1.2.0 kiadás még nincs jóváhagyva', text)
        self.assertIn('FINAL_120_OWNER_ACCEPTANCE_20261014.md',
                      (ROOT / 'docs/README.md').read_text())
