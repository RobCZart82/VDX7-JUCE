# Q2: preserve UI edits against stale PERFORMANCE/tuning publication

2026-09-23, Draft #47. Baseline local `70b68c7c20be63e8c90c38de50012a25869285fd`,
remote `8084077ca8aa50767b76a3beb269c8ca068396f7`, identical tree
`124ae3d1d611f667265399d897282488732cf734`. Both baseline platform Actions passed
before this round was uploaded. No MONO compatibility workaround is included.

## Reproduce before fixing

The actual processor's frame capture/publication was factored through a test
seam, initially retaining its unconditional atomic store. Pause after real
engine/pending-value capture, call the real public UI setter, then finish the
publication. Mod-wheel range requested 99 but the display returned the captured
old 0; `--publication-only` failed with
`Publication field=0, scenario=0, expected=99, actual=0`.

This is not an independent model or comparison between two snapshot getters.
The test drives the real capture code and public setters, checks explicit
expected numbers immediately, and later inspects the engine's actual settings.
The pending request was not proven permanently lost: the defect is stale display
publication until the next refresh/application. Tuning shared the same pattern.

## Correction and concurrency contract

`VDX7LatestDisplay` keeps one atomic packed frame. Bit 63 is an internal
"UI edited since this publication began" latch, masked out of reads. It is
not a small wrapping revision counter. PERFORMANCE retains its 58-bit encoding;
tuning uses a separate nine-bit offset encoding for -256..255, not saved-state
or host-parameter format changes.

The sole engine publisher, serialized by the existing `engineMutex_`, clears
the latch BEFORE capturing engine and pending UI values. UI field updates merge
into the same atomic frame and set the latch even for a same-value request.
The publisher then makes ONE strong compare/exchange attempt. If any UI edit
intervened, it skips its stale frame; no spin/retry, new lock or allocation on
the audio publication path. Subsequent ordinary publication refreshes engine
fields after UI activity settles. Continuous edits may postpone engine-only
display refresh; no real-time deadline is promised.

The acquire/release boundary also matters: tuning now publishes its pending
value/dirty bit before its display edit, matching the other coalesced setters.
Thus a publisher observing that completed UI edit also observes its request.
Readers still perform one atomic frame read and never acquire the engine lock.
UI field merging retains a CAS retry loop (as before for PERFORMANCE); this
does not claim all operations are wait-free or independently prove WCET.

The helper requires one engine publisher at a time. Processor publication sites
remain inside the existing engine-owned callback/transaction paths. The test
hook is a template-only seam: no stored callback, production test flag or
blocking branch was added.

## Regression coverage

- 88 deterministic real-processor interleavings: 22 coalesced fields x four
  schedules (new request, replacement of older pending request, ABA return to
  the same displayed value, and same-value request over a changed engine image).
  The immediate display assertion occurs BEFORE any repairing audio callback.
  Explicit engine and display values are also checked after firmware settling.
- New ROM-free `vdx7_latest_display`: edits during/after capture, 1/2/64/128/1024
  changes, same-value/ABA schedules, unrelated-field preservation, tuning bounds,
  100 synchronized two-thread schedules and concurrent disjoint-field writers.
- Existing engine-lock-held read/write tests, 1,000-write/audio overlap, explicit
  engine-value checks, state restore, firmware mode refresh, callback ordinary
  C++ allocation probe, Q1 and the reset/ownership regressions remain enabled.

`vdx7_stress_tests /absolute/path/to/private/dx7.bin --publication-only` runs the
88 focused cases; the ordinary local `vdx7_stress` test includes them too.
Public ROM-free CI now executes six tests and compiles all six integration
runners. It does not contain or execute the private ROM.

## Results

- Focused 88 processor interleavings: PASS.
- ROM-ON `vdx7_all_tests` and ROM-OFF `vdx7_ci_checks`: rebuilt successfully.
- ROM-free runtime: 6/6 PASS (1.54 s).
- Display helper with AddressSanitizer/UndefinedBehaviorSanitizer: PASS,
  no reported errors (component-only, not full-plugin coverage).
- Full local registered suite with desktop access: **23/23 PASS (256.47 s)**,
  including the graphical processor test and existing 1,000-write stress case.
  Two labelled MONO characterization groups assert the known defect, not repair.
- Standalone display helper under ThreadSanitizer: PASS, no reported race.
  Component-only; neither sanitizer run constitutes full-plugin race coverage.
- Local arm64 VST3 built; manual ad-hoc signing after the existing Xcode-license
  helper warning and strict signature verification passed. Installed plugin
  unchanged; not Developer ID/notarization or release acceptance.
- Original MONO-zero acceptance diagnostic rerun: still FAIL (exit 1,
  MIDI/held/MONO 0/1/16), unchanged and outside the registered passing set.

## Separate portamento observation and limits

The first post-edit 64-sample callback in the time-field fixture could report
engine/display 0 while the latest requested value was 99: an older in-flight
native CC5 command rewrote RAM before the newer command was consumed. The final
value settles correctly. This is DIFFERENT from a pre-captured snapshot committing
over a completed UI edit. It remains an explicit open portamento command/ack
display issue, alongside O1 immediate save during recovery; no claim that all
possible GUI reversion or portamento persistence is solved.

The integration fixture allows 100 x 64 samples for serial settling between
schedules and after the immediate publication assertion. This is test isolation
and eventual-value validation, not a new production wait, latency threshold or
weakened assertion for the reproduced Q2 publication window.

State-restore epochs versus concurrent setters, cached pitch/mod delivery,
bypass, native MONO note zero, real DAW acceptance and remaining release gates
stay open. No ROM/core behavior, GUI layout, parameter IDs, main merge, tag,
installed binary or release asset was changed.
