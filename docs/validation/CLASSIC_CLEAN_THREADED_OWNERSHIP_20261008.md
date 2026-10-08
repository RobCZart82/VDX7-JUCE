# D5 Classic Clean threaded ownership validation

Date: 2026-10-08. Baseline: `9ee7c9d6ff85381e853fb0fd7c2e7c13f78322d3`.
Branch: `test/classic-clean-threaded-ownership`.
PR: [#167](https://github.com/RobCZart82/VDX7-JUCE/pull/167).
Validated component/test revision: `56f0b47264ce9cdd094ae3960783d62b32dfed43`;
the following checkpoint commit adds only these PR references.
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

## Remaining gates

NOT RUN: actual processor ownership/epoch/mono-policy integration, production
pending-ROM completion, engine instruction-overshoot/SRC/lifecycle integration,
private original v1.8 Classic null-difference, live UI, REAPER or listening.
The owner does not manage ramp/active mode or prove click-free transitions.
No new release or corresponding-source artifact was produced or published.

Magyar összefoglaló: önálló komponens valódi zárolásos és szálas előkészítő
ellenőrzése, nem teljes processzor- vagy hangmotor-integráció. A kívánt mód,
pending projekt és mentési pillanatkép tulajdonlását teszteljük; a tényleges
pluginbekötés és kiadási/hostkapuk külön maradnak.
