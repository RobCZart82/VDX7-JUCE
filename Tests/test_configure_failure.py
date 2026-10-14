"""Execute the actual Windows configure shell body with a failing CMake."""
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import unittest


class ConfigureFailureTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('bash'), 'bash required (provided by Windows Git/CI)')
    def test_failure_stops_before_following_work(self):
        bash = shutil.which('bash')
        if os.name == 'nt':
            # PATH may resolve Windows' WSL launcher instead of Git Bash.
            git = shutil.which('git')
            if git:
                git_bash = Path(git).resolve().parent.parent / 'bin' / 'bash.exe'
                if git_bash.is_file():
                    bash = str(git_bash)
        text = (Path(__file__).resolve().parents[1] /
                '.github/workflows/prepare-v1.2.0-candidate.yml').read_text(encoding='utf-8')
        block = text.split('- name: Configure Windows x64 candidate', 1)[1]
        block = block.split('- name: Build VST3', 1)[0].split('run: |', 1)[1]
        body = '\n'.join(line[10:] for line in block.splitlines() if line.strip())
        self.assertEqual(body.count('cmake -S'), 1)
        for release, candidate in (('ON', ''), ('OFF', 'rc4')):
            with self.subTest(release=release):
                # Set empty candidate in the shell itself: Windows/MSYS environment
                # transport may omit empty values, which would trip set -u.
                assignments = '\n'.join(
                    f'export {key}={shlex.quote(value)}' for key, value in
                    {'GITHUB_WORKSPACE': '/tmp/workspace with spaces',
                     'CONFIGURE_RELEASE': release, 'CONFIGURE_CANDIDATE': candidate}.items())
                result = subprocess.run(
                    [bash, '--noprofile', '--norc', '-c', assignments +
                     '\ncmake() { echo invoked; return 37; };\n' + body +
                     '\necho SHOULD_NOT_RUN'], capture_output=True, text=True)
                self.assertEqual(result.returncode, 37,
                                 f'bash={bash}\nstdout={result.stdout}\nstderr={result.stderr}')
                self.assertEqual(result.stdout.strip(), 'invoked')
