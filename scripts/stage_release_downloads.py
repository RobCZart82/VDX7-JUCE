"""Stage exactly four checked user downloads; never publish a Release."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil

SUFFIXES = ("Windows-x64-Setup.exe", "Windows-x64-Manual.zip",
            "macOS-universal.pkg", "macOS-universal-Manual.zip")


def download_names(label):
    if not isinstance(label, str) or not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", label):
        raise ValueError("A numeric stable package version is required")
    return tuple(f"VDX7-{label}-{suffix}" for suffix in SUFFIXES)


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def regular(path):
    if not path.is_file() or path.is_symlink():
        raise ValueError(f"Expected a regular file: {path.name}")


def read_checksums(manifest):
    regular(manifest)
    entries = {}
    for line in manifest.read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([^/\\\x00]+)", line)
        if not match or match[2] in (".", "..") or match[2] in entries:
            raise ValueError("Malformed or duplicate checksum entry")
        entries[match[2]] = match[1]
    return entries


def stage(assets, output, label):
    assets, output = Path(assets), Path(output)
    names = download_names(label)
    if not assets.is_dir() or assets.is_symlink():
        raise ValueError("Expected a regular asset directory")
    if output.exists() or output.is_symlink():
        raise ValueError("Use a new staging directory; existing files will not be overwritten")
    if assets.resolve() in output.resolve().parents:
        raise ValueError("Public staging must be outside the retained validation directory")
    entries = read_checksums(assets / "SHA256SUMS.txt")
    # Verify all selected assets before creating any public payload.
    for name in names:
        path = assets / name
        regular(path)
        if name not in entries or digest(path) != entries[name]:
            raise ValueError(f"Missing or mismatched checksum: {name}")
    output.mkdir(parents=True)
    for name in names:
        shutil.copyfile(assets / name, output / name)
        if digest(output / name) != entries[name]:
            raise ValueError(f"Copied asset checksum mismatch: {name}")
    if {path.name for path in output.iterdir()} != set(names):
        raise ValueError("Unexpected public download inventory")
    return {name: entries[name] for name in names}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--package-label", required=True)
    args = parser.parse_args()
    for name, sha256 in stage(args.assets, args.output, args.package_label).items():
        print(f"{sha256}  {name}")
