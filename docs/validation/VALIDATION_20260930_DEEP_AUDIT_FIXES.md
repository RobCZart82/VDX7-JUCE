# VDX7 deep audit fixes and validation

Date: 2026-09-30. Baseline: main
`3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0`, after PR #107.
The corrective branch is `codex/1.0.1-deep-audit-fixes`.

This round fixes the four reproduced P2 findings in the
[original Hungarian audit](VALIDATION_20260930_DEEP_AUDIT_HU.md).
It preserves the approved GUI, parameter identities/order and saved-state
format. Local validation and independent code review are complete. GitHub
Windows/macOS and ASan/UBSan checks must pass on the final PR head before merge.
This record does not authorize or certify publication of 1.0.1; the existing
1.0.0 tag, assets and installed plugins remain unchanged.

## Changes and regression coverage

### State snapshots and ROM installation

S1 detaches the no-ROM deferred-edit values and masks inside the same engine
lock as the loaded/RAM/ROM generation. XML serialization remains outside the
lock, and saving does not consume the live edit masks. The regression covers
feedback and operator values in both mask halves, a first ROM load overlapping
save, newer live edits versus the saved generation, and save/reopen controls.

S2 removes unconditional dirty-mask clearing after firmware/RAM installation.
The pending project recaptures edits after the RAM restore; edits received after
that capture remain pending for normal application. Mailbox callbacks also
request host publication, including callbacks that finish after the loader
publishes its view. The matrix covers a fresh first load and pending projects
with/without packed RAM, before/during/after warm-up and RAM installation,
final edit application, host publication and save/reopen.

The runner uses compile-gated scheduling hooks, absent from the shipping
plugin. Its delayed-listener routing case invokes the production callback
through the existing test friend: an ordinary JUCE parameter adapter holds an
additional listener mutex that would prevent this particular loader schedule.
This isolated callback test is distinct from the matrix using normal parameter
and state APIs. It checks both host parameter views before any state-save-forced
sync, using the production publication predicate and a real audio callback.

### CC120 event timeline

M1 anchors deferred playback at the triggering CC120 sample and advances the
completed input callback exactly once. An empty reset tail retains that anchor
until the reset completes. Tests check the precise relative event offset,
next-block Note On/Off and expression, repeated resets, empty tails, aggregate
versus split callbacks and real capacity overflow. New ROM-free helper cases
also execute in the existing deferred-MIDI sanitizer target.

### CC121 controller reset

M2 retains six mandatory zeros outside the application FIFO, then submits them
through the existing owner-thread handshake after older messages drain. Newer
controller values have separate bounded storage. In particular, a later sustain
ON cannot erase the OFF edge needed to release previously sustained notes.
An explicit host/panic reset supersedes old pedal ON intent; CC123/direct
All Notes Off supersedes only old sustain ON, preserving other controllers.
Partial delivery interrupted by a reset/overflow preserves later analog values.

GitHub review also identified post-CC121 notes overtaking a deferred sustain
ON. A direct-engine held-note release reproduced sustain loss without overload.
The processor now pauses the
existing bounded MIDI timeline at CC121, retains all followers together and
resumes only after the controller-reset transaction drains. Unlike CC120, this
wait renders ordinary audio rather than muting existing voices. The engine
exposes its busy state to its owner; direct engine callers must obey this
sequencing boundary, as they already do for the host-reset boundary. No second
engine event queue or heap allocation is added.

Two additional processor regressions check sustain ON before held/fresh note
releases, and an earlier release fully consumed before pedal ON in a later
callback. The new runner linked against committed `794b933` fails its retained
sample-order assertion. That particular host burst does not fail firmware
ownership on the old source; it is ordering-invariant evidence, distinct from
the direct-engine sustain-loss witness. The corrected runner passes all six
ROM-backed timeline cases, including actual firmware ownership in both
directions and later pedal-OFF cleanup, with no overload recovery.

The final 24 regression cases include FIFO occupancies 0, 1018, 1020, 1023 and 1024;
repeated CC121; later input during acknowledgment; newer sustain ON/OFF; a fresh
note lifecycle; mandatory-zero and post-acknowledgment handshake interruption;
and CC123/direct All Notes Off. They inspect actual v1.8 pedal/analog/voice
state and the data-controller handshake, and preserve physically held notes,
packed voice RAM and persistent performance settings.

