# ROM-free component regressions — 2026-09-25

## Scope

The following repository test programs were compiled directly with the
available `g++` compiler and run from the source tree at commit
`3d6218797a1af2fd09e8484fe0b42ae2f3f956e4` (the head that became PR #63).
Direct compilation was used because CMake/CTest is unavailable in this
environment. The check is therefore evidence for these standalone test
programs only; it is not a normal project build or a substitute for the
configured CTest suite.

## Results

- `VDX7DeferredMidiTests.cpp`: **PASS**. Exhaustive MIDI channel-message
  validation, edit/deferred/keyboard queue ordering and recovery, SysEx
  admission, and deferred event bounds.
- `VDX7LatestDisplayTests.cpp`: **PASS**. Publication ordering, same-value and
  ABA UI edits, coherent reads, and concurrent field merges.
- `VDX7MonoCorrectionTests.cpp`: **PASS**. The correction-policy component's
  occupancy, release/lookup identity, slot/PC bounds and native guards. This is
  not a firmware-backed test and does not prove the plugin's firmware path.
- `VDX7VoiceDataTests.cpp` with `VDX7VoiceData.cpp` and `VDX7Sysex.cpp`:
  **PASS**. Voice-data and SysEx component tests.

All four executables exited successfully. No files in the project source were
modified by the compilation; temporary binaries were written under `/tmp`.

## Not run / limitations

- Full CMake configure/build and CTest: **NOT RUN** (CMake/CTest unavailable).
- JUCE-dependent editor/processor tests, including the header pixel regression:
  **NOT RUN** locally.
- Plugin build or host test: **NOT RUN**.
- ROM-backed processor, MONO correction, and Note 12–120 boundary acceptance:
  **NOT RUN** (private v1.8 ROM unavailable in this environment).

The macOS/Windows GitHub Actions for PR #63 passed; those checks do not replace
the local ROM-backed and real-host acceptance gates listed in `docs/release/ROADMAP_1.0.md`.
