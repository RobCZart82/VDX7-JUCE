# MONO acceptance checkpoint — 2026-09-24

## Version and policy

Tested local source: `61bd4e8bdbbca12dc396a8747782c846df72bedc`.
This adds tests only, not a production correction change.
User decision: retain native and corrected modes in Settings, with separate
acceptance requirements. This is not merge/publication approval and does not
change the native default or turn its known failing test into PASS.

## New dual-instance test

`vdx7_mono_soak` exercises two real processors with the supported local firmware,
correction enabled, 48 kHz / 64 samples. It runs 120 sustain/Note0/legato/release
cycles without resetting between cycles: 300.5 simulated seconds per instance.
The processors receive independent follow-up notes (60 and 72), velocity-zero
Note On releases, pedal release, and alternating master-volume parameter updates.
Assertions examine each processor's audio and ownership separately, including
final silence and zero MIDI/held/sustained/MONO ownership each cycle.

This is an accelerated offline processor test, not five minutes of live DAW
load or proof of sample-accurate host automation. Callback timing is diagnostic,
not an xrun oracle. The standalone run reported 25 callbacks above the nominal
budget (maximum 54873.3 us) in instrumented offline execution; this must not be
reported as a hard-realtime timing PASS.

## Results

Commands use the configured build directory and local ROM fixture; ROM paths
and raw logs remain private.

- Targeted `ctest -R '^vdx7_mono_soak$' --output-on-failure`: **2/2 PASS**,
  including the ROM fixture; exit 0, 44.06 seconds.
- Full `ctest --output-on-failure`: **28/30 PASS**, exit 8, 488.83 seconds.
- Full-run `vdx7_processor`: FAIL because the sandbox lacked desktop access.
- Separate desktop rerun `ctest -R '^vdx7_processor$' --output-on-failure`:
  **1/1 PASS**, exit 0, 9.33 seconds.
- Native `vdx7_mono_note_zero_acceptance`: **FAIL**, MIDI=0, held=1,
  MONO count=16. The original firmware path is intentionally unchanged.
- Corrected processor acceptance and the new soak both PASS in the full run.

Therefore 29 distinct tests have successful results across the full run and
desktop retry. This is NOT a single 29/29 or 30/30 successful full run.

Host-reset test executable SHA256:
`e340bddb23cd30380bfe832adcca8b40c563f9ff0c28872b58c05fbbdf12a091`.

## Additional REAPER observation

REAPER 7.80, Apple Silicon, 44.1 kHz / 512 samples. Existing workspace VST3
binary SHA256 `437da776c521f6f68b2d67de93933938b3b856d912046bf13cd35933eb4e6154`
was retained; no installed plugin replacement. A separate private project
contains a soloed instance and a generated CC64/Note0/Note60/Note72 sequence.
Saved project MIDI was inspected to verify pedal-on, legato, velocity-zero
release, pedal-off and follow-up note events. REAPER adds CC123 at item end;
this host recording is consequently NOT an independent natural-release oracle.

The live capture contains two audible sequence-length passages near its end,
peak 0.0449067354, final one-second window exactly zero. There is a long idle
lead-in (total capture 4062.389 seconds), not an hour-long played soak test.
This is limited activity/tail evidence, NOT completed transport/pedal acceptance
or a pitch check. Raw audio and project data remain outside Git.

## Still open

- Timestamped live transport boundaries during sustain/legato, including loops
  and seek, with assertions that cannot be masked by host all-notes-off.
- Longer real-time DAW load/automation and dropout measurements.
- Per-instance DAW pitch verification (separate from processor pitch tests).
- Physical MIDI/pedal, Windows and Intel Mac host runs: NOT RUN here.

The first milestone remains partially complete, not release acceptance.
