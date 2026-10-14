"""Execute the actual Windows configure shell body with a failing CMake."""
import os
from pathlib import Path
import shutil
import subprocess
import unittest


class ConfigureFailureTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('bash'), 'bash required (provided by Windows Git/CI)')
    def test_failure_stops_before_following_work(self):
        text = (Path(__file__).resolve().parents[1] /
                '.github/workflows/prepare-v1.2.0-candidate.yml').read_text(encoding='utf-8')
        block = text.split('- name: Configure Windows x64 candidate', 1)[1]
        block = block.split('- name: Build VST3', 1)[0].split('run: |', 1)[1]
        body = '\n'.join(line[10:] for line in block.splitlines() if line.strip())
        self.assertEqual(body.count('cmake -S'), 1)
        for release, candidate in (('ON', ''), ('OFF', 'rc4')):
            with self.subTest(release=release):
                env = dict(os.environ, GITHUB_WORKSPACE='/tmp/workspace with spaces',
                           CONFIGURE_RELEASE=release, CONFIGURE_CANDIDATE=candidate)
                result = subprocess.run(
                    ['bash', '-c', 'cmake() { echo invoked; return 37; };\n' + body +
                     '\necho SHOULD_NOT_RUN'], env=env, capture_output=True, text=True)
                self.assertEqual(result.returncode, 37)
                self.assertEqual(result.stdout.strip(), 'invoked')
