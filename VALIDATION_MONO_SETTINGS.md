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

Keep Draft/P1 open pending broader pedal/overload/transition and real-host
acceptance. Unsupported-image status and pending-ROM selection need additional
GUI interaction coverage. This is not a claim that native Note 0 now passes.
