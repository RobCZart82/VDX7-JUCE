"""Build a deterministic development source ZIP from exact Git objects (no upload)."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import zipfile

JUCE_SHA = "e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8"
CORE_SHA = "d5473776a0449d60a997b91bdc888598a33265ac"
MANIFEST = "SOURCE_MANIFEST.json"


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args])


def safe_path(name):
    path = PurePosixPath(name)
    if (not name or path.is_absolute() or "\\" in name or ":" in name
            or any(p in ("", ".", "..") for p in name.split("/"))):
        raise ValueError(f"Unsafe archive path: {name}")
    return path


def inspect_payload(name, data, upstream=False):
    path = safe_path(name)
    if any(p.lower() in {".git", ".svn", "node_modules", "__pycache__", "build", "_deps"}
           for p in path.parts):
        # Upstream JUCE contains source directories named Build, not local caches.
        if not (upstream and "build" in {p.lower() for p in path.parts}
                and not any(p.lower() in {".git", ".svn", "node_modules", "__pycache__", "_deps"}
                            for p in path.parts)):
            raise ValueError(f"Cache/VCS path: {name}")
    if path.suffix.lower() in {".exe", ".dll", ".pdb", ".rom", ".syx", ".bin", ".pfx", ".p12", ".pem", ".key", ".o", ".a", ".lib"}:
        raise ValueError(f"Forbidden source-package payload: {name}")
    if path.suffix.lower() == ".obj" and not (upstream and path.name == "teapot.obj"):
        raise ValueError(f"Object file: {name}")
    if path.name.lower().startswith(".env") or path.name.lower() in {"id_rsa", "id_ed25519", "credentials"}:
        raise ValueError(f"Credential-like filename: {name}")
    if not upstream and path.parts[0].lower() == "rom" and name != "ROM/PUT_YOUR_ROM_HERE.txt":
        raise ValueError(f"Unexpected ROM-directory content: {name}")
    if not upstream and path.suffix.lower() in {".zip", ".7z", ".tar", ".gz"}:
        raise ValueError(f"Nested wrapper archive requires manual review: {name}")
    if not upstream and re.search(rb"-----BEGIN [A-Z ]*PRIVATE KEY-----|ghp_[A-Za-z0-9]{36}|github_pat_[A-Za-z0-9_]{40,}", data):
        raise ValueError(f"Secret-like content: {name}")
    if not upstream and re.search(rb"(?:[A-Za-z]:[\\/]Users[\\/]|/Users/|/home/)(?!YourName|USERNAME|<)[A-Za-z0-9_.-]+[\\/]", data):
        raise ValueError(f"Personal absolute path: {name}")


def snapshot(repo, sha, paths=()):
    if not re.fullmatch(r"[0-9a-f]{40}", sha):
        raise ValueError("Use a full lowercase 40-character commit SHA")
    if git(repo, "rev-parse", f"{sha}^{{commit}}").decode().strip() != sha:
        raise ValueError("Commit identity mismatch")
    records = []
    for raw in git(repo, "ls-tree", "-rz", sha, "--", *paths).split(b"\0"):
        if not raw:
            continue
        metadata, name = raw.split(b"\t", 1)
        mode, kind, oid = metadata.decode().split()
        name = name.decode("utf-8")
        safe_path(name)
        if kind != "blob" or mode not in ("100644", "100755"):
            raise ValueError(f"Unsupported Git member: {name}")
        records.append((name, int(mode, 8) & 0o777, oid))
    # Read blobs directly: git archive can apply machine-specific CRLF or
    # local export attributes, which would violate cross-platform identity.
    data = subprocess.check_output(["git", "-C", str(repo), "cat-file", "--batch"],
                                   input="".join(oid + "\n" for _, _, oid in records).encode())
    result = {}
    stream = io.BytesIO(data)
    for name, mode, oid in records:
        header = stream.readline().decode().split()
        if len(header) != 3 or header[:2] != [oid, "blob"] or name in result:
            raise ValueError(f"Invalid blob response: {name}")
        size = int(header[2])
        content = stream.read(size)
        if len(content) != size or stream.read(1) != b"\n":
            raise ValueError(f"Truncated blob: {name}")
        result[name] = (content, mode)
    return result


def write_zip(destination, files, manifest):
    payload = dict(files)
    if MANIFEST in payload:
        raise ValueError("Reserved manifest path already exists")
    payload[MANIFEST] = ((json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode(), 0o644)
    # Stored entries avoid zlib/version-dependent compressed bytes. Timestamps,
    # permissions and ordering are fixed; no host paths or current time are added.
    with zipfile.ZipFile(destination, "x", compression=zipfile.ZIP_STORED) as archive:
        for name, (data, mode) in sorted(payload.items()):
            item = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            item.create_system = 3
            item.external_attr = (0o100000 | mode) << 16
            archive.writestr(item, data)


def package(repo, juce, core, sha, output):
    files = snapshot(repo, sha)
    cmake = files["CMakeLists.txt"][0].decode()
    if JUCE_SHA not in cmake or CORE_SHA not in cmake:
        raise ValueError("Source dependency pins changed; review/update the packager")
    for name, (data, _) in files.items():
        inspect_payload(name, data)
        if name.startswith("third_party/"):
            raise ValueError("Wrapper snapshot must not already contain vendored dependencies")
    for name, item in snapshot(juce, JUCE_SHA).items():
        inspect_payload(name, item[0], upstream=True)
        files["third_party/JUCE/" + name] = item
    for name, item in snapshot(core, CORE_SHA, ("source/dx7Lib", "LICENSE.txt", "README.md")).items():
        inspect_payload(name, item[0], upstream=True)
        target = ("third_party/dx7Lib/" + name.removeprefix("source/dx7Lib/")
                  if name.startswith("source/dx7Lib/") else "third_party/retromulator-notices/" + name)
        files[target] = item
    for required in ("LICENSE.txt", "NOTICE.md", "THIRD_PARTY.md", "third_party/JUCE/LICENSE.md",
                     "third_party/retromulator-notices/LICENSE.txt", "third_party/dx7Lib/dx7.cpp"):
        if required not in files:
            raise ValueError(f"Required source/notice missing: {required}")
    manifest = {
        "schema": 1, "kind": "development-corresponding-source", "source_commit": sha,
        "release_accepted": False, "dependencies": {"JUCE": JUCE_SHA, "Retromulator": CORE_SHA},
        "files": [{"path": name, "sha256": hashlib.sha256(data).hexdigest(), "size": len(data), "mode": mode}
                  for name, (data, mode) in sorted(files.items())],
    }
    # Refuse any existing destination, even an empty directory; never overwrite.
    output.mkdir(parents=True, exist_ok=False)
    filename = f"VDX7-1.0.0-dev-{sha}-corresponding-source.zip"
    archive = output / filename
    write_zip(archive, files, manifest)
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / "SHA256SUMS.txt").write_text(f"{checksum}  {filename}\n", encoding="utf-8", newline="\n")
    print(f"PASS: {len(files)} source files; {filename}; SHA256={checksum}")
    return archive


def verify(archive_path):
    with zipfile.ZipFile(archive_path) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError("Duplicate ZIP paths")
        for name in names:
            safe_path(name)
        manifest = json.loads(archive.read(MANIFEST))
        if (manifest.get("schema") != 1 or manifest.get("release_accepted") is not False
                or manifest.get("dependencies") != {"JUCE": JUCE_SHA, "Retromulator": CORE_SHA}
                or not re.fullmatch(r"[0-9a-f]{40}", manifest.get("source_commit", ""))):
            raise ValueError("Invalid development-source manifest identity")
        entries = manifest["files"]
        expected = [entry["path"] for entry in entries]
        if len(expected) != len(set(expected)) or set(names) != set(expected) | {MANIFEST}:
            raise ValueError("ZIP inventory differs from manifest")
        for entry in entries:
            data = archive.read(entry["path"])
            inspect_payload(entry["path"], data, upstream=entry["path"].startswith("third_party/"))
            if len(data) != entry["size"] or hashlib.sha256(data).hexdigest() != entry["sha256"]:
                raise ValueError(f"Hash/size mismatch: {entry['path']}")
            info = archive.getinfo(entry["path"])
            if entry["mode"] not in (0o644, 0o755) or info.external_attr >> 16 != (0o100000 | entry["mode"]):
                raise ValueError("ZIP mode mismatch")
        print(f"PASS: verified {len(entries)} files against embedded manifest")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("create")
    for option in ("repo", "juce", "core", "output"):
        create.add_argument("--" + option, type=Path, required=True)
    create.add_argument("--commit", required=True)
    check = commands.add_parser("verify")
    check.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "create":
        verify(package(args.repo, args.juce, args.core, args.commit, args.output))
    else:
        verify(args.archive)
