# v0.7.0 development: firmware MIDI note input

The v0.6.6 release/tag and installed plugins are not modified.

The previous adapter discarded notes outside MIDI 36–96 and passed keyboard
indices 0–60 to the emulated sub-CPU. The v0.7 development adapter then forwarded
MIDI 0–127. The current product policy is MIDI Note 12–120 inclusive.
Velocity mapping remains the existing curve. Host channels retain legacy omni
behaviour, mapped onto the firmware receive channel (not multitimbral/MPE).
Zero-velocity note-on is normalised to note-off. All Notes Off releases tracked
notes across the entire range and clears sustain, including pedal-held releases.

Serial MIDI has finite baud-rate latency: dense chords are delivered sequentially,
not sample-simultaneously. The existing upstream serial FIFO remains finite;
arbitrary MIDI floods are not claimed to be supported. No firmware, EGS frequency
tables or synthesis algorithm changes are made. No pitch wrapping/transposition
workaround is used. The onscreen keyboard retains its existing visual range.

Current product boundary: Note On, Note Off, and velocity-zero Note On below 12
or above 120 are filtered in both MONO compatibility Settings modes (Native and
Correct MONO Note 0). Rejected pitches are
never transposed to another note. The lower cutoff protects against the observed
MONO Note 0 lockup; 0–11 is excluded as a full low octave, and 120 is the inclusive
upper limit by product decision. REAPER uses C0 for Note 12 and C9 for Note 120
with its default octave labels. Automated processor tests cover both modes and
range boundaries; new REAPER boundary acceptance remains outstanding.

Settings policy: Native firmware is the default and recommended everyday mode.
Correct MONO Note 0 remains available as an advanced, under-the-hood compatibility
option; ordinary users are not expected to toggle it. The 12–120 input boundary
is identical in both modes, and the advanced option does not restore Note 0–11.

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

A 0–127 tartományt továbbító v0.7 fejlesztési változat után az aktuális
termékpolitika a Note 12–120 tartományt engedi a hangmotornak. A firmware és a
hangképzés változatlan. Az összes
hostcsatorna továbbra is ugyanazt a hangszert vezérli; ez nem MPE vagy multitimbral
mód. A képernyő-billentyűzet mérete változatlan. Sűrű akkordoknál a MIDI soros
átvitele kis időbeli eltolódást jelenthet. A régi kiadás érintetlen, az új változat
REAPER-es tesztje és platformonkénti elfogadása még szükséges.

A Note 0–11 és 121–127 Note On/Off, illetve velocity-zero Note On eseményei
mindkét SETTINGS-kompatibilitási módban (Native és Correct MONO Note 0) szűrtek;
nincs transzponálás. A határok Note 12 / C0 és Note 120 / C9 a REAPER
alapértelmezett oktávelnevezésével. Az új REAPER-es
határteszt még hátravan.

A firmware-hű Native az alapértelmezett és ajánlott normál használatra. A Correct
MONO Note 0 megmarad haladó, motorháztető alatti kompatibilitási lehetőségként;
általános használatkor nem szükséges kapcsolgatni. A két mód MIDI-tartománya azonos.
