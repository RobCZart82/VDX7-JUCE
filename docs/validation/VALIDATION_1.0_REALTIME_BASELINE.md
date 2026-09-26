# Milestone 3A — keyboard handoff, stress grid and SRC baseline

2026-09-20. Baseline: merged PR #7, main
`226d3298579176853b20f111665c0cc940d7d9c1`; both macOS and Windows Actions
passed before this chapter. This is a development checkpoint, not completion
of milestone 3, host acceptance or a release. Firmware-faithful scope remains.

## Reproduced and fixed keyboard lock

Previously `processBlock` called JUCE's `MidiKeyboardState::processNextMidiBuffer`.
That function takes the keyboard lock, invokes listeners, constructs MIDI
messages (including potentially allocating copies of SysEx), and can grow the
host MIDI buffer when injecting UI events. A controlled experiment held the UI
keyboard listener/lock for 50 ms: the audio callback remained blocked.

Audio no longer calls `MidiKeyboardState`, including panic/lifecycle visual
cleanup. Its UI listener sends three-byte note events through a fixed 256-entry
queue, with producer calls serialized by JUCE's keyboard lock and one audio
consumer. Events enter at the next block start (no added 30 Hz polling delay).
Host MIDI retains its original sample positions. During engine contention both
sources use the existing deferred timeline. Audio never modifies/grows the host
MIDI buffer to inject keyboard events.

Overflow drops the stale keyboard batch and requests deferred all-notes-off
reconciliation. New notes are accepted again after recovery. This is bounded
fail-safe behavior, not lossless handling of arbitrarily large UI bursts.

Audio publishes atomic channel/note masks. A processor-owned 30 Hz message-thread
timer mirrors them into the UI; a thread-local guard prevents feedback into the
audio queue. Independently tracked physical UI holds prevent a timer tick from
erasing a newly pressed key before audio consumes it. A physically held UI key
may remain visually down after an audio panic/restart, but is not retriggered;
releasing/repressing it starts a new note. MIDI keyboard API is UI-thread-only.
The audible MIDI path does not depend on this timer or on an open editor.

Tests cover queue reuse, concurrent handoff, saturation/recovery, UI on/off,
timer-before-audio, channel-2 host feedback without echo, and callback completion
while the keyboard UI lock remains held. The latter uses a generous one-second
deadlock timeout, not a realtime deadline claim.

## Repeatable local-ROM stress grid

`vdx7_stress_tests <local-ROM>` runs 44.1/48/96 kHz × 64/128/256/512 samples.
At each rate the same absolute MIDI and automation timeline must produce exactly
identical samples at all four block sizes. It includes 16-note chords, frequent
modulation/pitch bend, sustain, program/bank changes and feedback automation.
96 kHz event positions are doubled to keep notes long enough for firmware serial
and envelope response; this is not a cross-rate sample-equality assertion.

An independent second instance renders silence and retains its bank/program.
Assertions cover finite dual-mono output, non-silent output, final selection,
consumed MIDI and released note/sustain ownership. Durations are only about half
a second per scenario; this does not replace long-running host/physical-MIDI or
simultaneously scheduled multi-instance tests. Wall-clock callback maxima are
printed as diagnostics and deliberately are not scheduler-sensitive pass gates.

## ROM-free production SRC measurement

`vdx7_resampling_tests` feeds generated sine samples into the existing engine's
native-sample buffer and calls the production resampler. It never runs firmware
or uses presets. One second of settling precedes a one-second coherent spectral
measurement. Results are relative to the measured 1 kHz amplitude, not dBFS.

| Output rate | Synthetic input / measured component | Relative amplitude |
| --- | --- | --- |
| 44.1/48/96 kHz | 10 kHz fundamental | -1.19 dB |
| 44.1/48/96 kHz | 20 kHz fundamental | -5.02 dB |
| 44.1 kHz | 23 kHz input folds to 21.1 kHz | -6.79 dB |
| 96 kHz | 10 kHz input creates a 39.096 kHz image | -24.88 dB |

This confirms inadequate stopband suppression in linear interpolation; it is
NOT proof of audible severity on every patch, nor a comparison with physical
DX7 hardware. The test checks finite output, valid reference gain and bounded
gain, but intentionally does NOT call these alias/image levels acceptable.
Both CI workflows now build and run this ROM-free probe.

Decision: retain this measured baseline, then evaluate a band-limited SRC with
explicit passband/stopband and CPU targets. Check onset, latency reporting,
partition invariance and reference renders before changing the shipping path.
No SRC algorithm or firmware sound generation changed in this checkpoint.

## Remaining audit / gates

Local macOS arm64 Release result: 9/9 CTest passed in 35.67 seconds, including
five opt-in local-ROM tests. The development VST3 rebuilt, was ad-hoc signed and
passed strict signature verification. The installed plugin was not touched.

- Removed the known keyboard lock/listener/allocation path from `processBlock`.
- Engine access still uses a non-blocking try-lock. JUCE load metering uses
  `ScopedTryLockType` in `registerRenderTime`; lifecycle reset is non-realtime.
- Fixed MIDI/edit queues and engine sample buffers remain bounded. The firmware
  serial queue and edit bursts still need sustained overload characterization.
- This is source inspection plus targeted contention tests, NOT exhaustive
  allocation instrumentation, race-detector coverage or a hard-realtime proof.
- Complete remaining allocation/overload audit and implement/validate SRC before
  closing milestone 3; then firmware-backed PERFORMANCE/SETTINGS in milestone 4.
- New-source macOS/Windows CI and PR approval are required before the next chapter.
- No firmware, installed plugin replacement, tag or release is included.
