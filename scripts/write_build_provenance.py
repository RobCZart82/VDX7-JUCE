"""Record observed packaging toolchain metadata; not a reproducibility certificate."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

SHA = re.compile(r"[0-9a-f]{40}")


def command(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT).strip()


def cache_values(text):
    return dict(re.findall(r"^([A-Za-z0-9_]+):[^=\n]+=(.*)$", text, re.MULTILINE))


def cmake_values(text):
    # Only read generated literal metadata. Never execute a CMake file.
    return dict(re.findall(r'^set\(([A-Za-z0-9_]+) "([^"\n]*)"\)$', text, re.MULTILINE))


def required(values, name):
    value = values.get(name, "").strip()
    if not value or "\n" in value or "\r" in value:
        raise ValueError(f"Missing or invalid metadata: {name}")
    return value


def dependency(source, path, name):
    declaration = re.search(r"FetchContent_Declare\(\s*" + re.escape(name)
                            + r"\b(.*?)\)", (source / "CMakeLists.txt").read_text(), re.S | re.I)
    pin = re.search(r"\bGIT_TAG\s+([0-9a-f]{40})\b", declaration.group(1)) if declaration else None
    if not pin:
        raise ValueError(f"Missing dependency pin: {name}")
    actual = command("git", "--no-replace-objects", "-C", str(path), "rev-parse", "HEAD")
    if not SHA.fullmatch(actual) or actual != pin.group(1):
        raise ValueError(f"Dependency identity mismatch: {name}")
    if command("git", "--no-replace-objects", "-C", str(path), "status", "--porcelain", "--untracked-files=no"):
        raise ValueError(f"Modified dependency: {name}")
    return actual


def configured_sources(build, source, juce, core, cache):
    # These are observations emitted by the product configure (or the exact
    # workflow/approval hook for an older product), not caller-supplied identities.
    # Read literals only; never source/execute generated CMake or infer a
    # default _deps checkout when the evidence is missing.
    record = build / "VDX7DependencySources.cmake"
    if not record.is_file():
        raise ValueError("Missing configured dependency source record; reconfigure with the provenance hook")
    observed = cmake_values(record.read_text(encoding="utf-8"))
    checks = (
        (required(cache, "CMAKE_HOME_DIRECTORY"), source, "wrapper"),
        (required(observed, "VDX7_SOURCE_DIR"), source, "wrapper"),
        (required(observed, "VDX7_JUCE_SOURCE_DIR"), juce, "JUCE"),
        (required(observed, "VDX7_CORE_SOURCE_DIR"), core / "source/dx7Lib", "Retromulator"),
    )
    for actual, supplied, name in checks:
        path = Path(actual)
        if not path.is_absolute() or path.resolve() != supplied.resolve():
            raise ValueError(f"Configured {name} source mismatch")


def collect(build, source, juce, core, platform, environment, inno=None, inno_version=None):
    cache = cache_values((build / "CMakeCache.txt").read_text(encoding="utf-8"))
    configured_sources(build, source, juce, core, cache)
    compiler_files = list((build / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if len(compiler_files) != 1:
        raise ValueError("Expected exactly one configured C++ compiler record")
    compiler = cmake_values(compiler_files[0].read_text(encoding="utf-8"))
    generator = required(cache, "CMAKE_GENERATOR")
    records = {
        "CMake": command(required(cache, "CMAKE_COMMAND"), "--version").splitlines()[0],
        "Generator": generator,
        "C++ compiler": required(compiler, "CMAKE_CXX_COMPILER_ID"),
        "C++ compiler version": required(compiler, "CMAKE_CXX_COMPILER_VERSION"),
        "Runner OS": required(environment, "RUNNER_OS"),
        "Runner architecture": required(environment, "RUNNER_ARCH"),
        "Runner image": required(environment, "ImageOS"),
        "Runner image version": required(environment, "ImageVersion"),
        "Workflow run": required(environment, "GITHUB_RUN_ID"),
        "Workflow attempt": required(environment, "GITHUB_RUN_ATTEMPT"),
        "JUCE commit": dependency(source, juce, "JUCE"),
        "Retromulator commit": dependency(source, core, "retromulator"),
    }
    if platform == "Windows-x64":
        if generator != "Visual Studio 17 2022" or required(cache, "CMAKE_GENERATOR_PLATFORM") != "x64":
            raise ValueError("Expected Visual Studio 2022 x64 configuration")
        if records["C++ compiler"] != "MSVC" or records["Runner OS"] != "Windows":
            raise ValueError("Expected Windows/MSVC metadata")
        root = ET.parse(build / "VDX7.vcxproj").getroot()
        sdk = {node.text for node in root.iter() if node.tag.endswith("}WindowsTargetPlatformVersion")}
        if len(sdk) != 1 or not re.fullmatch(r"\d+\.\d+\.\d+\.\d+", next(iter(sdk)) or ""):
            raise ValueError("Missing or ambiguous configured Windows SDK")
        records["Windows SDK"] = next(iter(sdk))
        if not inno or inno_version != "6.7.1":
            raise ValueError("Expected workflow-verified Inno Setup 6.7.1")
        records["Inno Setup"] = inno_version
        records["ISCC SHA-256"] = hashlib.sha256(inno.read_bytes()).hexdigest()
    elif platform == "macOS-universal":
        if generator != "Xcode" or records["C++ compiler"] != "AppleClang" or records["Runner OS"] != "macOS":
            raise ValueError("Expected macOS/Xcode/AppleClang metadata")
        if set(required(cache, "CMAKE_OSX_ARCHITECTURES").split(";")) != {"arm64", "x86_64"}:
            raise ValueError("Expected both Universal architectures")
        sdk = cache.get("CMAKE_OSX_SYSROOT") or "macosx"
        records["Xcode"] = command("xcodebuild", "-version").replace("\n", "; ")
        records["macOS SDK"] = command("xcrun", "--sdk", sdk, "--show-sdk-version")
        records["Deployment target"] = required(cache, "CMAKE_OSX_DEPLOYMENT_TARGET")
    else:
        raise ValueError("Unsupported packaging platform")
    for name, value in records.items():
        if not value or "\n" in value or "\r" in value:
            raise ValueError(f"Invalid observed value: {name}")
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("build", "source", "juce", "core", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--platform", choices=("Windows-x64", "macOS-universal"), required=True)
    parser.add_argument("--inno", type=Path)
    parser.add_argument("--inno-version")
    args = parser.parse_args()
    records = collect(args.build, args.source, args.juce, args.core, args.platform,
                      os.environ, args.inno, args.inno_version)
    text = "\nObserved build toolchain (not a bit-reproducibility or signing claim):\n"
    text += "".join(f"{name}: {value}\n" for name, value in records.items())
    # Validate everything before writing; failure must not replace old evidence.
    args.output.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
