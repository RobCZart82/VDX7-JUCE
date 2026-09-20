# ROM recovery fixes — 2026-09-20

Scope: P1-2 and P1-3 from the 1.0 RC review, plus documentation corrections.
This is an incremental development checkpoint, NOT RC acceptance.
Base source: `5164fabd36c8fdd745e272fc1f493c0c352c1ced`.

## Reproduction and implementation

Before fixes, the local integration test failed on:

- `missing-ROM re-save includes new host parameter`
- `invalid optional companion permits firmware load`

Missing-ROM saves now capture the current APVTS state. Explicit operator/voice
edits made after restore are recorded in an additive `DeferredVoiceEdits` child.
They are applied after restoring original RAM when firmware becomes available;
untouched parameter defaults do not overwrite legacy RAM. This covers both
direct firmware loading and save/restart/loading. Existing parameter identities
and the original state root/RAM encoding are unchanged. Older plugin versions
ignore this new child and cannot apply these additional offline edits.

Optional factory data is size-checked before reading and validated after reading.
A missing companion permits firmware-only loading. Invalid/unreadable companion
data is ignored with a visible warning; a valid companion still loads the banks.

## Local evidence

- macOS arm64 Release: expanded stability test PASS.
- Complete local CTest suite: 7/7 PASS, no skipped tests (final run 30.94 seconds).
- Covers master re-save/restart/firmware load; exact RAM/bank/program preservation;
  explicit voice/operator edits with/without intervening save; invalid, valid
  and absent companion behavior.
- Test firmware is generated only in a unique temporary directory, cleaned on
  normal completion or exception, and is never packaged or uploaded.
- No installed plugin, release, tag or existing release asset changed.

## Not covered / still open

P0 program/bank/edit ordering and audio-thread host notifications remain open.
Deferred MIDI timing, lifecycle cleanup, out-of-range program normalization,
dense/multi-instance stress, SRC measurement and actual REAPER acceptance remain
open. This passing suite must not be interpreted as validation of those issues.

New-source GitHub CI and exact-commit RC workflow are pending, not claimed PASS.
The earlier main commit's CI results are recorded separately in
`VALIDATION_1.0_DEV.md`. Use `RELEASE_CHECKLIST_1.0_RC.md` for the remaining gates.
