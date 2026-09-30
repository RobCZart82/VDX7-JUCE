# Test coverage and descriptive metadata — 2026-09-30

AUDIT-20260930-A8/A9/A10 are maintenance findings, not three demonstrated
audio defects. This change follows the pending-boundary and source-verifier
fixes; it does not reopen accepted GUI or keyboard PITCH policy.

- A8: the MSVC C4805 warning was observed in the local all-test build.
  Explicit integer conversion of the carry boolean preserves the status byte
  without mixing boolean and integer operands. Rebuild the host-reset test
  executable and check warnings; do not suppress the diagnostic.
- A9: `.github/workflows/sanitizers.yml` now builds `vdx7_resampling_tests`
  with the existing ASan/UBSan flags and selects `vdx7_resampling` in CTest.
  This exercises the production resampler with synthetic input, without ROM.
  Both additions are needed: a regex alone does not instrument/build a target.
- A10: the JUCE plugin DESCRIPTION replaces `prototype` with `instrument`.
  Product version, bundle ID, manufacturer/plugin codes and parameter IDs
  are unchanged; this is not a release-version bump.

PASS: changes inspected and whitespace check clean. Fresh Windows Release
host-reset/trace executable and its plugin shared-code dependencies rebuilt
without C4805 (no warning/error in the captured build log). PASS: 4/4 focused
CTest entries: version identity, resampling, private ROM profile and MONO trace
characterization (37.92 seconds). Remote Windows/macOS/sanitizer execution
is a separate required PR gate; follow its checks for the current outcome.
No claim of
bit-reproducible installers, host acceptance or full DSP correctness follows
from a sanitizer pass. No REAPER or installed plugin is used by this work.
