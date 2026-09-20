# USER-bank GUI integration (development)

## Implemented behavior

- One persistent SAVE AS header button opens a dialog. Its default action saves
  the captured single patch to USER; Export Patch and Export Bank use the existing
  SysEx file chooser. Name and destination-slot fields apply to USER saving only.
- Capture happens before opening the dialog, under the engine lock after pending
  edits are flushed. Later program changes cannot substitute another USER patch.
- USER has 32 slots. The dialog shows occupancy/names and selects the first empty
  slot. An occupied destination requires a second explicit confirmation; storage
  rechecks the target under its locks. Conflicts require reopening the dialog.
- Cancellation does not create a library directory or write the bank. Saving
  creates the per-user `VDX7-JUCE/User Banks` folder and stores `USER.vub` there,
  under JUCE's userApplicationDataDirectory (plus Application Support on macOS).
- No factory ROM is written. A successful copy deliberately retains current
  working-bank edit markers: saving a copy is not committing the working bank.
- The LCD bank menu offers `USER (load copy)`, with the existing unsaved-edit
  warning. Loading chooses the first occupied slot and creates an independent
  CUSTOM working bank; empty slots are silent placeholders. Reload explicitly
  rereads disk. Editing the working copy does not automatically save the library.
- Project state embeds the loaded working bank, not a dependency on the library
  file. Global PERFORMANCE settings survive loading and remain project-only.
- UTILITY no longer duplicates exports; it retains rename and operator copy/paste.

## Verification

Local macOS arm64: all 10 CTest tests passed in 50.68 seconds with desktop access.
The VST3 build completed; its local ad-hoc signature was refreshed with codesign
and passed strict verification (the build helper emitted Xcode licence warnings).
No Xcode licence setting was changed. The installed plugin was not replaced.

Processor integration tests cover immutable capture, one-patch copy, unchanged
working RAM/dirty markers, library load, first occupied slot, global preservation,
project restore, working-copy independence, and corrupt-file rejection.
GUI construction tests open SAVE AS, check its default action, 32 slots and name,
then cancel. Optional test screenshots include the actual SAVE AS dialog.
These dialog tests require desktop/display access; a headless sandbox is rejected
with an explicit test error before constructing the native dialog.
Storage overwrite/conflict/concurrency tests remain ROM-free in both CI builds.

## Remaining acceptance

This is functional wiring on the existing skin, not the final hardware-inspired
GUI. Real REAPER interaction and Windows GUI acceptance remain required. The
library currently has one fixed USER bank, not a named-bank manager. USER exports
are made by loading its working copy and exporting that bank; SysEx cannot carry
occupancy metadata. No firmware, release, tag or installed-plugin replacement is
part of this change.
