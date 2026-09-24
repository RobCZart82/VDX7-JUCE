# Consolidated regression — 2026-09-24

Tested local commit905cae5f6eded984bacdb450653f6af06b23beab, remote equivalent
c98b959d2cd2ef10e0689250048fc3d78bb000a2, identical source tree
ee502ff6a9dfa3530d06f9db275761cc8cf627f4. Subsequent changes in this round are
documentation only. macOS arm64 Release, explicit private verified v1.8 ROM.

Rebuilt `vdx7_all_tests`, then ran the entire registered CTest suite without
exclusions, expected-failure inversion or changed assertions.

**27/29 PASS, CTest exit8, 359.60 seconds.**

- `vdx7_processor` FAIL at SAVE AS GUI desktop/display prerequisite. Not PASS,
  not a completed end-to-end GUI test; later assertions may not execute.
- `vdx7_mono_note_zero_acceptance` FAIL, unchanged native MIDI/held/MONO0/1/16.
  The opt-in corrected path does not turn native firmware acceptance green.
- All other27 groups PASS, including stability, full MIDI range, timing,
  stress, reset/history/overlap/overload, staged install/bypass/deferred-input,
  portamento, wheel delivery, experiment and corrected processor45.39s.

No additional failing group appeared in this run. This is not proof of no
remaining bugs: live scheduler interleavings, suspended-callback DAW bypass,
mixed controller saturation, full GUI and real hosts still need acceptance.

GitHub status sampled during this round for the remote equivalent: macOS run
35965806494 succeeded; Windows35965806549 still in progress at last check.
These are CI compile/ROM-free checks, not private-ROM acceptance.

Next: obtain an actual desktop session for complete GUI checks; expand the
documented host/concurrency/controller acceptance matrix. Keep PR47 Draft.
No main merge, installed binary replacement, ROM upload, tag or release.
