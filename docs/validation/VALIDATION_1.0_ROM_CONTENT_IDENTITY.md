# Project-state ROM content identity (F5)

Status: baseline failure reproduced; fix and focused local-ROM tests
implemented; public CI and review/merge are still pending.

## Reproduced failure

Baseline: `29ab5e350019e32f64650b068afcc7709b409607`.

The ROM-backed regression saves project state from one combined firmware/factory
image, changes the contents at the same path or makes that path unavailable,
and then offers a different but valid ROM. Before the fix, a loaded alternate
ROM could receive the saved RAM/program state; the focused test failed on the
baseline with `saved ROM identity should remain pending when a different ROM
is loaded`.

## Behavior implemented

- New saved states include a versioned SHA-256 identity for the firmware and
  the factory voice image actually installed. Firmware-only images include a
  distinct no-factory marker; a valid companion voice file participates in the
  identity. Thus the identity reflects the effective sound library, not path.
- If a saved identity differs from the installed ROM, the project state stays
  pending and the UI status explains the mismatch. A later load whose content
  matches resumes restoration, including when the file path is different.
- States predating `romIdentity` remain path-based to preserve existing
  behavior: a recorded path must match the currently loaded image path before
  the pending state is restored. Pathless legacy states retain their prior
  behavior because no identity is available. Legacy states cannot be
  content-verified retroactively; new saves gain the stronger check.
- Hashing is confined to ROM load/state capture, not the audio callback.

## Local verification

Using the existing private v1.8 ROM fixture (not copied into the repository):

- Baseline reproduction: failed as expected.
- Focused content-identity regression: PASS for changed contents at the same
  path, a missing saved path while an alternate image is loaded, and a later
  matching image loaded from another path.
- Existing concurrent state/RAM-to-ROM-path interleaving regression: PASS.
- `vdx7_ci_checks` build: PASS.
- ROM-free CTest: 10/10 PASS.
- The broader ROM-backed stability run initially exposed incorrect assumptions
  in two existing test assertions: delayed-timeline activity also represents
  paused samples during contention, even when all input events were filtered.
  The test now drains that empty timeline before checking it. It also restores
  Native MONO after iterating both compatibility modes. This changed test
  setup/assertion logic only; the processor’s range-filter behavior was not
  changed by that correction.
- After adding the F6 regression described in
  `docs/validation/VALIDATION_1.0_NO_ROM_FIRST_EDIT.md`, the complete local-ROM
  stability executable passes.

Still required: review the patch, public Windows/macOS CI, and exact-candidate
retest. These results are local to the current unmerged branch.

## Legacy-path regression follow-up — 2026-09-26

Source review found the first F5 implementation treated every state without a
content identity as a match, even when its recorded path differed from the
currently loaded ROM. The predicate now compares recorded paths for legacy
states, and the focused regression additionally checks that a pending legacy
project resumes when its original path becomes available. Using the private
local ROM fixture, the updated focused pending-identity test, the concurrent
state/RAM-path interleaving test and the complete ROM-backed stability
executable all passed. ROM-free CTest also passed 10/10 and `git diff --check`
is clean. The ROM remains outside the repository. Public CI and review are
still required.
