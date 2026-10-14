"""Guard current navigation; dated historical evidence remains untouched."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CurrentDocumentationTests(unittest.TestCase):
    def test_archived_contract_context_links_both_directions(self):
        history = ROOT / 'docs/archive/DEVELOPMENT_PLAN_HISTORY_20261014.md'
        self.assertTrue(history.is_file())
        self.assertIn('../development/CLASSIC_CLEAN_STATE_CONTRACT.md',
                      history.read_text(encoding='utf-8'))
        contract = (ROOT / 'docs/development/CLASSIC_CLEAN_STATE_CONTRACT.md').read_text(encoding='utf-8')
        self.assertIn('../archive/DEVELOPMENT_PLAN_HISTORY_20261014.md', contract)

    def test_plan_introduction_does_not_reopen_published_release(self):
        text = (ROOT / 'docs/development/DEVELOPMENT_PLAN.md').read_text(encoding='utf-8')
        intro = text.split('## Aktuális kiadási állapot', 1)[0]
        self.assertIn('Publikálás TELJESÜLT', intro)
        self.assertNotIn('A következő kiadás célja v1.2.0', intro)
        self.assertNotIn('publikálás továbbra is külön kapu', intro)

    def test_bilingual_downloads_target_current_release(self):
        for name in ('README.md', 'README_HU.md', 'docs/guides/GUIDE_EN.md',
                     'docs/guides/GUIDE_HU.md'):
            with self.subTest(name=name):
                text = (ROOT / name).read_text(encoding='utf-8')
                self.assertIn('/releases/tag/v1.2.0', text)
                self.assertIn('Source/', text)
                self.assertNotIn('/releases/tag/v1.0.1-source', text)

    def test_notice_and_dependency_guide_identify_published_source(self):
        for name in ('NOTICE.md', 'docs/guides/SOURCE_DEPENDENCIES.md'):
            text = (ROOT / name).read_text(encoding='utf-8')
            self.assertIn('7e2271352d0f05fdad39bae26fdec764064fa1e1', text)
            self.assertNotIn('stabil 1.2.0 kiadás még nincs jóváhagyva', text)
            self.assertNotIn('Stable `1.2.0` publication is not yet approved', text)
        self.assertIn('FINAL_120_OWNER_ACCEPTANCE_20261014.md',
                      (ROOT / 'docs/README.md').read_text(encoding='utf-8'))
