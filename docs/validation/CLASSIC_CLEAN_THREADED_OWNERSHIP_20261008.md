# D5 Classic Clean threaded ownership validation

Date: 2026-10-08. Baseline: `9ee7c9d6ff85381e853fb0fd7c2e7c13f78322d3`.
Branch: `test/classic-clean-threaded-ownership`.
PR: [#167](https://github.com/RobCZart82/VDX7-JUCE/pull/167).
Initial locally validated component/test revision: `56f0b47264ce9cdd094ae3960783d62b32dfed43`;
the next checkpoint adds only PR references. The subsequent compatibility fix
and its separate validation are recorded below.
The [active development plan](../development/DEVELOPMENT_PLAN.md) and
[state contract](../development/CLASSIC_CLEAN_STATE_CONTRACT.md) define the scope.

## Scope

The detached `Source/VDX7SoundModeOwner.h` component uses a caller-supplied
engine/payload mutex, not a new internal mutex or an atomic mailbox. It is not
connected to PluginProcessor, SETTINGS, firmware, native rendering or SRC.
The plugin still neither accepts nor writes live Clean metadata through this
component. No parameter, plugin ID, JUCE/Retromulator pin, release asset or
installed plugin changed.

The fixture uses its own small synthetic ValueTree, not valid firmware, Yamaha
patches or a complete semantically valid VDX7 project. Full project validation
and ROM compatibility are external prerequisites to the owner APIs. Tests of
those prerequisites are not replaced by the fixture's tiny admission rule.

## Tests

`Tests/VDX7SoundModeOwnershipTests.cpp` uses actual threads/mutexes and explicit
promise rendezvous, with a five-second watchdog and no sleeps:

- Invalid scalar recall preserves the entire fixture snapshot; legacy recall
  selects Classic; identical recalls retire old tokens; revision exhaustion
  rejects without committing payload.
- UI/audio try-locks finish while another thread holds the mutex, without
  notification, dispatch or delayed replay. Separate instances remain isolated.
- A recall scheduled between a pre-lock snapshot and audio ownership forces
  the audio visitor to observe the new project's current mode and payload.
  The visitor retains the payload lock throughout its callback.
- Old UI tokens and old compatible completions cannot mutate a newer pending
  project. A fresh pending edit persists through compatible completion; mismatched
  completion and audio dispatch onto an unrelated engine are rejected.
- A capture-before-recall, encode-after-recall actual JUCE binary round trip
  preserves coherent old payload plus the latest accepted desired mode at capture, without
  mutating the new live project or serializing runtime revision/ready/gain fields.
- Host notification executes outside the mutex. Same-thread reentrant save
  observes the accepted mode; reentrant recall is not overwritten on callback return.

Two opt-in broken adapters are required to fail with exit code 1:
`--pre-lock-audio-negative-control` and `--notify-under-lock-negative-control`.
They bypass correct caller behavior only in the test, not in the component.

## Results and reproduction

PASS: 78 Python tests; registration checker/self-tests; actual ROM-free CTest
inventory of 20 tests, including `vdx7_sound_mode_ownership` with timeout/labels.
PASS: full `vdx7_ci_checks` build and 20/20 ROM-free CTests under ASan/UBSan;
the new ownership test also passed 50 consecutive repetitions. Both broken
adapters failed immediately with exit code 1 at the intended invariants:
pre-lock audio cannot use the retired project's mode, and host notification
must run outside engine/payload ownership. No rendezvous watchdog fired.
PASS: `git diff --check` and 138 local file targets in the four changed documents.

Configured on macOS ARM64 with AppleClang 21.0.0.21000334, RelWithDebInfo,
`VDX7_ENABLE_ROM_TESTS=OFF`, C and C++ ASan/UBSan instrumentation. Resolved clean
JUCE checkout: `e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`; Retromulator:
`d5473776a0449d60a997b91bdc888598a33265ac` (both unchanged).

```sh
cmake --build build-owner-sanitized --target vdx7_ci_checks -j3
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-owner-sanitized -L rom-free --output-on-failure --no-tests=error
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-owner-sanitized -R '^vdx7_sound_mode_ownership$' --repeat until-fail:50 --output-on-failure
```

ASan/UBSan do not constitute ThreadSanitizer coverage. No leak-detection or
real-time allocation/CPU guarantee is claimed. Final-head GitHub Windows/macOS/
sanitizer CI and review remain separate merge gates.

## Xcode compatibility correction

The first final-head macOS build (37837520325) and ASan/UBSan build
(37837520353) failed during compilation, before test execution. Both used
Xcode 15.4 and reported no `std::jthread` in `std`; Windows (37837520328) passed.
The initial local AppleClang 21 build did not expose this toolchain mismatch.

The test now uses a small noncopyable `JoiningThread` around `std::thread`.
Scope exit joins a still-joinable worker; explicit joins, rendezvous, watchdogs,
negative controls and all ownership assertions remain. No stop token was used
by the original fixture. Additional lifetime probes verify joining on normal
scope exit and exception unwinding before captured state is destroyed.
No owner component, plugin code, toolchain pin or workflow gate was changed.

After the correction, the local full `vdx7_ci_checks` build passed, followed by
20/20 ROM-free CTests under ASan/UBSan, 50 consecutive ownership repetitions,
both negative controls with the intended exit 1, 78 Python tests and registration
self-tests. These results use the same local AppleClang 21 configuration above;
the corrected final-head GitHub Xcode 15.4 build remains a required independent
confirmation before merge. This correction does not turn the earlier failed
CI runs into PASS evidence.

## ROM admission race correction — 2026-10-09

Fix starts from PR #167 head `2ebe4bed1cea3cc03b043cc24c68e3b190bb12cf`,
current main `9ee7c9d6ff85381e853fb0fd7c2e7c13f78322d3`. Local branch
`fix/pr167-rom-admission`; update the existing PR branch, without force push.
The old head passed Windows `37839598022`, macOS `37839598025` and sanitizer
`37839598014`, but the unresolved review exposed a real component admission gap.
Old green CI is not validation of this correction.

### Reproduction before the fix

New real-thread rendezvous tests were built against the unchanged owner API.
One thread computes a compatible identity, another changes the simulated ROM
under the shared mutex, without advancing the project revision, then the first
attempts completion/install with its cached result. Both reproduce deterministically:

- `--rom-completion-repro-only`: FAIL / exit 1,
  `ROM reload without project recall must reject stale compatible completion`.
- `--rom-install-repro-only`: FAIL / exit 1,
  `ROM reload before project install must keep validated project pending, not falsely ready`.

No genuine firmware is loaded. The mutable identity is a synthetic integer,
with all reads/writes under the shared mutex. This is not a released plugin bug.

### Correction and regression coverage

`installValidated(desired, checkReady, commit)` and
`completePending(revision, checkCompatible, commit)` now require nonthrowing
predicates, evaluated under the owner lock before payload commit, with no unlock
between admission and commit. Compile-time probes reject plain bools and throwing
queries. Caller must read current protected identity, not wrap stale results in
a lambda; immutable synthetic fixture constants are not real ROM admission.
Read-only checks must be bounded: no file/hash/host notification/reentry; payload
commit may not invalidate the checked ROM compatibility. All mutable identity
writers must use the same mutex.

Stale and non-pending completion, invalid enum and revision exhaustion invoke
neither check nor commit. Current mismatch invokes check but not completion
commit and preserves the full snapshot. A valid project with a now mismatched
ROM still installs as pending, preserving its desired mode, not falsely ready.
Additional two-thread probes verify both predicate and commit retain the same
ownership, blocking an identity-changing contender. Matching current admission
remains a positive control, completion occurs only once, and latest desired
edits remain preserved by the original regressions.

### Local results and remaining gates

Windows x64 / MSVC 17.14.60 / Release, existing ROM-free build at
`C:/Users/gyuriczar/Documents/Codex/build-imported-ui-20261008`.

| Check | Result |
|---|---|
| Both original repro-only commands after correction | PASS / exit 0 |
| Complete ownership executable | PASS |
| Ownership CTest repeated until-fail:50 | PASS, 50 consecutive runs |
| New cached-ROM completion/install adapters | PASS control: intended FAIL / exit 1 at the original messages |
| Existing pre-lock audio / under-lock notification adapters | PASS control: intended FAIL / exit 1 |
| Full Windows CI-target build and ROM-free CTest | PASS, 20/20, 0 FAIL |
| Python | PASS with coverage gap: 78 run, 77 PASS / 1 Windows symlink-permission SKIP, 0 FAIL/ERROR |
| Inventory / registration self-test / document-link / diff checks | PASS, actual 20-test inventory and 23 local document-link targets |
| New final-head GitHub Windows/macOS/ASan/UBSan and review | NOT RUN yet; required before merge |
| Local ASan/UBSan/TSan, production processor/DSP/real v1.8/REAPER | NOT RUN in this correction |

Rebuild `vdx7_sound_mode_ownership_tests`, run the two repro-only commands and
the normal executable. Then run `vdx7_ci_checks`, full `ctest -L rom-free`,
and ownership `--repeat until-fail:50`. The four opt-in negative controls must
each exit 1, not be mistaken for failing normal CTest runs:

```text
--cached-rom-completion-negative-control
--cached-rom-install-negative-control
--pre-lock-audio-negative-control
--notify-under-lock-negative-control
```

The new check/commit boundary does not implement firmware validation, later
ready-state invalidation when a real processor reloads ROM, or epoch/mono/SRC
lifecycle integration. Those remain production-adapter gates. The component is
still not wired to PluginProcessor/SETTINGS/DSP. No dependency pin, real audio,
Yamaha data, release/tag/asset or installed application changed.

## Remaining production gates (unchanged)

NOT RUN: actual processor ownership/epoch/mono-policy integration, production
pending-ROM completion, engine instruction-overshoot/SRC/lifecycle integration,
private original v1.8 Classic null-difference, live UI, REAPER or listening.
The owner does not manage ramp/active mode or prove click-free transitions.
No new release or corresponding-source artifact was produced or published.

Magyar összefoglaló: önálló komponens valódi zárolásos és szálas előkészítő
ellenőrzése, nem teljes processzor- vagy hangmotor-integráció. A kívánt mód,
pending projekt és mentési pillanatkép tulajdonlását teszteljük; a tényleges
pluginbekötés és kiadási/hostkapuk külön maradnak.
