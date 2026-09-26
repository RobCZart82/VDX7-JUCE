# GUI/function integration 1 — LCD, algorithm and Save As

Development only; not a final skin or release. Based on merged PR #10
(`8fbfd146a63a3b39c2d32bb6fc629cc7fc1d94ec`); its PR and main macOS/Windows
workflows were successful before this chapter began.

## Implemented

- Removed duplicate header patch display. Existing previous/next callbacks now
  belong to buttons immediately beside the LCD; bank-local 01/32 wrap retained.
- Replaced GLOBAL algorithm encoder and number label with an explicit 1–32
  ComboBox attached to the existing algorithm host parameter. No processor
  parameter IDs, ordering, defaults or firmware behavior changed. Feedback and
  operator-node selection remain connected to the diagram.
- Persistent SAVE AS... header button opens voice-file or 32-voice-bank-file
  export via the existing validated SysEx implementation. Disabled without ROM.
  Native save dialog retains overwrite confirmation, cancellation and the
  existing voice-change guard. Utility exports remain accessible.
- No factory ROM is modified. This is **file export**, not USER-bank library
  management, bank-slot Save As or performance-state export. Those are explicitly
  recorded as subsequent chapters in ROADMAP_1.0.md.
- Current skin and disabled PERFORMANCE/SETTINGS tabs remain. The accepted
  performance mockup is not represented as working code. No new placeholder
  active controls have been introduced.

## Local verification

macOS arm64 Release: **9/9 CTest passed, 49.11 seconds**.

Expanded processor tests cover ROM-missing disabled controls, the 32 selector
items, host-to-selector/diagram and selector-to-host/diagram/RAM for all 32
algorithms, removal of duplicate ALGO encoder, and LCD 01/32 navigation wrap.
Existing tests cover legacy indices, state/RAM restore, all voice parameters,
rename, single/bank export/import, failed-I/O dirty protection, operator selection,
and rendering. UI checks run at 960, 1440 and 1600 pixel widths. The 960-pixel
editor snapshot was visually inspected for LCD, header and algorithm layout.

VST3 built successfully; direct ad-hoc signing and strict verification passed.
The build helper emitted the pre-existing Xcode licence warning; no licence or
system settings were changed. Installed plugin was not replaced. Local ROM was
used only for integration tests and is not included in source or outputs for GitHub.

## Remaining acceptance

- Fresh remote macOS/Windows CI for this exact PR commit.
- Real DAW/native-dialog interaction and keyboard/accessibility acceptance.
- Firmware-backed PERFORMANCE and SETTINGS with state compatibility.
- USER bank library / slot destination / naming and overwrite-safe Save As.
- Concept-3 visual redesign, cross-view navigation and host usability.

No release, tag, factory payload or installed binary is part of this chapter.
