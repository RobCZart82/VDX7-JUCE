"""Build deterministic corresponding-source ZIPs from exact Git objects (no upload)."""
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
VERIFIER = ".vdx7-source-tools/package_source.py"
PACKAGE_README = "SOURCE_PACKAGE_README.txt"
APPROVAL_PATH = "docs/release/RELEASE_APPROVAL.json"
APPROVED_GIT_REF = "refs/heads/main"
SUPPORTED_LABEL = r"1\.0\.[01](?:-dev|-rc[1-9][0-9]*)?"
APPROVED_WORKFLOW_REF = (
    "RobCZart82/VDX7-JUCE/.github/workflows/prepare-stable-package.yml@" + APPROVED_GIT_REF)


def git(repo, *args):
    # A local refs/replace entry must not substitute a different object's
    # policy/source while rev-parse still reports the requested original SHA.
    return subprocess.check_output(["git", "--no-replace-objects", "-C", str(repo), *args])


def full_commit(repo, sha, description):
    if not isinstance(sha, str) or not re.fullmatch(r"[0-9a-f]{40}", sha):
        raise ValueError(f"{description} must be a full lowercase 40-character commit SHA")
    try:
        actual = git(repo, "rev-parse", f"{sha}^{{commit}}").decode().strip()
    except subprocess.CalledProcessError as error:
        raise ValueError(f"{description} is not an available commit") from error
    if actual != sha:
        raise ValueError(f"{description} identity mismatch")


def package_identity(label, accepted):
    if type(accepted) is not bool:
        raise ValueError("Release acceptance must be a boolean")
    if not isinstance(label, str) or not re.fullmatch(SUPPORTED_LABEL, label):
        raise ValueError("Package label must be 1.0.0 or 1.0.1, optionally followed by -dev or -rcN")
    if accepted and "-" in label:
        raise ValueError("Only a stable source package may be accepted for publication")


def validate_source_version(cmake, installer, label):
    # Keep historical 1.0.0 verification/fixtures compatible. New 1.0.1
    # creation must not relabel an old product or use an old installer version.
    if label.split("-", 1)[0] != "1.0.1":
        return
    project = re.search(r"project\(VDX7_JUCE\s+VERSION\s+(\d+\.\d+\.\d+)\s", cmake)
    app = re.search(r'^#define AppVersion "([^"]+)"$', installer, re.MULTILINE)
    if project is None or app is None or project[1] != "1.0.1" or app[1] != "1.0.1":
        raise ValueError("Source project and installer versions must both match 1.0.1")


def strict_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate approval JSON key: {key}")
        result[key] = value
    return result


