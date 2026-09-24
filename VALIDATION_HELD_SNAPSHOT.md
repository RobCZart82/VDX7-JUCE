# Held-note project snapshot regression — 2026-09-24

Audit A2 is now reproduced through the actual VDX7AudioProcessor, not only a
synthetic RAM projection. Production code is unchanged in this round.

## Reproduction

The corrected-processor lifecycle suite now includes a fifth transition:
save while Note 0 is held; release the note and pedal; verify empty ownership
and silence; restore the held snapshot without an extra reset; verify ownership,
silence, persistent settings and subsequent Note 0/72 playback and release.
The matrix includes one note, sixteen repeated notes and pedal-held sound.

Built `vdx7_host_reset_tests` successfully and ran
`--mono-corrected-processor-only` with the private verified v1.8 ROM.
The first new case failed with MIDI/held/MONO counts **1/1/0** after restore.
Exit status: **1**, `corrected lifecycle retained ownership`.
The two later new histories were not reached because the suite stops on failure.
The existing four lifecycle transitions completed before this failure. This is
not a full-suite result or a claim that every new history has been reproduced.

## Interpretation and next step

The inconsistent restored ownership is confirmed. Audible stuck-note duration
and subsequent-note behaviour in this failing case are not yet measured because
the ownership assertion stops execution first.

Implement a project-specific restoration boundary separating persistent voice,
bank and performance data from transient firmware/adapter voice ownership.
Do not blindly clear guessed RAM addresses or globally alter `restoreRam`, which
also serves editing/import paths. Preserve legacy project and missing-ROM
behaviour; test fresh-instance restore as well as an already running instance.
Keep this regression failing until the production path genuinely satisfies it.
No ROM data, installed plugin or release asset was modified.

## Implemented follow-up

`restoreProjectRam` now reboots the verified v1.8 image and restores the packed
bank plus explicit tuning, play, bend and controller settings. Project selection,
dirty flags, deferred edits and host parameters remain processor responsibilities.
Transient ownership, pedals and queues come from boot. General `restoreRam`
(editing/import) is unchanged. Unknown firmware retains the legacy restoration
path: this fix does not claim validated support for its memory layout.

All three new held-snapshot histories now pass with counts **0/0/0**, silence,
unchanged captured persistent settings, and subsequent Note 0/72 sound/release.
Persistence fixtures now save a genuinely held zero and check empty ownership
and silence in a fresh instance, including missing-ROM deferred loading.

Final targeted CTest run: corrected processor group PASS (45.03 s), profile PASS;
native Note 0 acceptance remains FAIL (known native behaviour). Processor GUI
test FAIL at desktop/display access prerequisite, including the desktop retry;
not counted as passed. Overall targeted run 2/4, exit 8, 49.28 s. No full suite
or DAW validation in this round. State-install concurrency is still a separate
open issue; these sequential results do not close it.
