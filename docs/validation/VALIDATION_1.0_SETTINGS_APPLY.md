# Settings Apply failure ordering

## Reproduced behavior

The Settings dialog previously committed master tuning and MIDI channel before
requesting the MONO-correction change. A project restore can become pending
between those calls; `setMonoCorrectionFromUi` then correctly rejects the mode
change, but the earlier tuning request remains visible. The deterministic
regression exercised this order with a pending mismatched-ROM project restore
and confirmed the partial tuning change.

## Change

The Settings dialog now routes all three values through
`applySettingsFromUi`. It validates the values and tuning availability first,
then performs the only expected-to-fail operation (MONO correction) before
committing the valid tuning and channel selections. A pending project restore
therefore rejects the Apply without changing any of the three settings. The
existing message-thread stale-dialog check and the settings values/layout are
unchanged.

## Verification — 2026-09-26

- `vdx7_stability_tests <owner-supplied-v1.8-dx7.bin>`: PASS; first confirms
  the former order leaves tuning changed after MONO rejection, then verifies
  the new Apply path leaves tuning, channel and MONO request unchanged.
- Rebuilt `vdx7_ci_checks`: PASS.
- ROM-backed CTest excluding `vdx7_processor`: **35/35 PASS**. The excluded
  processor test's SAVE AS integration path requires a desktop/display; that
  unrelated host-GUI check remains outstanding.
- `git diff --check`: PASS. Windows/macOS CI for this Settings change remains
  pending.

The private ROM fixture remained outside the repository and CI artifacts.
