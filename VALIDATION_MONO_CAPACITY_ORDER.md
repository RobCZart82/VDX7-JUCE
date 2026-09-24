# Corrected processor: capacity and release ordering

2026-09-24. Test-only expansion through actual processBlock, public correction
selection, known private v1.8 image, 48 kHz/64 samples. No production change.

- Eight stacked-zero histories: 1/16/17/32 Note Ons, each with ordinary Note Off
  and velocity-zero Note On release encoding. Require actual MONO allocation
  count min(history,16), audible output, complete key/voice cleanup and silence,
  then genuine subsequent Note72 sound/ownership/release.
- Establish single-note firmware pitch targets for 0/60/72. Exercise all six
  attack permutations times six release permutations (36 histories), checking
  three actual allocations, the final single held note's key/target/audio, and
  final silence with every ownership counter clear. Both release encodings are
  exercised. No assumption of last-note priority for intermediate states.
- No reset, program/mode cycle or recovery between histories. Overload counter
  must remain unchanged: these are capacity tests, not input-FIFO overload.
- Existing persistence, 12 transition histories and 36 sample-rate/block pitch
  fixtures remain. New ordering cases inspect target and audible output; they
  do not add a new frequency measurement or portamento-transition oracle.

Broader same-block overflow, pedal/portamento combinations, nonzero-only
native/corrected differential controls and actual DAW acceptance remain open.
Native failing acceptance is unchanged. Keep PR47 Draft.

Validation: rebuilt host-reset executable; profile PASS0.67s, corrected group
PASS39.16s, native acceptance FAIL0/1/16 (0.15s). Combined 2/3, exit8,39.99s.
Afterward only the stale success-message wording was updated to reflect existing
UI/persistence integration. No full-suite or DAW rerun in this test-only round.
Prior-head macOS/Windows Actions both passed before uploading these changes.
