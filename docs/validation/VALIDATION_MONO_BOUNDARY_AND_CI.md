# MONO boundary characterization and CI coverage

2026-09-23. Follow-up on Draft #47, baseline local `44aa803`, remote
`e199cb9c345cce2445cfa9fa95f06ebadb336ded`, identical source tree
`0ca1b5dfeff7ffc1eff5ddb5b19fae41b539aee1`.
No production engine, firmware, parameter, GUI or installed-plugin change.
Keep #47 Draft: this round does not close P1 or authorize release/merge.

Subsequent evidence: docs/validation/VALIDATION_MONO_INSTRUCTION_TRACE.md records the actual
loaded-image branch execution and subsequent-note failures without recovery.
The results below describe the earlier boundary/recovery round, not that later
test's scope. The original failure still has no production fix.

## Native MONO pitch-zero edge: investigated, NOT fixed

The original `--mono-note-zero-only` diagnostic is preserved. It expects all
ownership to disappear after releasing sixteen repeated pitch-zero notes,
without reset or lifecycle cleanup. It remains a known failing acceptance case,
outside the passing registered CTest set. Do not interpret a green build or the
new characterization test as this diagnostic passing.

`--mono-boundary-only` / `vdx7_mono_boundary_characterization` exercises 32
combinations: POLY/MONO, MIDI pitch 0/1/60/127, 1/16 repeated notes and ordinary
Note Off/velocity-zero Note On. Each case creates both a fresh processor and
a separate raw pinned dx7Lib emulator running the same explicit local v1.8 ROM.
The raw runner does not use VDX7Engine, processor MIDI bookkeeping, reset,
release retirement or resampling. All input stages must drain before inspection.

Both paths are checked against explicit expected ownership, not only against
each other. The processor uses the existing audible fixture; the raw runner
uses its loaded program. This is ownership characterization, not a claim of
bit-identical audio or identical voice patches between those fixtures.

| Cases after matching releases | MIDI entries | Held entries | MONO active count |
|---|---:|---:|---:|
| MONO pitch 0, one repeat, either Off encoding | 0 | 1 | 1 |
| MONO pitch 0, sixteen repeats, either Off encoding | 0 | 1 | 16 |
| MONO pitches 1/60/127 and all POLY controls | 0 | 0 | 0 |

No sustained entries remain in these pedal-off fixtures. Raw core and processor
agree in all 32 combinations. The four failing-release MONO-zero combinations
are expected observations in the characterization test, explicitly labelled
`known-firmware-characterization`; they have not become successful releases.

An explicit native POLY-to-MONO mode cycle clears the residual ownership in
both paths. The processor then must sound one fresh pitch-72 voice, release it
to silence and empty ownership, preserving persistent settings. The same fresh
note/release control is checked in the other 28 cases without a recovery cycle.
This proves a tested explicit recovery route, NOT an automatic workaround:
ordinary plugin playback does not change modes behind the user's back, and no
firmware RAM/counter is patched to suppress the finding.

The result agrees with the annotated native MONO add/remove routines' zero-key
sentinel behavior. It is evidence from the pinned emulator plus the original
local firmware, not measurement on physical Yamaha hardware. Keep the native
behavior/product-policy decision open; broader MONO/reset/sustain combinations
and actual-host acceptance are not covered by this boundary matrix.

## T1: ROM-free CI compilation gap

The shared `vdx7_ci_checks` target previously omitted four EXCLUDE_FROM_ALL
executables: processor, stability, MIDI-range and timing. It now depends on
all six integration runners, including stress and host reset. `vdx7_all_tests`
uses the same aggregate, so the two cannot drift at this level. Existing macOS,
Windows and candidate workflows already build `vdx7_ci_checks`.

This adds compile coverage only. Runtime registration remains opt-in; default
CTest contains exactly five ROM-free tests. No ROM, ROM path or generated
firmware fixture is added to public CI/artifacts.

## T2: explicit known-image runtime prerequisite

The host-reset binary's ownership assertions inspect the validated v1.8 map.
All nine groups now carry `local-rom;firmware-v1_8` labels and require the
`vdx7_v18_profile` fixture. That fixture verifies a 16384-byte firmware-only or
49152-byte combined file, with first-16384-byte FNV1a64 `20dd25e47a496ba0`.
It also checks a firmware-only copy and rejects an in-memory bit change and
truncation. These bytes are never written or uploaded. The checksum identifies
the compatibility profile; it is not a cryptographic security boundary.

Every direct invocation of the host-reset runner also enforces the preflight.
A wrong fixture is exit 1 with an explicit image-specific explanation, not exit
77/SKIP or a false generic-ROM failure. CTest does not run dependent groups when
the prerequisite fails. This does not validate another ROM or change production
support: unknown images retain the existing conservative fallback.

## Validation record

Local macOS arm64 Release, AppleClang 21; pinned dependencies from CMake.
Private combined ROM's firmware SHA256:
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.

- Targeted profile + 32-case matrix: PASS, 11.40 s before adding rejection controls.
- Fresh ROM-OFF configure/build of `vdx7_ci_checks`: PASS, all 11 executables
  compiled/linked (five ROM-free plus six integration runners). Initial JUCE
  helper configure lacked its SDK path; explicit local SDK/C++ include settings
  resolved this without changing repository build requirements or dependencies.
- Full rebuilt registered local suite: **20/20 PASS in 239.83 s**. This includes
  the explicit profile/rejection controls and the 32-case characterization
  (10.62 s), not the separate failing acceptance diagnostic below.
- Fresh ROM-OFF CTest: **5/5 PASS in 3.05 s**; cache confirms the ROM path is
  empty and local-ROM test registration is OFF.
- Wrong-fixture CLI preflight: exit 1 with expected explanation.
- Original `--mono-note-zero-only`: **still FAIL, exit 1**, MIDI/held/MONO
  counts 0/1/16. Assertion and failure expectation were not weakened.

No VST3 wrapper/installed bundle was rebuilt or replaced in this test-only round.
The shared plugin code is compiled for the test runners. No notarization,
physical hardware, Windows ROM runtime, DAW session, allocation-completeness or
worst-case realtime claim is made. Callback probes cover ordinary C++ allocation
on the observed thread and finite output, not every allocator.

Next retained stabilization work: Q1 processor-level block-partition regression,
then the deferred-timeline fix if reproduced; Q2 publication races and the other
items in docs/archive/AUDIT_TRIAGE_2026-09-23.md remain tracked. GUI/Retina finishing, actual
host acceptance and exact-source release-candidate checks still follow.
