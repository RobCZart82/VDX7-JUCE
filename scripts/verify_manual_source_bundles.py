"""Verify matching source content and platform-specific VST3 markers in Manual ZIPs."""
import argparse
import hashlib
from pathlib import Path
import zipfile


def verify_manual_source_bundles(manual_archives, source_name):
    if (not source_name.endswith("-corresponding-source.zip")
            or Path(source_name).name != source_name
            or "/" in source_name or "\\" in source_name):
        raise ValueError("Expected a corresponding-source ZIP basename")
    if not manual_archives:
        raise ValueError("At least one manual-install ZIP is required")

    verified_hashes = []
    for manual_path in map(Path, manual_archives):
        with zipfile.ZipFile(manual_path) as archive:
            original_names = archive.namelist()
            names = [name.replace("\\", "/").rstrip("/") for name in original_names]
            if len(names) != len(set(names)):
                raise ValueError(f"Duplicate ZIP members in {manual_path}")
            if any(name.startswith("/") or ":" in name.split("/")[0]
                   or ".." in name.split("/") for name in names):
                raise ValueError(f"Unsafe ZIP member path in {manual_path}")
            has_macos_bundle_marker = "VDX7.vst3/Contents/Info.plist" in names
            has_windows_bundle_markers = (
                "VDX7.vst3/Contents/Resources/moduleinfo.json" in names
                and "VDX7.vst3/Contents/x86_64-win/VDX7.vst3" in names
            )
            if not (has_macos_bundle_marker or has_windows_bundle_markers):
                raise ValueError(f"VST3 bundle marker missing from {manual_path}")

            source_member = f"Source/{source_name}"
            sums_member = "Source/SHA256SUMS.txt"
            if source_member not in names or sums_member not in names:
                raise ValueError(f"Matching source ZIP or checksum missing from {manual_path}")

            source_bytes = archive.read(original_names[names.index(source_member)])
            sums_bytes = archive.read(original_names[names.index(sums_member)])
            lines = [line.split() for line in sums_bytes.decode("utf-8").splitlines() if line.strip()]
            if len(lines) != 1 or len(lines[0]) != 2 or lines[0][1] != source_name:
                raise ValueError(f"Invalid source checksum manifest in {manual_path}")
            digest = hashlib.sha256(source_bytes).hexdigest()
            if lines[0][0].lower() != digest:
                raise ValueError(f"Source checksum mismatch in {manual_path}")
            verified_hashes.append(digest)

    if len(set(verified_hashes)) != 1:
        raise ValueError("Manual ZIPs contain different corresponding-source archives")
    return verified_hashes[0]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-name", required=True)
    parser.add_argument("--manual-zip", action="append", required=True, type=Path)
    args = parser.parse_args()
    digest = verify_manual_source_bundles(args.manual_zip, args.source_name)
    for archive in args.manual_zip:
        print(f"PASS: {archive} includes and verifies {args.source_name}")
    print(f"Source SHA256: {digest}")
