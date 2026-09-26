# MONO correction SETTINGS integration

2026-09-24. Adds an explicit native/corrected choice, native default, project
persistence and current active/waiting/unsupported status on dialog opening.
The dialog warns that changing policy restarts the engine and stops notes.
No automation parameter, silent default change or installed binary replacement.

Non-RT UI change holds engine ownership, commits pending sound edits, preserves
the working packed bank/selection and explicit tuning/performance settings,
reboots/revalidates the image and restores only persistent data. It invalidates
old MIDI timeline events and notifies the host after unlocking. Missing-ROM
selection updates pending project intent. Same-value requests are no-ops.

The first test caught a genuine implementation error: restoring the entire
pre-transition RAM also restored held Note 0 ownership when returning to native.
That attempt failed `corrected lifecycle retained ownership`. Fixed by retaining
the newly booted runtime state and restoring only the packed bank and supported
persistent settings. No ownership byte or incoming pitch patch is used.

New independent transition fixture holds corrected zero, disables correction,
checks native status/empty ownership/silence and unchanged settings, enables
again, then verifies fresh zero and 72 sound/release. Existing persistence,
three lifecycle fixtures and 36 rate/block pitch cases remain unchanged.
GUI test checks the two-item native-default choice; rendered SETTINGS screenshot
was inspected for readable labels/warning and nonoverlapping controls.

Initial fixed targeted run: profile and corrected suite PASS23.41s; unchanged
native acceptance FAIL0/1/16; 2/3, exit8,24.21s. Desktop processor tests with
screenshot output PASS. Final host-notification follow-up results recorded below.
Final rebuilt targeted desktop run: GUI/processor PASS8.58s, profile PASS0.52s,
corrected processor PASS23.42s; unchanged native acceptance FAIL0/1/16 (0.15s).
Combined 3/4, exit8,32.67s. Both previous-head platform checks succeeded.
No complete-suite rerun in this GUI/API round; prior persistence round contains
the rebuilt full-suite evidence. GUI callback itself is not end-to-end automated;
the visible control and public setter are tested separately.

## Settings recommendation (2026-09-24)

Product decision: Native firmware remains the default and recommended mode for
ordinary use. Keep Correct MONO Note 0 selectable as an advanced, under-the-hood
compatibility setting, not a control users are expected to toggle routinely.
The product Note 12–120 MIDI range is common to both modes; the option does not
make the excluded Note 0–11 octave available. The GUI helper text and labels now
state this explicitly.

At the time this validation round was recorded, PR #47 remained Draft pending
broader pedal/overload/transition and real-host acceptance. PR #47 has since
merged; those outstanding checks remain release work. Unsupported-image status
and pending-ROM selection still need additional GUI interaction coverage. This
is not a claim that native Note 0 now passes.

## Follow-up: stacked zero and pedal lifecycle

The four independent lifecycle cases (reset, release/prepare, project restore,
UI native/corrected round trip) now each run three histories: one held zero,
16 stacked zero Ons, and a pedal-held zero released with velocity-zero Note On.
The stacked case requires 16 actual firmware allocations before transition.
All cases retain empty ownership/silence, unchanged persistent settings and
fresh zero/72 playback/release assertions after the transition.

An initial test precondition incorrectly required a sustained-slot bit in MONO.
The actual measurement was MIDI/held/sustained=0/0/0, MONO=0 while audio remained
~0.04218. The isolated candidate already distinguishes MONO sustain via audio.
The corrected test requires empty released-key ownership plus audible sound,
then releases CC64 and requires silence/empty state, and recreates the pedal
hold before the transition. This is a test-oracle correction, not a production
bug fix or removal of the native failing acceptance gate.

This remains 48 kHz/64-sample transition coverage, not a full overload or
cross-platform ROM runtime result. No production source change in this round.
Rebuilt host-reset executable; final focused CTest profile PASS0.66s, corrected
group PASS32.67s (12 lifecycle histories plus existing persistence/pitch matrix),
native acceptance FAIL0/1/16 (0.15s). Combined 2/3, exit8,33.49s.
No full-suite rerun. Prior-head CI was still running at the start of the round.
