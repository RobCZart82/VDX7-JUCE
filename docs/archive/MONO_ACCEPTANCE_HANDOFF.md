# MONO Note 0 acceptance handoff — 2026-09-24

## Version provenance

Read-only GitHub check at the start of this follow-up:
- main: `98d61b8bf30b56813a31ce9241a8e0da324d973f`
- open, unmerged PR #47: `013a98cf8634dae3b809287c47dc02c149cbb2e5`
- local source: `013d72ae7a92df1d94f2d3d2ea88499d09e9a769`
- shared source tree: `a4cec1cd35dfc4fd1d0948e6819900cc70c7b57d`

Earlier full regression belongs to local
`e43c9b8eb5d9dcddf560c0683efef8a4defec66a`, equivalent remote
`c8d413c666ba7fcabd4a9e46435c06cb80e45ccd`, not automatically to a new
production revision. Subsequent changes were documentation and the Python
recording validator. See docs/validation/VALIDATION_CONSOLIDATED_2026-09-24.md for the
27/29 original run, separate desktop retry and retained native FAIL.

## Coverage review, not new bug reports

- Held Note 0 snapshots: existing corrected processor fixtures cover running
  instance restore, fresh instance and deferred missing-ROM loading.
- Mode persistence and two-instance isolation: existing targeted coverage;
  do not reopen integration as missing functionality.
- Public restore overlapping host and collected keyboard input: deterministic
  pre/post-lock schedule and negative control documented in
  VALIDATION_STATE_INSTALL_BOUNDARY.md. Multiple simultaneous public restores
  and unrestricted scheduler interleavings remain unproven.
- Actual REAPER transport stop/start, seek and loop combinations with pedals:
  NOT RUN in this follow-up. Add measured host cases, not merely playhead
  changes in a harness that the processor never observes.
- Per-instance pitch/release stems, sustained load and automation: NOT RUN.
- Physical controller/pedal, Windows and Intel host: NOT RUN.

## Fresh targeted verification

Build command: `cmake --build ../build-1.0 --target vdx7_host_reset_tests -j4`.
Result: no work to do, exit 0.
Test executable SHA256:
`9de7f7b4f6e762c4e95ceb0c2f3489cc541ae7fc2670d8bb19aba2fdbef04034`.
Test command: `ctest --test-dir ../build-1.0 -R '^vdx7_mono_corrected_processor$' --output-on-failure`.
Includes the required firmware-profile fixture; local firmware is not distributed.
Result: 2/2 PASS, exit 0, 51.97 seconds (corrected group 51.94 seconds).
This is one targeted run, not a full-suite rerun. Native acceptance was NOT RUN
in this follow-up and retains its previously observed FAIL status.
Local CTest log: `../build-1.0/Testing/Temporary/LastTest.log` (overwritten by
later CTest runs; the result and binary hash are recorded here).

## Publication boundary

Keep native known failure and corrected acceptance separate. No installed
plugin replacement, main merge, tag or release is authorized by this report.
Accepting the native limitation for publication remains a user product decision.
