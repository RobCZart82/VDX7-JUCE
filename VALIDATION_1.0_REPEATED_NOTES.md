# Repeated-note MIDI ownership

Base: main ed6509bfea832311b12a29149751236f1f19dbf7 (PR29).

Local user-supplied firmware reproduction, sustained isolated-carrier patch:
two Note Ons at pitch 60 followed by one Note Off left peak 0.0502965, while
hasHeldMidiNotes returned false. allNotesOff left peak 0.0502973. Reproduced both
on one host channel and across channels 1/2 (normalized to the single receiver).
The added audio test failed on the old implementation.

Bounded per-pitch counts (0..16, matching maximum polyphony) now track repeats.
Normal note messages still go unaltered through the established firmware path;
this is not multitimbral/channel-separated ownership. One off decrements one
count, velocity-zero Note On is an off, and unmatched offs cannot underflow.
All-notes-off reserves enough serial space for the counted releases and sends
one release per repeat. Lifecycle/overflow release batches use known repeats
and at least one release per pitch, preserving the prior unknown-pitch safeguard.
The conservative counts may overestimate voices after stealing; they are not
an exact copy of firmware allocation. In-flight offs during overflow and broader
mono/voice-stealing cases remain follow-up acceptance, not claimed solved here.

Regression matrix: same/split channel, 2/16 repeats, true Note Off/velocity-zero
Note On, sustain off/on. Verify finite/non-silent held audio, surviving voice and
held indication after one off, then silence and cleared ownership after all-off.
Existing 0..127 pitch, tuning, bend, mono/portamento tests remain.

An initial all-pitches-times-16 panic batch regressed the existing recovery-time
test and was rejected. The final batch uses recorded counts instead.

No parameter IDs, state schema, GUI layout, installed plugin, tag or release
changes. Firmware stays local and is not included in source or artifacts.

Final local arm64 VST3/all-tests build passed; CTest 10/10 passed in 65.64s.
Ad-hoc signature/strict verification passed. The pre-existing Xcode license
warning remains; no automatic license acceptance. Universal/Windows CI and
real-host acceptance remain separate from these local tests.
