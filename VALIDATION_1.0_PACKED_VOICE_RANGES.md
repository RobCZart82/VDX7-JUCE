# Packed voice range validation

## Audit disposition

**Implemented on `codex/packed-voice-validation-1.0` from current main
`af763f140c648418d9c95de40ea5a3c38dea149e`; public CI and merge are pending.**

The audit found that checksum-valid VMEM and VUB1 data could contain packed
parameter values outside the DX7 voice ranges. VCED import already validated
its unpacked values, but bank import/export only rejected invalid detune nibbles.

`VDX7VoiceData::hasValidPackedVoice` now checks the actual semantic fields:

- operator rates, levels, breakpoint, scaling depths, output level and fine
  frequency are 0–99;
- operator detune is -7…+7 (packed nibble 0…14);
- pitch-envelope rates/levels and LFO speed/delay/depth values are 0–99;
- LFO waveform is 0…5;
- transpose is 0…48 in packed VMEM (product range -24…+24).

Bit fields whose encodings already span their entire legal range are not
needlessly rejected. Reserved bits are preserved. Full packed-voice validation
is applied to VMEM decoding and encoding, live bulk-SysEx admission, the
processor's internal packed import boundary, and occupied USER-bank voices on
load and before save. Occupied USER patch names must be printable ASCII and
contain at least one non-space character. A failed USER-bank load leaves the
caller’s existing snapshot unchanged.

## Regression coverage

ROM-free tests build synthetic checksum-valid VMEM and CRC-valid VUB1 fixtures.
They verify rejection for every constrained operator and voice byte, invalid
detune, waveform and transpose values, invalid occupied USER names, output
snapshot preservation, and rejection before USER-bank write. No firmware is
needed or used.

## Verification record

- Base SHA: `af763f140c648418d9c95de40ea5a3c38dea149e`.
- Platform: macOS 26.7, Apple clang 21.0.0.
- Configure: `cmake -S . -B /private/tmp/VDX7-packed-validation-clean -DCMAKE_BUILD_TYPE=Debug` — PASS.
- Build: `cmake --build /private/tmp/VDX7-packed-validation-clean --target vdx7_ci_checks -j 4` — PASS.
- Tests: `ctest --test-dir /private/tmp/VDX7-packed-validation-clean --output-on-failure --no-tests=error` — 10/10 PASS after live-admission coverage was added.
- ROM-backed tests: NOT RUN; no ROM was used.
- GitHub Actions: NOT YET VERIFIED for this candidate branch.