def release_authorization(repo, approval_commit, source_commit, package_label,
                          workflow_ref, workflow_git_ref, accepted=False, packager_commit=None):
    """Resolve exact packaging provenance, not a publisher signature.

    The canonical main workflow supplies the real GitHub context and checks out
    its own approval commit. CLI-supplied context alone cannot authenticate a
    publisher or replace human review of that protected-main approval record.
    """
    if type(accepted) is not bool:
        raise ValueError("Release acceptance must be a boolean")
    package_identity(package_label, accepted)
    full_commit(repo, approval_commit, "Approval commit")
    full_commit(repo, source_commit, "Source commit")
    if package_label.startswith("1.0.1"):
        try:
            validate_source_version(
                git(repo, "show", source_commit + ":CMakeLists.txt").decode(),
                git(repo, "show", source_commit + ":installer/windows/VDX7.iss").decode(),
                package_label)
        except subprocess.CalledProcessError as error:
            raise ValueError("Source version metadata is missing") from error
    if git(repo, "rev-parse", "HEAD").decode().strip() != approval_commit:
        raise ValueError("Approval checkout must match the exact approval commit")
    # Preparation can run on branches/tags or forks, but cannot export unsafe
    # output strings. Accepted mode is restricted to the canonical main ref.
    if (not isinstance(workflow_git_ref, str)
            or not re.fullmatch(r"refs/(?:heads|tags)/[A-Za-z0-9._/-]+", workflow_git_ref)
            or not isinstance(workflow_ref, str)
            or not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+/\.github/workflows/"
                                r"prepare-stable-package\.yml@" + re.escape(workflow_git_ref), workflow_ref)):
        raise ValueError("Invalid workflow ref context")
    result = {
        "source_commit": source_commit, "approval_commit": approval_commit,
        "package_label": package_label, "workflow_ref": workflow_ref,
        "workflow_git_ref": workflow_git_ref, "release_accepted": accepted,
    }
    if not accepted:
        packager_commit = packager_commit or approval_commit
        full_commit(repo, packager_commit, "Packager commit")
        result["packager_commit"] = packager_commit
        return result
    if workflow_ref != APPROVED_WORKFLOW_REF or workflow_git_ref != APPROVED_GIT_REF:
        raise ValueError("Accepted packaging requires the canonical main workflow ref")
    policy_files = snapshot(repo, approval_commit, (APPROVAL_PATH,))
    if APPROVAL_PATH not in policy_files:
        raise ValueError("Committed release approval policy is missing")
    policy_bytes = policy_files[APPROVAL_PATH][0]
    policy = json.loads(policy_bytes, object_pairs_hook=strict_json_object)
    if (not isinstance(policy, dict) or set(policy) != {"schema", "approved_release"}
            or type(policy["schema"]) is not int or policy["schema"] != 1):
        raise ValueError("Invalid release approval policy schema")
    approved = policy["approved_release"]
    if not isinstance(approved, dict) or set(approved) != {
            "package_label", "source_commit", "packager_commit", "workflow_ref"}:
        raise ValueError("No valid approved release tuple is committed")
    if (approved["package_label"] != package_label or approved["source_commit"] != source_commit
            or approved["workflow_ref"] != workflow_ref):
        raise ValueError("Requested release does not match the approved release tuple")
    approved_packager = approved["packager_commit"]
    full_commit(repo, approved_packager, "Approved packager commit")
    if packager_commit is not None and packager_commit != approved_packager:
        raise ValueError("Packager commit does not match the approved release tuple")
    if approved_packager == approval_commit:
        raise ValueError("Freeze the packager before the separate approval commit")
    for commit in (source_commit, approved_packager):
        try:
            git(repo, "merge-base", "--is-ancestor", commit, approval_commit)
        except subprocess.CalledProcessError as error:
            raise ValueError("Approved source and packager must be ancestors of the approval commit") from error
    result["packager_commit"] = approved_packager
    result["policy_sha256"] = hashlib.sha256(policy_bytes).hexdigest()
    return result


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
    data = subprocess.check_output(["git", "--no-replace-objects", "-C", str(repo), "cat-file", "--batch"],
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


def package(repo, juce, core, sha, output, package_label="1.0.1-dev",
            accepted_for_publication=False, packager_commit=None, approval_repo=None,
            approval_commit=None, workflow_ref=None, workflow_git_ref=None):
    package_identity(package_label, accepted_for_publication)
    if packager_commit is not None and (not isinstance(packager_commit, str)
                                       or not re.fullmatch(r"[0-9a-f]{40}", packager_commit)):
        raise ValueError("Packager commit must be a full lowercase 40-character SHA")
    if accepted_for_publication and packager_commit is None:
        raise ValueError("Accepted stable source packages must identify the release packager commit")
    authorization = None
    if accepted_for_publication:
        authorization = release_authorization(
            approval_repo or repo, approval_commit, sha, package_label,
            workflow_ref, workflow_git_ref, True, packager_commit)
    files = snapshot(repo, sha)
    cmake = files["CMakeLists.txt"][0].decode()
    validate_source_version(cmake, files.get("installer/windows/VDX7.iss", (b"", 0))[0].decode(),
                            package_label)
    if JUCE_SHA not in cmake or CORE_SHA not in cmake:
        raise ValueError("Source dependency pins changed; review/update the packager")
    for name, (data, _) in files.items():
        inspect_payload(name, data)
        if name.startswith(".vdx7-source-tools/") or name in (PACKAGE_README, MANIFEST):
            raise ValueError(f"Reserved generated-package path: {name}")
        if name.startswith("third_party/"):
            raise ValueError("Wrapper snapshot must not already contain vendored dependencies")
    # Keep the exact product snapshot, including its potentially older checker.
    # Package separately identified tooling from the actual packager commit.
    packager_commit = packager_commit or sha
    tooling = snapshot(repo, packager_commit, ("scripts/package_source.py",))
    checker = tooling["scripts/package_source.py"]
    if checker[0] != Path(__file__).read_bytes().replace(b"\r\n", b"\n"):
        raise ValueError("Running packager differs from packager_commit; commit tooling first")
    inspect_payload(VERIFIER, checker[0])
    files[VERIFIER] = checker
    files[PACKAGE_README] = ((
        "Corresponding-source package verification\n"
        f"Product commit: {sha}\nPackager commit: {packager_commit}\n"
        f"From the extracted folder run: python {VERIFIER} verify <original-source.zip>\n"
        "Use this bundled checker, not the older product scripts/package_source.py.\n"
        "The product snapshot is unchanged; .vdx7-source-tools and this readme are\n"
        "generated packaging additions recorded in SOURCE_MANIFEST.json.\n"
        "Hashes establish integrity, not publisher identity.\n"
    ).encode("utf-8"), 0o644)
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
    kind = ("development-corresponding-source" if package_label.endswith("-dev")
            else "release-candidate-corresponding-source" if "-rc" in package_label
            else "stable-release-corresponding-source" if accepted_for_publication
            else "stable-release-preparation-corresponding-source")
    manifest = {
        "schema": 1, "kind": kind, "package_label": package_label,
        "source_commit": sha, "release_accepted": accepted_for_publication,
        "dependencies": {"JUCE": JUCE_SHA, "Retromulator": CORE_SHA},
        "files": [{"path": name, "sha256": hashlib.sha256(data).hexdigest(), "size": len(data), "mode": mode}
                  for name, (data, mode) in sorted(files.items())],
    }
    if packager_commit is not None:
        manifest["packager_commit"] = packager_commit
    if authorization is not None:
        manifest["release_approval"] = {
            key: value for key, value in authorization.items() if key != "release_accepted"}
    # Refuse any existing destination, even an empty directory; never overwrite.
    output.mkdir(parents=True, exist_ok=False)
    filename = f"VDX7-{package_label}-{sha}-corresponding-source.zip"
    archive = output / filename
    write_zip(archive, files, manifest)
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    with (output / "SHA256SUMS.txt").open("w", encoding="utf-8", newline="\n") as sums_file:
        sums_file.write(f"{checksum}  {filename}\n")
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
        label = manifest.get("package_label", "")
        accepted = manifest.get("release_accepted")
        packager_commit = manifest.get("packager_commit")
        stable_kind = ("stable-release-corresponding-source" if accepted is True
                       else "stable-release-preparation-corresponding-source" if accepted is False
                       else None)
        valid_label = isinstance(label, str) and re.fullmatch(SUPPORTED_LABEL, label)
        kind = ("development-corresponding-source" if valid_label and label.endswith("-dev")
                else "release-candidate-corresponding-source" if valid_label and "-rc" in label
                else stable_kind if valid_label and "-" not in label
                else None)
        if (manifest.get("schema") != 1
                or (kind in ("development-corresponding-source", "release-candidate-corresponding-source")
                    and accepted is not False)
                or (accepted is True and not re.fullmatch(r"[0-9a-f]{40}", packager_commit or ""))
                or (packager_commit is not None and not re.fullmatch(r"[0-9a-f]{40}", packager_commit))
                or kind is None
                or manifest.get("kind") != kind
                or manifest.get("dependencies") != {"JUCE": JUCE_SHA, "Retromulator": CORE_SHA}
                or not re.fullmatch(r"[0-9a-f]{40}", manifest.get("source_commit", ""))):
            raise ValueError("Invalid corresponding-source package identity")
        approval = manifest.get("release_approval")
        if "release_approval" in manifest:
            required = {"source_commit", "packager_commit", "approval_commit", "package_label",
                        "workflow_ref", "workflow_git_ref", "policy_sha256"}
            if (accepted is not True or not isinstance(approval, dict) or set(approval) != required
                    or approval["source_commit"] != manifest["source_commit"]
                    or approval["packager_commit"] != packager_commit
                    or approval["package_label"] != label
                    or approval["workflow_ref"] != APPROVED_WORKFLOW_REF
                    or approval["workflow_git_ref"] != APPROVED_GIT_REF
                    or not isinstance(approval["approval_commit"], str)
                    or not re.fullmatch(r"[0-9a-f]{40}", approval["approval_commit"])
                    or approval["approval_commit"] == packager_commit
                    or not isinstance(approval["policy_sha256"], str)
                    or not re.fullmatch(r"[0-9a-f]{64}", approval["policy_sha256"])):
                raise ValueError("Invalid release approval provenance")
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
        if accepted is True:
            if approval is None:
                print("Legacy acceptance metadata: integrity only; release approval was not validated.")
            else:
                print("Approval provenance is internally consistent; integrity is not publisher authentication.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("create")
    for option in ("repo", "juce", "core", "output"):
        create.add_argument("--" + option, type=Path, required=True)
    create.add_argument("--commit", required=True)
    create.add_argument("--package-label", default="1.0.1-dev",
                        help="1.0.0 or 1.0.1, optionally followed by -dev or -rcN")
    create.add_argument("--release-accepted", action="store_true",
                        help="mark a reviewed stable source package accepted for publication")
    create.add_argument("--packager-commit",
                        help="full commit SHA of the source packager used to generate the manifest")
    create.add_argument("--approval-repo", type=Path,
                        help="checkout of the exact canonical workflow/approval commit")
    create.add_argument("--approval-commit")
    create.add_argument("--workflow-ref")
    create.add_argument("--workflow-git-ref")
    authorize = commands.add_parser("authorize", help="validate packaging context before building/output")
    authorize.add_argument("--repo", type=Path, required=True)
    authorize.add_argument("--approval-commit", required=True)
    authorize.add_argument("--commit", required=True)
    authorize.add_argument("--package-label", required=True)
    authorize.add_argument("--workflow-ref", required=True)
    authorize.add_argument("--workflow-git-ref", required=True)
    authorize.add_argument("--release-accepted", action="store_true")
    authorize.add_argument("--packager-commit")
    authorize.add_argument("--github-output", type=Path)
    check = commands.add_parser("verify")
    check.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "create":
        verify(package(args.repo, args.juce, args.core, args.commit, args.output,
                      args.package_label, args.release_accepted, args.packager_commit,
                      args.approval_repo, args.approval_commit, args.workflow_ref, args.workflow_git_ref))
    elif args.command == "authorize":
        authorization = release_authorization(
            args.repo, args.approval_commit, args.commit, args.package_label,
            args.workflow_ref, args.workflow_git_ref, args.release_accepted, args.packager_commit)
        if args.github_output is not None:
            with args.github_output.open("a", encoding="utf-8", newline="\n") as output:
                for key, value in authorization.items():
                    output.write(f"{key}={str(value).lower() if type(value) is bool else value}\n")
        print(json.dumps(authorization, sort_keys=True))
    else:
        verify(args.archive)
