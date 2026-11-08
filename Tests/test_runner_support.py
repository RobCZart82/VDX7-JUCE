"""Do not dispatch jobs to the retired macOS 14 image."""
from pathlib import Path
import unittest


class RunnerSupportTests(unittest.TestCase):
    def test_workflows_do_not_use_retired_image(self):
        root = Path(__file__).resolve().parents[1]
        for path in (root / '.github/workflows').glob('*.yml'):
            with self.subTest(workflow=path.name):
                text = path.read_text(encoding='utf-8')
                self.assertNotIn('runs-on: macos-14', text)
                self.assertNotIn('os: macos-14', text)
        build = (root / '.github/workflows/build-macos.yml').read_text(encoding='utf-8')
        self.assertIn('runs-on: macos-15', build)
        self.assertIn('CMAKE_OSX_DEPLOYMENT_TARGET=11.0', build)
        self.assertIn('arm64;x86_64', build)
