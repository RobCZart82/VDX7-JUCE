# CC32 bank-selection correction

## Behavior

- CC32 values 0-7 select the eight available factory banks directly.
- Values 8-127 are ignored rather than wrapped modulo eight. Unsupported banks
  are rejected before serial capacity checks, so they cannot themselves trigger
  overflow recovery when that queue is full.
- The processor clears unexported-voice markers and publishes bank state only
  when engine factory-bank RAM was actually replaced. An engine-thread revision
  token records successful loads, including reloading the same bank. It is not
  a host parameter or saved-state field.
- Factory-bank loads still replace the working bank, including its edits. This
  change does not introduce automatic saving or a MIDI confirmation dialog.
- Compatibility policy is unchanged otherwise: CC0 is ignored, CC32 acts
  immediately, and subsequent Program Change selects the program. This is the
  plugin's eight-bank mapping, not a claim of full 14-bit bank selection.

## Regression coverage

Local macOS arm64: all 10 CTest tests passed in 57.92 seconds, including the
ROM-backed integration and GUI tests. VST3 build and strict ad-hoc signature
verification passed. The build helper still reports the existing Xcode licence
warning; no system licence/settings were changed. Windows and fresh GitHub CI
are not covered by this local result.

`VDX7StabilityTests` checks ignored 8/15/127 values against a dirty CUSTOM bank
and edited factory banks: bank/program markers, dirty state and all 4096 voice
bytes must survive unchanged. It checks 0/7 boundary loads, same-bank reload,
factory byte equality, and CC0=0 -> CC32 -> Program Change. The engine overflow
test checks that unsupported banks do not trigger recovery with a full queue.
These tests use the user's local ROM, which is never committed or distributed.

The broader malformed-MIDI, mixed host automation and overflow UI investigations
remain separate tasks. The CUSTOM test uses a live SysEx working copy; it does
not claim a dedicated USER-library GUI save/reload acceptance test.

No plugin installation, release, tag, firmware modification or main-branch push
is part of this chapter. Fresh PR checks and user merge are required.
