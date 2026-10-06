# ROM diagnostic wiring validation

2026-10-06 baseline: `08aae7cae05d4a185d972a26d51b2023df6e0ab8`.

## Changes

The processor passes a `RomLoadDiagnostic` output to the engine and formats the
actual failure category. Input/factory-data/firmware-rejection/boot-failure
messages are distinct; factory failure retains the shared bank/voice/field detail.
Formatting is non-RT and contains no firmware bytes or file paths. Admission
rules and DSP/GUI/parameter identities are unchanged.

The sanitizer workflow builds and selects `vdx7_rom_diagnostics`, closing its
previous exclusion from that workflow. It remains ROM-free in public CI.

## Local results

- PASS: freshly compiled direct engine diagnostic test with Apple clang C++20,
  ASan and UBSan, including freshly compiled HD6303R/HD6303R_inst/dx7 core sources
  from the existing pinned corresponding-source dependency copy. No private ROM argument.
- PASS: freshly compiled voice-data/SysEx test with ASan/UBSan, including the
  32,768 single-byte admission-parity controls.
- PASS: Python unittest discovery in `Tests` ran 77 tests successfully. These
  packaging/release fixture checks are not acceptance of new release assets.
- Formatter assertions cover success, invalid input, detailed factory failure,
  firmwareRejected and bootFailed. The last two are synthetic result categories,
  not reproduction of real firmware rejection/boot failure.
- Leak detection disabled, matching the macOS sanitizer workflow; no leak-test claim.

These local component runs are not a complete plugin build, processor/GUI test,
private-ROM full suite or real REAPER acceptance. Windows/macOS/plugin compilation
and hosted sanitizer execution of this change remain the PR verification gate.
The earlier baseline main platform runs passed 15/15 ROM-free CTests; that is
prior evidence, not PASS for this modified source.

No installed plugin, REAPER project, public release asset or firmware file was modified.
The development plan retains D1 as in progress until remaining gates are resolved.
