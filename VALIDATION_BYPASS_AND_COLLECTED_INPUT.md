# Bypass and collected-input follow-up — 2026-09-24

## State boundary control

Extracted the existing epoch observation and post-lock collected-input rejection
into private methods used by processBlock. A deterministic friend-access test
calls the same methods before/after the actual locked restore routine: unchanged
epoch retains both host MIDI and keyboard count; intervening installation clears
both. No test hook, wait or extra lock was added to the audio callback.
This verifies the shared boundary logic, not an OS-scheduled pause inside a live
public callback. Public concurrent restore/GUI/DAW stress remains open.

## Bypass defect and correction

Real processBlockBypassed fixture: audible Note60 -> bypass with NoteOff and
CC64 release -> normal processing. Before override, FAIL (exit1):
`bypass lost NoteOff/pedal release and revived held note`.
The inherited implementation did not advance this synth's MIDI/firmware path.

Override now uses normal processing then clears audio and meters. Bypass remains
silent, while releases, pending resets and firmware time continue normally.
This deliberately retains processing CPU cost; it is not a sleep/CPU-saving mode.
MIDI received during bypass is consumed by this instrument, just as normally.
Still-held notes may be audible on resume; released notes must not revive.

Two fixtures (pedal absent/present) check initially audible sound, bypass silence,
consumed MIDI, no ordinary C++ allocations/deallocations in bypass callbacks,
empty firmware/adapter ownership and silence on resume, plus fresh Note72
sound/release. Registered through the existing deferred-processor test group.

Scope: 48kHz/64, local verified v1.8 image, native POLY fixture. This does not
establish all-host behaviour, suspended-callback recovery, bypass/reset/restore
overlap, other rates or corrected MONO pedal behaviour. No installed plugin change.

Rebuilt affected processor/test code: targeted CTest profile, deferred group
(including the two new controls) and corrected MONO group **3/3 PASS**, exit0.
No full-suite, desktop GUI or DAW run. Native Note0 remains separately open.
