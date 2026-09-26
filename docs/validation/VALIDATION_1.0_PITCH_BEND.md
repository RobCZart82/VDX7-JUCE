# PERFORMANCE pitch bend — development slice

Two selectors expose firmware RANGE and STEP, both 0-12. STEP 0 is continuous;
nonzero STEP uses firmware quantisation, whose stepped path does not use RANGE.
They are global project settings, not voice SysEx or new automation parameters.
Existing 148 parameter IDs/indices and RAM state layout remain unchanged.

## Evidence and correction

The [annotated firmware](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm)
identifies RANGE at 0x2328, STEP at 0x2329, input at 0x232A and the
PITCH_BEND_PARSE routine. The previous wrapper read an unrelated location and
forced a two-semitone fallback, then added a second EGS offset to native bending.
This bypassed range zero and could add to firmware bending. That extra offset
is now zero; the existing sub-CPU analog message lets the firmware calculate pitch.

Live setting changes request one bounded refresh after pending messages drain.
A fresh queued wheel event satisfies the refresh itself, preventing re-injection
of an older firmware RAM input before the fresh event has been processed.
The wrapper retains the latest dispatched wheel value for later refreshes;
RAM restore reconstructs that value from the saved firmware input.
No ROM bytes or dependency sources are modified or distributed.

Compatibility note: projects whose stored firmware range was zero no longer
receive the old forced two-semitone bend. Set RANGE explicitly if desired.
This is a deliberate firmware-fidelity bug fix, not identical playback of that
previous wrapper defect. The wheel path still uses seven-bit MSB resolution.

## Verification

Final local macOS arm64 CTest: **10/10 passed, 51.80 seconds** with desktop
access for the GUI checks and the user's local firmware for integration tests.

Tests render a sustained isolated carrier and measure frequency for zero, six,
twelve and negative-twelve semitones, stepped bending, centre return and a range
change with no subsequent wheel event. Processor tests cover bounds validation,
project recall, unchanged voice dirty state and real GUI selector bindings.
Existing GUI checks cover three sizes and preserve the four-controller matrix.
The 960-pixel PERFORMANCE rendering was visually inspected. Local VST3 build
and direct ad-hoc signature verification succeeded; the pre-existing build-helper
Xcode licence warning was not resolved by changing system settings.

Pending: real MIDI wheel / REAPER acceptance, expanded step-threshold tests,
mono/poly, portamento, MIDI-channel SETTINGS and master tuning. This is not a
completed PERFORMANCE/SETTINGS milestone or final graphical redesign.
