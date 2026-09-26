# First ROM load after a fresh-instance voice edit (F6)

Status: baseline failure reproduced; targeted fix implemented and covered by
the local-ROM stability executable. Public CI and review/merge are pending.

## Reproduced behavior

On baseline `29ab5e350019e32f64650b068afcc7709b409607`, create a fresh processor
without a ROM, change voice parameters through the host parameter API, and then
load a valid ROM for the first time. The explicit edits were lost: ROM loading
cleared the pending dirty masks before applying them, and parameter
synchronization then published the ROM's initial voice over the user's edits.

The regression sets feedback to 3 and operator 6 output level to 37 before the
first ROM load. It failed on the baseline at `first ROM load preserves explicit
voice edits made before firmware was available`.

## Behavior implemented

- When there is no pending saved project to restore, the successful first ROM
  load applies explicit pending operator/voice parameter edits to the loaded
  initial voice, then reloads that voice in the engine.
- When a saved project is pending, its packed RAM remains authoritative; explicit
  edits made while that saved project's ROM is unavailable continue to be
  captured as deferred edits and applied by the existing restore path.
- No-ROM state save/restore behavior, performance parameters, and failed ROM
  loading semantics are not changed by this fix.

## Local verification

Using the existing private ROM fixture (not copied into the repository):

- Baseline first-load regression: failed as expected.
- Focused regression after fix: PASS for both voice and upper operator dirty
  parameter paths.
- Full ROM-backed `vdx7_stability_tests`: PASS. During this run, two pre-existing
  contention assertions were corrected to drain event-free delayed time before
  checking queue inactivity; the unsupported MIDI event filters themselves
  were not changed.
- ROM-free CTest: 10/10 PASS.
- `git diff --check`: PASS.

The ROM-backed results are local to the current unmerged branch. Public Windows
and macOS CI passed on commit `36df778` on 2026-09-26: plugin builds, all 10
ROM-free tests, and ROM-test registration smoke passed, but CI did not execute
the firmware-dependent regression. The private ROM and any derived firmware
data remain outside the repository and build artifacts.
