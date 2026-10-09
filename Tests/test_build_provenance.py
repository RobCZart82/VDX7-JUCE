"""Toolchain evidence tests use synthetic metadata, never a real installer or ROM."""
import importlib.util
from pathlib import Path
import tempfile
import shutil
import subprocess
import unittest
import sys
from unittest.mock import patch

SCRIPT = Path(__file__).parents[1] / "scripts/write_build_provenance.py"
HOOK = Path(__file__).parents[1] / "cmake/VDX7DependencySources.cmake"
spec = importlib.util.spec_from_file_location("build_provenance", SCRIPT)
provenance = importlib.util.module_from_spec(spec)
spec.loader.exec_module(provenance)


class BuildProvenanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.build = self.root / "build"
        self.meta = self.build / "CMakeFiles/3.31.6"
        self.meta.mkdir(parents=True)
        self.compiler = self.meta / "CMakeCXXCompiler.cmake"
        self.cache = self.build / "CMakeCache.txt"
        self.inno = self.root / "ISCC.exe"
        self.inno.write_bytes(b"synthetic compiler fixture")
        self.source = self.root / "source"
        self.source.mkdir()
        self.pins = {"JUCE": "a" * 40, "retromulator": "b" * 40}
        (self.source / "CMakeLists.txt").write_text("\n".join(
            f"FetchContent_Declare( {name} GIT_TAG {sha} )" for name, sha in self.pins.items()))
        self.environment = {"RUNNER_OS": "Windows", "RUNNER_ARCH": "X64", "ImageOS": "win22",
                            "ImageVersion": "20261001.1", "GITHUB_RUN_ID": "123", "GITHUB_RUN_ATTEMPT": "1"}
        self.windows()
        self.configured_sources()

    def windows(self):
        self.cache.write_text("CMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022\n"
                              "CMAKE_GENERATOR_PLATFORM:INTERNAL=x64\nCMAKE_COMMAND:INTERNAL=cmake\n")
        self.compiler.write_text('set(CMAKE_CXX_COMPILER_ID "MSVC")\n'
                                 'set(CMAKE_CXX_COMPILER_VERSION "19.44.35229.0")\n')
        (self.build / "VDX7.vcxproj").write_text('<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
                                               '<WindowsTargetPlatformVersion>10.0.26100.0</WindowsTargetPlatformVersion></Project>')

    def command(self, *args):
        if args[0] == "git":
            if args[-1] == "HEAD":
                return self.pins[Path(args[3]).name]
            return ""
        return {"cmake": "cmake version 3.31.6\n", "xcodebuild": "Xcode 16.2\nBuild version 16C5032a",
                "xcrun": "15.2"}[args[0]]

    def collect(self, platform="Windows-x64", **changes):
        arguments = dict(build=self.build, source=self.source, juce=self.root / "JUCE",
                         core=self.root / "retromulator", platform=platform, environment=self.environment,
                         inno=self.inno, inno_version="6.7.1")
        arguments.update(changes)
        with patch.object(provenance, "command", side_effect=self.command):
            return provenance.collect(**arguments)

    def configured_sources(self, source=None, juce=None, core=None):
        # Literal metadata written by CMake from its resolved source directories.
        source = source or self.source
        juce = juce or self.root / "JUCE"
        core = core or self.root / "retromulator/source/dx7Lib"
        (self.build / "VDX7DependencySources.cmake").write_text(
            f'set(VDX7_SOURCE_DIR "{source.as_posix()}")\n'
            f'set(VDX7_JUCE_SOURCE_DIR "{juce.as_posix()}")\n'
            f'set(VDX7_CORE_SOURCE_DIR "{core.as_posix()}")\n')
        with self.cache.open("a") as stream:
            stream.write(f"CMAKE_HOME_DIRECTORY:INTERNAL={self.source.as_posix()}\n")

    def test_other_compiled_juce_checkout_cannot_be_reported_as_supplied_pin(self):
        self.configured_sources(juce=self.root / "old-JUCE")
        with self.assertRaisesRegex(ValueError, "Configured JUCE source mismatch"):
            self.collect()

    def test_other_compiled_core_cannot_be_reported_as_supplied_pin(self):
        self.configured_sources(core=self.root / "other-core/source/dx7Lib")
        with self.assertRaisesRegex(ValueError, "Configured Retromulator source mismatch"):
            self.collect()

    def test_other_configured_wrapper_cannot_supply_the_dependency_pins(self):
        self.configured_sources(source=self.root / "other-wrapper")
        with self.assertRaisesRegex(ValueError, "Configured wrapper source mismatch"):
            self.collect()

    def test_missing_malformed_relative_or_wrong_cache_evidence_fails_closed(self):
        record = self.build / "VDX7DependencySources.cmake"
        original = record.read_text()
        for text in (None, "", original.replace("VDX7_JUCE_SOURCE_DIR", "WRONG_KEY"),
                     original.replace((self.root / "JUCE").as_posix(), "relative/JUCE")):
            with self.subTest(record=text):
                if text is None:
                    record.unlink()
                else:
                    record.write_text(text)
                with patch.object(provenance, "command") as run:
                    with self.assertRaises(ValueError):
                        provenance.collect(self.build, self.source, self.root / "JUCE",
                                           self.root / "retromulator", "Windows-x64", self.environment)
                    run.assert_not_called()
                record.write_text(original)
        self.cache.write_text(self.cache.read_text().replace(
            self.source.as_posix(), (self.root / "other-product").as_posix()))
        with self.assertRaisesRegex(ValueError, "Configured wrapper source mismatch"):
            self.collect()

    def test_aliases_are_normalized_and_generated_cmake_is_not_executed(self):
        record = self.build / "VDX7DependencySources.cmake"
        record.write_text(record.read_text().replace(
            (self.root / "JUCE").as_posix(), (self.root / "unused/../JUCE").as_posix())
            + f'file(WRITE "{(self.root / "must-not-exist").as_posix()}" "executed")\n')
        self.assertEqual(self.collect()["JUCE commit"], "a" * 40)
        self.assertFalse((self.root / "must-not-exist").exists())

    def test_real_mismatch_capture_preserves_existing_output(self):
        self.configured_sources(juce=self.root / "old-JUCE")
        output = self.root / "old-evidence.txt"
        output.write_text("previous evidence")
        argv = [str(SCRIPT), "--build", str(self.build), "--source", str(self.source),
                "--juce", str(self.root / "JUCE"), "--core", str(self.root / "retromulator"),
                "--platform", "Windows-x64", "--output", str(output)]
        with patch.object(sys, "argv", argv), patch.object(provenance, "command") as run:
            with self.assertRaisesRegex(ValueError, "Configured JUCE source mismatch"):
                provenance.main()
            run.assert_not_called()
        self.assertEqual(output.read_text(), "previous evidence")

    @unittest.skipUnless(shutil.which("cmake"), "CMake required for actual configure hook test")
    def test_actual_cmake_hook_supports_old_product_and_duplicate_include(self):
        for current in (False, True):
            for vendored in (False, True):
                with self.subTest(current_product=current, vendored=vendored):
                    src = self.root / f"product-{current}-{vendored}"
                    src.mkdir()
                    configured_juce = src / ("third_party/JUCE" if vendored else "build/_deps/juce-src")
                    configured_core = src / ("third_party/dx7Lib" if vendored else "build/_deps/retromulator-src/source/dx7Lib")
                    include = f'include("{HOOK.as_posix()}")\n' if current else ""
                    (src / "CMakeLists.txt").write_text(
                        'cmake_minimum_required(VERSION 3.22)\nproject(VDX7_JUCE LANGUAGES NONE)\n'
                        + include + f'set(juce_SOURCE_DIR "{configured_juce.as_posix()}")\n'
                        + f'set(VDX7_CORE_DIR "{configured_core.as_posix()}")\n')
                    build = src / "build"
                    subprocess.run([shutil.which("cmake"), "-S", str(src), "-B", str(build),
                                    "-DCMAKE_PROJECT_VDX7_JUCE_INCLUDE=" + str(HOOK)],
                                   check=True, capture_output=True, text=True)
                    observed = provenance.cmake_values((build / "VDX7DependencySources.cmake").read_text())
                    self.assertEqual(Path(observed["VDX7_SOURCE_DIR"]).resolve(), src.resolve())
                    self.assertEqual(Path(observed["VDX7_JUCE_SOURCE_DIR"]).resolve(), configured_juce.resolve())
                    self.assertEqual(Path(observed["VDX7_CORE_SOURCE_DIR"]).resolve(), configured_core.resolve())

    def test_windows_observed_versions_and_hash(self):
        result = self.collect()
        self.assertEqual(result["Windows SDK"], "10.0.26100.0")
        self.assertEqual(result["C++ compiler version"], "19.44.35229.0")
        self.assertEqual(result["JUCE commit"], "a" * 40)
        self.assertEqual(result["ISCC SHA-256"], provenance.hashlib.sha256(self.inno.read_bytes()).hexdigest())

    def test_mac_uses_configured_sdk_and_universal_architectures(self):
        self.environment["RUNNER_OS"] = "macOS"
        self.cache.write_text("CMAKE_GENERATOR:INTERNAL=Xcode\nCMAKE_COMMAND:INTERNAL=cmake\n"
                              "CMAKE_OSX_ARCHITECTURES:STRING=arm64;x86_64\n"
                              "CMAKE_OSX_SYSROOT:PATH=/SDK/MacOSX15.2.sdk\nCMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.0\n")
        self.configured_sources()
        self.compiler.write_text('set(CMAKE_CXX_COMPILER_ID "AppleClang")\nset(CMAKE_CXX_COMPILER_VERSION "16.0.0")\n')
        with patch.object(provenance, "command", side_effect=self.command) as run:
            result = provenance.collect(self.build, self.source, self.root / "JUCE", self.root / "retromulator",
                                        "macOS-universal", self.environment)
        self.assertEqual(result["Xcode"], "Xcode 16.2; Build version 16C5032a")
        run.assert_any_call("xcrun", "--sdk", "/SDK/MacOSX15.2.sdk", "--show-sdk-version")

    def test_missing_runner_or_compiler_evidence_rejected(self):
        for name in self.environment:
            with self.subTest(name=name):
                env = dict(self.environment)
                del env[name]
                with self.assertRaises(ValueError):
                    self.collect(environment=env)
        self.compiler.write_text('set(CMAKE_CXX_COMPILER_ID "MSVC")\n')
        with self.assertRaises(ValueError):
            self.collect()

    def test_ambiguous_compiler_or_sdk_rejected(self):
        other = self.build / "CMakeFiles/old/CMakeCXXCompiler.cmake"
        other.parent.mkdir()
        other.write_text(self.compiler.read_text())
        with self.assertRaises(ValueError):
            self.collect()
        other.unlink()
        (self.build / "VDX7.vcxproj").write_text('<Project xmlns="urn:test">'
                                               '<WindowsTargetPlatformVersion>10.0.1.0</WindowsTargetPlatformVersion>'
                                               '<WindowsTargetPlatformVersion>10.0.2.0</WindowsTargetPlatformVersion></Project>')
        with self.assertRaises(ValueError):
            self.collect()

    def test_wrong_inno_or_architecture_rejected(self):
        with self.assertRaises(ValueError):
            self.collect(inno_version="6.7.2")
        with self.assertRaises(ValueError):
            self.collect(inno=None)
        self.cache.write_text(self.cache.read_text().replace("=x64", "=ARM64"))
        with self.assertRaises(ValueError):
            self.collect()

    def test_dependency_mismatch_modified_or_missing_pin_rejected(self):
        self.pins["JUCE"] = "c" * 40
        with self.assertRaises(ValueError):
            self.collect()
        self.pins["JUCE"] = "a" * 40
        with patch.object(provenance, "command", return_value="modified"):
            with self.assertRaises(ValueError):
                provenance.dependency(self.source, self.root / "JUCE", "JUCE")
        (self.source / "CMakeLists.txt").write_text("# no dependency declarations")
        with self.assertRaises(ValueError):
            self.collect()

    def test_failed_capture_does_not_replace_existing_output(self):
        output = self.root / "old-evidence.txt"
        output.write_text("previous evidence")
        argv = [str(SCRIPT), "--build", str(self.build), "--source", str(self.source),
                "--juce", str(self.root / "JUCE"), "--core", str(self.root / "retromulator"),
                "--platform", "Windows-x64", "--output", str(output)]
        with patch.object(sys, "argv", argv), patch.object(provenance, "collect", side_effect=ValueError("missing")):
            with self.assertRaises(ValueError):
                provenance.main()
        self.assertEqual(output.read_text(), "previous evidence")

    def test_mac_wrong_architecture_rejected(self):
        self.environment["RUNNER_OS"] = "macOS"
        self.cache.write_text("CMAKE_GENERATOR:INTERNAL=Xcode\nCMAKE_COMMAND:INTERNAL=cmake\n"
                              "CMAKE_OSX_ARCHITECTURES:STRING=arm64\nCMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.0\n")
        self.configured_sources()
        self.compiler.write_text('set(CMAKE_CXX_COMPILER_ID "AppleClang")\nset(CMAKE_CXX_COMPILER_VERSION "16.0.0")\n')
        with self.assertRaises(ValueError):
            self.collect(platform="macOS-universal")


if __name__ == "__main__":
    unittest.main()
