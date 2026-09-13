# v0.7.0 development: firmware MIDI note input

The v0.6.6 release/tag and installed plugins are not modified.

The previous adapter discarded notes outside MIDI 36–96 and passed keyboard
indices 0–60 to the emulated sub-CPU. The new adapter forwards valid note
messages through the firmware's serial MIDI receiver for MIDI 0–127.
Velocity mapping remains the existing curve. Host channels retain legacy omni
behaviour, mapped onto the firmware receive channel (not multitimbral/MPE).
Zero-velocity note-on is normalised to note-off. All Notes Off releases tracked
notes across the entire range and clears sustain, including pedal-held releases.

Serial MIDI has finite baud-rate latency: dense chords are delivered sequentially,
not sample-simultaneously. The existing upstream serial FIFO remains finite;
arbitrary MIDI floods are not claimed to be supported. No firmware, EGS frequency
tables or synthesis algorithm changes are made. No pitch wrapping/transposition
workaround is used. The onscreen keyboard retains its existing visual range.

Build the opt-in `vdx7_midi_range_tests` target and pass your own local combined
ROM path as its only argument. ROM data is not bundled. The test exercises all
128 notes, host channels 1–16, note-off, zero-velocity note-on, all-notes-off,
sustain release and finite/non-silent audio. An isolated ratio-1 carrier checks
representative pitches, including both endpoints. Factory-voice release checks
use the test ROM's initial bank/program; different ROM contents may need adjusted
release expectations. Existing processor integration tests remain applicable.

Manual acceptance still required: long piano-roll notes below/above the old range,
chords straddling the old boundaries, sustain, transport stop, pitch/mod automation,
project recall and export on macOS and Windows. Do not replace public v0.6.6 assets
until host acceptance is complete.

## Magyar összefoglaló

A 61 billentyűs belső protokoll helyett a firmware valódi soros MIDI-bemenete
kapja a 0–127 hangszámokat. A firmware és a hangképzés változatlan. Az összes
hostcsatorna továbbra is ugyanazt a hangszert vezérli; ez nem MPE vagy multitimbral
mód. A képernyő-billentyűzet mérete változatlan. Sűrű akkordoknál a MIDI soros
átvitele kis időbeli eltolódást jelenthet. A régi kiadás érintetlen, az új változat
REAPER-es tesztje és platformonkénti elfogadása még szükséges.
