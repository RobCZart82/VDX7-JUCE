# 1.0.0-dev01 validation — 2026-09-13

This is the first stabilization milestone toward 1.0.0, NOT final acceptance.
The published main and release assets are unchanged.

## Automated checks on macOS arm64

- PASS: VST3 builds; bundle version metadata is 1.0.0, visible UI version is 1.0.0-dev.
- PASS: direct ad-hoc signature and strict verification. Not notarized.
- PASS: CTest, 7/7 tests (none skipped): deferred MIDI, voice/SysEx data,
  algorithms, stability, all 128 notes/pitch/release, timing, processor integration.
- PASS: existing 148 parameters and legacy parameter indices remain intact.
- PASS: state/RAM/145-field round trips, GUI layouts, rename/copy/paste,
  file SysEx import/export and finite non-silent audio at 44.1/48/96 kHz,
  64/128/256 samples (existing processor suite).
- PASS: missing-ROM re-save/restart/manual-restore, invalid-ROM preservation,
  CC64/65 boundary values, CC11 silence/gain, deferred note-off, overflow
  reconciliation, live bank metadata/parameter synchronization and CC32 bank RAM.
- PASS: both local dependency checkouts are clean and match CMake's pinned SHAs.

## Timing experiment

Same user-supplied combined ROM/factory program, 48 kHz, note 60 velocity 100.
After 250 ms or more of startup settling, locate the first sample whose magnitude
exceeds 0.0001. Results include firmware response and patch attack; they are not
end-to-end device latency or a guarantee for every patch.

| Event sample | Previous 512-sample lookahead, ms | Demand-driven, ms |
|---:|---:|---:|
| 12000 | 3.083 | 6.563 |
| 12016 | 13.188 | 2.750 |
| 12064 | 12.188 | 4.354 |
| 12128 | 10.854 | 3.021 |
| 12256 | 8.188 | 2.896 |
| 12384 | 5.521 | 2.979 |
| 12511 | 2.875 | 2.875 |

Worst observed onset in this sample set decreases from 13.188 ms to 6.563 ms.
Not every event improves; the firmware's scheduling remains visible. For each
event timeline, audio is bit-identical across host partitions 64/128/256/512.
The test is reproducible with vdx7_timing_tests and a local ROM path.

## Still required before 1.0.0

- New-build REAPER acceptance on M1 and Windows; physical Intel Mac if shipped.
- PERFORMANCE/SETTINGS and any other agreed final feature scope.
- Audio-thread ownership/host callback audit. Transactions may still silence blocks.
- Dense MIDI/automation and multi-instance stress, transport restart, audio-device changes.
- Resampling quality measurements and implementation/acceptance.
- Updated final documentation, exact-source release packaging and acceptance.

The old release-publishing workflow has been removed from this development branch.
Its replacement stages development artifacts from an exact commit with read-only
permissions, and cannot publish or overwrite release assets. It has not been run
on GitHub yet. Final publication is deliberately separate.

The system Xcode wrapper still reports an unaccepted license. Compilation used
the installed compiler/SDK directly and signing used codesign directly; no license
was accepted on the user's behalf and no global toolchain settings were changed.

No installed plugin was removed or replaced. The existing system VST3 remains at
/Library/Audio/Plug-Ins/VST3/VDX7.vst3. No firmware is included in development packages.
