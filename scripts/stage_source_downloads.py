"""Stage exact matching source and its checksum for a separate source release."""
import argparse
import json
from pathlib import Path
import re
import shutil
import zipfile

import package_source
from stage_release_downloads import digest, read_checksums, regular
from write_sha256_manifest import write_manifest


def stage(assets, output, label, source_commit, packager_commit, approval_commit, accepted=False):
    package_source.package_identity(label, accepted)
    if label != "1.0.1":
        raise ValueError("Separate source staging requires stable 1.0.1")
    for sha in (source_commit, packager_commit, approval_commit):
        if not isinstance(sha, str) or not re.fullmatch(r"[0-9a-f]{40}", sha):
            raise ValueError("Source/tooling/approval identities must be full commit SHAs")
    assets, output = Path(assets), Path(output)
    if not assets.is_dir() or assets.is_symlink():
        raise ValueError("Expected a regular validation directory")
    if output.exists() or output.is_symlink():
        raise ValueError("Use a new source staging directory")
    if assets.resolve() in output.resolve().parents:
        raise ValueError("Source staging must be outside retained validation assets")
    name = f"VDX7-{label}-{source_commit}-corresponding-source.zip"
    archive = assets / name
    regular(archive)
    entries = read_checksums(assets / "SHA256SUMS.txt")
    if name not in entries or digest(archive) != entries[name]:
        raise ValueError("Corresponding-source checksum mismatch")
    package_source.verify(archive)
    with zipfile.ZipFile(archive) as contents:
        manifest = json.loads(contents.read(package_source.MANIFEST),
                              object_pairs_hook=package_source.strict_json_object)
    expected = {"package_label": label, "source_commit": source_commit,
                "packager_commit": packager_commit, "release_accepted": accepted}
    if any(manifest.get(key) != value for key, value in expected.items()):
        raise ValueError("Source archive differs from the guarded product/tooling tuple")
    if accepted and manifest.get("release_approval", {}).get("approval_commit") != approval_commit:
        raise ValueError("Source archive lacks the exact committed release approval proof")
    output.mkdir(parents=True)
    staged = output / name
    shutil.copyfile(archive, staged)
    if digest(staged) != entries[name]:
        raise ValueError("Copied source checksum mismatch")
    write_manifest([staged], output / "SHA256SUMS.txt")
    if {path.name for path in output.iterdir()} != {name, "SHA256SUMS.txt"}:
        raise ValueError("Unexpected separate source download inventory")
    return {name: entries[name]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ("assets", "output"):
        parser.add_argument("--" + arg, type=Path, required=True)
    for arg in ("package-label", "source-commit", "packager-commit", "approval-commit"):
        parser.add_argument("--" + arg, required=True)
    parser.add_argument("--release-accepted", action="store_true")
    args = parser.parse_args()
    for name, sha in stage(args.assets, args.output, args.package_label, args.source_commit,
                           args.packager_commit, args.approval_commit, args.release_accepted).items():
        print(f"{sha}  {name}")
