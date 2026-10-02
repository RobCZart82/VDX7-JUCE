"""Synthetic minimum source inventory; no firmware or upstream source bytes."""
import hashlib

REQUIRED = (
    "CMakeLists.txt", "LICENSE.txt", "NOTICE.md", "THIRD_PARTY.md",
    "third_party/JUCE/LICENSE.md", "third_party/retromulator-notices/LICENSE.txt",
    "third_party/dx7Lib/dx7.cpp", ".vdx7-source-tools/package_source.py",
    "SOURCE_PACKAGE_README.txt",
)


def files():
    return {name: (b"// synthetic source/notice fixture\n", 0o644) for name in REQUIRED}


def entries(payload):
    return [{"path": name, "size": len(data), "mode": mode,
             "sha256": hashlib.sha256(data).hexdigest()}
            for name, (data, mode) in sorted(payload.items())]