The second GitHub review found that the host gate could reopen before the
pitch/modulation reset reached firmware. The transaction now keeps that gate
closed while the existing wheel branch submits pending values and completes
its handshake. For verified v1.8, completion also waits for the firmware main
loop: transport `haveMsg` can clear before the IRQ stores/scales its input.
Four tests inspect the very first gate-clear sample, with empty/full FIFOs and
newer wheel input. The previous source fails the pending/handshake assertion;
a transport-only intermediate fix fails the firmware wheel-value assertion.

Independent review additionally found that late input through the ordinary
FIFO could inherit an older fallback completion timer. The final fallback
interval restarts after each actual controller submission. Two tests inject
late wheel/breath input during the final interval, require a fresh 16384-cycle
wait after the last transfer, and inspect firmware values at gate reopening.
The late-wheel case fails before the timer fix; both pass afterward.

Pedal acknowledgment uses a firmware address only for the verified v1.8
profile. The unrecognized-profile path uses conservative emulated-cycle pacing.
Forcing that path on the same v1.8 image passed; this is not validation of
arbitrary firmware or proof of an acknowledgment address for other ROMs.

## Executed validation

The local environment is macOS 26.7 arm64, Apple Clang 21.0.0, CMake 4.4.3 and
Python 3.9.6. A new offline Release build directory was configured against the
corrective checkout; JUCE/core sources use the repository's dependency pins.
Firmware tests use the owner's private combined 48 KB v1.8 ROM. No firmware
bytes or personal ROM paths are committed.

- The new state-save, pending-installation and CC120 regressions fail against
  the original baseline. The CC121 runner passes its empty/1018 controls and
  fails at occupancy 1020 on baseline. Delayed-publication and interrupted
  controller cases also failed before their corresponding fix. A failing
  assertion returns exit 1; passing runs return exit 0.
- The final `vdx7_ci_checks` build and executable CTest suite pass: **43/43**
  in 178.55 seconds with three parallel tests.
  There are 44 registered tests: 13 ROM-free and 31 local-ROM. The existing
  desktop/save-dialog runner `vdx7_processor` is excluded, not counted as PASS.
- Python source-packaging/checksum regression suite: **12/12 PASS**, no skip.
- CTest inventory, labels, fixture/timeouts/failure policy and seven negative
  registration controls: **PASS**.
- A separate fresh ASan/UBSan build passes all **24 CC121 cases**, plus the
  deferred-MIDI and resampling component CTests (**2/2**). LeakSanitizer is
  disabled on this platform. This is not full-plugin sanitizer or TSan coverage.
- Independent cross-review of the state, CC120 and final CC121 changes found
  no blocking correctness regression. `git diff --check` is clean.

The narrow sanitizer build does not build the GUI-linked ROM-profile fixture
runner. An initial CTest selection therefore correctly reported that fixture
and dependent controller test NOT RUN. The corrected invocation runs the
instrumented CC121 executable directly; it independently requires the verified
v1.8 profile before its firmware assertions. The regular suite runs the fixture.

Reproduction commands use local-path placeholders:

```text
cmake --build <release-build> --target vdx7_ci_checks -j4
ctest --test-dir <release-build> --output-on-failure --no-tests=error -E '^vdx7_processor$' -j3
ctest --test-dir <release-build> -N --show-only=json-v1 > <inventory.json>
python3 scripts/check_test_registration.py <inventory.json> --self-test
python3 -m unittest discover -s Tests -p 'test_*.py' -v
cmake --build <asan-build> --target vdx7_controller_reset_tests vdx7_deferred_midi_tests vdx7_resampling_tests -j4
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 <asan-build>/vdx7_controller_reset_tests <private-ROM>
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir <asan-build> --output-on-failure --no-tests=error -R '^vdx7_(deferred_midi|resampling)$'
```

## Merge gate and remaining scope

Public CI compiles all new runners without firmware, executes the 13 ROM-free
tests, validates optional ROM-test registration, and runs packaging checks.
Its separate ASan/UBSan workflow instruments eight non-GUI component targets,
including the new deferred-MIDI helper assertions. Public CI does not execute
private-ROM tests. Final PR checks and review conversations must be clear;
the merge is pinned to the checked head. Post-merge platform builds must finish
before another merge/publication step is started.

No new REAPER/desktop-host test, installed-plugin replacement, Windows private
ROM run or final 1.0.1 binary/installer acceptance was performed locally.
Broader host, architecture and unknown-ROM acceptance remain deferred.
The existing A4–A7 work and exact-candidate gates remain in the
[single execution plan](../release/EXECUTION_PLAN_1.0.md); these four runtime
fixes do not close unrelated provenance, installer-migration or release gates.
