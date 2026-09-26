# Prepare-time MIDI epoch synchronization

## Reproduced issue

The v1.8 ROM-backed `vdx7_stress` test failed at `UI note reaches firmware`.
`loadRomData` increments the MIDI timeline epoch after installing an image.
When the host then called `prepareToPlay`, the processor reset firmware MIDI
ownership and cleared queues, but left the audio-owned observed epoch stale.
The first fresh virtual-keyboard note queued after preparation was consequently
discarded by the first audio callback as if it belonged to the pre-install
timeline.

## Fix

`prepareToPlay` now synchronizes the observed audio epoch after resetting the
MIDI lifecycle and clears the stale deferred state-restore release request.
This is limited to the stopped-device preparation path, which the existing
host contract requires to run without concurrent audio callbacks. The
real-time `processBlock` path remains non-blocking.

## Local verification — 2026-09-26

- Before the fix, `vdx7_stress_tests <v1.8-combined-image>` failed at
  `UI note reaches firmware` both in CTest and when run by itself.
- After the fix, `vdx7_stress_tests <v1.8-combined-image>` PASS.
- Rebuilt the complete `vdx7_ci_checks` target and reran all CTest cases except
  `vdx7_processor`: **35/35 PASS**, including stress, the full local-ROM
  lifecycle/ownership/state matrix, F5 ROM identity, and F6 first-ROM-load edit
  retention.
- The omitted `vdx7_processor` test requires desktop access for its SAVE AS
  dialog and fails in this headless environment before product behavior can be
  assessed. It remains a desktop/host check, not a product-test PASS.
- `git diff --check`: PASS. Windows CI run `36269491224` and macOS CI run
  `36269491357` both PASS on commit `fed4721` (2026-09-26), including plugin
  builds, ROM-free regressions, and the no-execution local-ROM registration
  smoke. The Actions warning about actions moving from Node.js 20 to 24 is
  non-failing and unrelated to this source change.

The v1.8 fixture stayed outside the repository and build artifacts.
