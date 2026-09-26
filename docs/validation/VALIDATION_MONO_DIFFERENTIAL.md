# Native/corrected MONO nonzero differential

2026-09-24. Test-only comparison at 48 kHz / 64 samples through actual
processBlock. Four independent paired fixtures: sustain off/on x portamento
off/on (time 0/64, MIDI CC65 0/127). Each pair uses identical initialization,
patch, events and callback partitions; only explicit correction selection differs.

Input sequence: Note1/60/127/60 On, then 60 Off, velocity-zero127, 1 Off, 60 Off,
pedal release. No Note0. This includes low/high endpoints, repeated nonzero notes
and overlapping legato. Every rendered sample on both channels is compared
(absolute tolerance 1e-6); both paths must genuinely sound, finish silent and
clear ownership. Every callback also compares MIDI/held/sustained/MONO counts
and native target pitch. Ordinary callback allocation/finite-output checks apply.

This is not proof for all patches, sample rates or MIDI schedules. In particular
it does not establish physical-hardware equivalence, zero-note portamento glide
accuracy, overloaded FIFO behavior or actual DAW acceptance. No production code
change. Existing corrected acceptance tests and native failing gate remain.

Rebuilt host-reset executable. All four paired fixtures PASS with maximum
sample difference exactly 0 in this run. Focused CTest: profile PASS0.67s,
corrected suite PASS41.18s; unchanged native acceptance FAIL0/1/16 (0.15s).
Combined 2/3, exit8,42.01s. No full-suite or DAW rerun in this test-only round.
Previous-head macOS/Windows Actions both succeeded before upload.
