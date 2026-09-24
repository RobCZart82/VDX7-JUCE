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

## Later consolidated run: integrated correction and expanded boundaries

Tested local commit `2fa75fcc0f2e48b2b4220bda58cb4e537881428e`;
GitHub equivalent `3309ef3322349ec59c05b64a62c1caa83779f4c4`.
Both have source tree `dcc55ecb43aa8ce39c46e8e9c1b4533a425fc4c8`.
Working tree was clean before rebuilding `vdx7_all_tests`. During/after this
run only Markdown documentation was changed; no source, test or build settings
changed. macOS arm64 Release, local verified v1.8 ROM, desktop access enabled
for the GUI test. All 29 registered tests ran without exclusions or inversion.

**28/29 PASS; CTest exit8; 371.82 seconds.** This supersedes the earlier 27/29
result for this newer source, without rewriting the historical evidence above.

| Acceptance area | Result |
| --- | --- |
| Corrected MONO: `vdx7_mono_corrected_processor` | PASS, 45.12s |
| Native MONO: `vdx7_mono_note_zero_acceptance` | FAIL, 0.14s; MIDI/held/MONO = 0/1/16 |
| Other regression groups | All 27 PASS |

Notable passing groups: `vdx7_processor` (GUI,9.11s),
`vdx7_deferred_partition` (10.76s), `vdx7_wheel_delivery` (9.89s),
`vdx7_portamento` (26.40s), plus profile, ROM-free, stability, timing, full-range,
stress, reset/history/ownership/lifecycle and the isolated candidate experiment.
The candidate and known-behaviour characterization remain distinct from product
acceptance; their success cannot substitute for the corrected processor group.

Coverage attribution:
- Ignored CC0/100/101 and unsupported-bank flood controls are in the deferred
  group, separate from the 24 wheel saturation fixtures.
- Held-Note0 snapshot/save/release/restore and fresh-note checks, including
  repeated-note and pedal histories, are in the corrected processor group.
- Public restoration after deferred input and inside the active callback's
  collection/lock gap are in the deferred group. Its negative-control failure
  was established in the prior targeted round, not rerun as a mutant here.
- The 24 native-POLY/corrected-MONO mixed-wheel saturation fixtures are in the
  wheel group. These are bounded scenarios, not arbitrary traffic proof.

No new failing group appeared. The complete suite is still FAIL, not “all green”.
Native behaviour stays visible and unchanged. Correction is implemented and
targeted/consolidated processor checks pass; real DAW/platform acceptance and
an explicit mode-specific publication decision remain outstanding. No Windows,
physical Intel Mac, sanitizer or live multi-instance DAW test was run here.
Public simultaneous restore requests and other scheduler interleavings remain
separate coverage questions. PR47 remains development work, not a release.
