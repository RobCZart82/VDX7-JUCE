# Corrected MONO instance isolation — 2026-09-24

Added an interleaved two-processor regression to the corrected processor suite.
Both instances use the verified local v1.8 ROM and opt-in MONO correction.
Instance A holds Note 0; instance B holds Note 72. Resetting A must silence
and retire A without changing B's note ownership, sounding audio or persistent
settings. B's subsequent Note Off must release its note and reach silence.

Build succeeded. Targeted CTest run: firmware profile and corrected processor
suite passed (2/2, 46.45 seconds total). This is sequentially interleaved
processor coverage, NOT parallel execution or 1/4/8-instance DAW acceptance.
Production code and native MONO behaviour are unchanged.

Separate local REAPER 7.80 smoke evidence on the preceding production build:
44.1 kHz / 512 samples; corrected MONO state survived project reopening.
Offline and Online Render completed the repeated Note 0/subsequent Note 72
sequence, with nonzero subsequent audio and exact silence in the final
13–14.9 second window. These checks do not prove pitch/legato priority,
physical pedal operation, or multi-instance DAW performance. Local project
and factory voice state are not included in the repository.
