"""Verify the actual Inno 6 compiler engine, without creating an installer."""
import argparse
from pathlib import Path
import re
import subprocess
import sys

PROBE = """[Setup]
AppName=VDX7 compiler version probe
AppVersion=1
CreateAppDir=no
Uninstallable=no
Output=no
"""


def verify(inno, expected):
    # Inno 6.7.1 has no --version switch; /? does not load the engine.
    # A successful /O- stdin compile loads ISCmplr.dll and prints its version.
    result = subprocess.run([str(inno.resolve()), "/O-", "-"], input=PROBE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, encoding="utf-8", timeout=60, check=False)
    if result.returncode != 0:
        raise ValueError(f"Inno Setup probe failed ({result.returncode}): {result.stdout}")
    versions = re.findall(r"^Compiler engine version: Inno Setup ([0-9]+\.[0-9]+\.[0-9]+)\r?$",
                          result.stdout, re.MULTILINE)
    if len(versions) != 1 or versions[0] != expected:
        raise ValueError(f"Inno Setup version mismatch: expected {expected}, observed {versions!r}")
    return versions[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inno", type=Path, required=True)
    parser.add_argument("--expected", required=True)
    args = parser.parse_args()
    try:
        version = verify(args.inno, args.expected)
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(error, file=sys.stderr)
        return 1
    print(version)
    return 0


if __name__ == "__main__":
    sys.exit(main())
