# Merged-main local ROM suite — `d696e56`

## Source and environment

- Source commit: `d696e56b8b69521f8b3ba879f8325c044b410b70` (PR #79 merge).
- The local checkout's source tree matched this merged-main tree exactly.
- Host: macOS 26.7, Apple clang 21.0.0 (`clang-2100.3.34.2`), CMake 4.4.3.
- Fixture: owner-supplied Retromulator DX7 v1.8 package, used locally. The ROM
  and factory data remained outside the repository and uploaded artifacts.

## Commands

```sh
cmake --build build-local --target vdx7_ci_checks -j2
ctest --test-dir build-local --output-on-failure --no-tests=error \
  -E '^vdx7_processor$'
```

## Result

- Build target `vdx7_ci_checks`: PASS.
- CTest: **35/35 PASS**, including 25 opt-in local-ROM tests and all 10
  ROM-free tests. This includes GUI snapshots at each supported fixed size,
  lifecycle/reset/reactivation, MIDI-range/timing/stability/stress, state/ROM
  identity, direct ROM reload, deferred MIDI, portamento, wheel delivery,
  corrected MONO behavior and soak coverage.
- `vdx7_processor` was excluded from the 35-test run, then attempted separately.
  It stopped at its explicit primary-display precondition (`SAVE AS GUI test
  requires desktop/display access`) in the command-runner environment, before
  opening the dialog or running the remaining GUI assertions. Classify that
  attempt as **environment-blocked / NOT RUN**, not a product-test failure or
  PASS.
- `vdx7_processor` remains an open GUI-capable-host acceptance item. This test
  run is not a complete release sign-off and does not replace exact-candidate
  REAPER/platform acceptance.
