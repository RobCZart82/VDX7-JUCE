# MONO correction: project-state persistence

2026-09-24. Baseline local c4fdefd0c280a6d1e2491682fba065396ab7b9c9,
remote Draft #47 92dfa4f9f23df9fe7ba6ad4ca0da9949cb6bcc12.
No GUI selection, installed-plugin replacement or release acceptance.

## Implemented boundary

- State stores `monoNoteZeroCorrection` intent, not firmware-dependent activity.
- Missing property selects native behavior for legacy projects. Invalid values
  are rejected before changing the existing project.
- Fresh-instance restore configures intent before ROM loading. Missing-ROM
  pending states retain it through resave and later explicit ROM loading.
- Changing intent on an already-loaded instance occurs only at destructive
  project restore, under engine ownership. The current firmware and factory
  voices are copied, reloaded and booted, then the project's saved state is
  applied. This prevents switching policy beneath occupied slots. Current-image
  reload also handles an unavailable saved ROM path. This is not a seamless
  performance control and performs non-RT work, like existing state restore.
- Every image load retains whole-image and six-site compatibility checks.
  Unknown firmware cannot activate the correction. No ROM bytes or MIDI pitches
  are rewritten. Native defaults and direct pre-load selection remain.
- This does not fix pre-existing concurrent state-restore transaction concerns
  or add a live GUI toggle/automation parameter.

## New actual-processor coverage

Fresh instance restores the serialized enabled setting and plays/releases zero.
A second fixture has no saved ROM path: remains inactive/unloaded, resaves
intent, then activates after the user's ROM is loaded and plays/releases zero.
Both restore a legacy project over a held corrected zero with an unavailable
saved path, verify native intent/activity and cleared MONO count, restore the
corrected state again, and reject malformed policy without losing that mode.
Existing held-note lifecycle and 36 rate/block pitch fixtures remain enabled.

Initial targeted run: profile and corrected suite PASS (23.69 s), unchanged
native acceptance FAIL (0/1/16); 2/3, exit 8, 24.52 s.
Full rebuilt local suite: 26/28 PASS, exit 8, 331.49 s. Failures were the
unchanged native acceptance (0/1/16) and a desktop-access prerequisite in the
processor GUI test. The SAME processor executable rerun with desktop access
passed (1/1, 8.30 s). Thus all 27 non-native-acceptance groups passed across
these runs, not a claimed single 27/28 desktop-enabled full run. Corrected
persistence/lifecycle/matrix suite passed 22.61 s in the full invocation.
Rebuilt ROM-free CI aggregate: runtime 7/7 PASS, 2.52 s.

## Remaining

Visible settings selection and disclosure, safe user-initiated transition
workflow, unknown-image product status, broader restore/concurrency and DAW
acceptance. The original native Note 0 failure remains visible and P1/release
acceptance remains open. Keep #47 Draft; do not merge as a finished release fix.
