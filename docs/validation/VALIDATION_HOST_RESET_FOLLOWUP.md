# Host reset: fresh-note release acceptance

Date: 2026-09-23. Base: `aa47ebe` (merged PR #44).
Scope: regression tests only; no engine, parameter or installed-plugin change.

## Added observations

- The existing long-release/nonzero-L4 cases now send Note Off for the fresh
  post-reset note and assert that held ownership clears. They intentionally do
  not require immediate silence for a patch with a long or nonzero-floor release.
- A separate fast-release fixture starts a sustained note, requests public
  `reset()`, and supplies a fresh note immediately on the next callback. It must
  sound, then release ownership and become silent after Note Off without another
  reset, sustain-off or panic message. Persistent settings must remain unchanged.
- The first audible block is reported in audio-timeline units, not wall-clock
  execution time. Its threshold includes firmware note dispatch, envelope onset
  and SRC delay; it is not an isolated measurement of reset cleanup duration.
- Processor fixtures in this executable are heap-owned to avoid adding pressure
  to the Windows default stack. Allocation probes still surround callbacks.

## Local results

macOS arm64 Release, Apple clang 21.0.0, private local combined ROM:
target `vdx7_host_reset_tests` builds; CTest `vdx7_host_reset` PASS in 5.60 s.
The original assertions and all six new rate/block cases pass.

All test targets subsequently built successfully. The full CTest run completed
10/11 successfully in 71.92 s; `vdx7_processor` stopped at its explicit desktop
access requirement for the Save As GUI test in the sandbox. Re-running that
unchanged test with desktop access passed in 7.82 s. Thus every one of the eleven
tests has a passing local result, but this was not a single clean 11/11 run.

| Sample rate | Block | First audible block | Block-start timeline, ms |
| --- | --- | --- | --- |
| 44100 | 64 | 36 | 52.245 |
| 44100 | 256 | 9 | 52.245 |
| 48000 | 64 | 38 | 50.667 |
| 48000 | 256 | 10 | 53.333 |
| 96000 | 64 | 75 | 50.000 |
| 96000 | 256 | 19 | 50.667 |

These are observations for this fixture, not a guaranteed upper bound for a
long-lived instance. No scheduler-dependent wall-time assertion was added.

## Remaining acceptance

- Long-lived instance with expanded repeated-note release budgets; measure
  cleanup and post-reset onset separately, including deferred-queue limits.
- Actual VST3 wrapper reset entry in a host. Transport Stop alone is not proof:
  the host may send panic MIDI or deactivate/reprepare the plugin instead.
- Reset overlapping state/ROM replacement and firmware recovery.
- Windows execution of the extended tests, sanitizer checks and real-host tests.

The installed `/Library/Audio/Plug-Ins/VST3/VDX7.vst3` was left untouched.
Its version metadata alone does not identify the source commit. The results
above belong to the locally built test executable, not the installed VST3.
