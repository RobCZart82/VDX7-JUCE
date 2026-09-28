"""Write a stable, basename-only SHA-256 list for release-preparation assets."""
import argparse
import hashlib
from pathlib import Path


def write_manifest(files, output):
    paths = [Path(path) for path in files]
    names = [path.name for path in paths]
    if not paths or len(names) != len(set(names)):
        raise ValueError("Provide files with unique basenames")
    lines = []
    for path in sorted(paths, key=lambda item: item.name):
        if not path.is_file() or path.is_symlink():
            raise ValueError(f"Not a regular file: {path}")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        lines.append(f"{digest}  {path.name}")
    destination = Path(output)
    with destination.open("w", encoding="utf-8", newline="\n") as manifest:
        manifest.write("\n".join(lines) + "\n")
    return destination


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()
    result = write_manifest(args.files, args.output)
    print(f"Wrote SHA-256 manifest: {result}")
