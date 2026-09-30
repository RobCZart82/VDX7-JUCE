# Pending-project operation boundary — 2026-09-30

Baseline: `3fa8c2e00ad4dcd1860551cf3596ee4ad29de789`.
Findings: AUDIT-20260930-A1/A2. Release 1.0.0 product source has the same
affected processor code. This is corrective development, not publication.

## Reproduction and policy

The pre-fix processor accepted SysEx import while a project waited for a
different ROM; feedback edited from 1 to 6 reverted to 1 on the matching ROM
load. USER/SysEx capture returned the incompatible engine's patch. The dirty
message hid the recovery warning. These were reproduced in the audit harness.
The new integrated regression also failed on the unmodified baseline at
`pending recovery warning outranks dirty export advice`.

Unsupported persistent operations are now rejected at processor boundaries,
not merely disabled in the editor. Voice/operator host-parameter edits and
project saving remain supported. Program/bank/live SysEx and persistent CCs
cannot mutate the incompatible engine. Transient note/release/expression
delivery is retained. A ready project regains the normal operation paths.

`Tests/VDX7PendingBoundaryTests.h` runs inside the existing `vdx7_stability`
registration, preserving the local-ROM registration inventory. Its eight
combinations cover no/foreign ROM, no/active callbacks, and direct/re-save
restore. It checks single/bank/USER import, USER/SysEx capture, rename,
copy/paste, program/bank, play/bend/tune/controller rejection, live persistent
MIDI rejection, warning priority, feedback/operator preservation and ready
positive controls. Temporary private factory-name variation changes ROM
identity without changing firmware instructions. No ROM is checked in.

## Executed checks

- PASS: Windows Release `vdx7_stability_tests`, including the new matrix and
  existing stability/Settings/restore/contention/live-bank regressions.
- PASS: `vdx7_all_tests` build and Windows `VDX7_VST3` build, not installed.
- PASS: eight Python source-packager tests and ordinary checksum test.
- FAIL (environment): complete ten-test Python invocation hits WinError 1314
  when creating a test symlink. This is not a product assertion failure;
  the symlink check is NOT RUN locally and must pass in remote CI.
- Full local CTest run excluding the interactive `vdx7_processor` test:
  IN PROGRESS when this record was prepared; update with the final result.
- NOT RUN: REAPER, interactive SAVE AS, macOS local runtime, concurrent
  public state-install stress, final 1.0.1 binaries and installer upgrade.
- Remote PR checks and merge: pending; do not infer PASS from local results.

Commands: configure with `VDX7_ENABLE_ROM_TESTS=ON` and an existing private
48 KB v1.8 fixture; build `vdx7_all_tests VDX7_VST3`; run
`ctest -C Release --output-on-failure --no-tests=error -E '^vdx7_processor$'`.
Run Python checks with `python -m unittest discover -s Tests -p 'test_*.py' -v`.
The first Windows multi-target build hit an inherited Path/PATH environment
collision (MSB6001). Normalizing the child environment resolved it; no project
source workaround or system setting change was needed. The pre-existing
test-only C4805 remains tracked as A8, not silently suppressed.

No editor appearance, parameter identities, installed plugin, release asset,
tag or branch protection was changed. The main execution plan tracks the
remaining A3–A10 work and separate exact-candidate acceptance.
