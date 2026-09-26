# Q1: deferred MIDI must not depend on successful callback size

2026-09-23, follow-up on Draft #47. Baseline local
`429721f62f9832666400ae4ed35d7d04355eac7a`, remote
`191f797b4d1a0165db27df37a59f9371fb6801b0`, matching tree
`07551b0812d8a958fee8f4b9273a004bac8947fe`.
Both baseline macOS and Windows Actions succeeded before this round.

## Reproduced before changing production code

The new test uses the real processor, private known v1.8 ROM and controlled
single-sine voice. Another thread holds the actual engine mutex for a 64-sample
callback containing Note On 72 at offset 16. Then rendering resumes and a
future host Note Off arrives, without reset/reload/mode-cycle recovery.

On the baseline at 44.1 kHz, 64- and 1024-sample successful blocks each produce
peak 0.0422761 and release to silence. A successful 88200-sample block instead
produces peak 0 and fails `large successful block lost deferred Note On` (exit 1).
This is now a processor-level reproduction, not merely a queue model.

## Production correction

The previous check compared input time *after* adding the current block with
playback time *before* rendering it. A 64-sample backlog plus a two-second
successful block looked older than the two-second limit, discarding both events.

`advanceInputBlock` now explicitly distinguishes paused from rendering
callbacks. A rendering callback credits exactly the samples that its following
`renderBlock` will advance when checking lag. Paused callbacks add their full
length to lag. Existing panic remains latched until reconciliation; no new
storage, firmware run budget or callback allocation is added.

The processor grants credit only with the engine lock and loaded engine, no
pending reset and no reset already in progress. Muted reset-drain work is NOT
deferred timeline playback, even if the reset finishes within that callback.
The actual lag threshold stays two seconds; the old `max(two seconds, block)`
exception is removed, so a single truly skipped oversized callback cannot extend
the limit. This is not an increase in the allowed waiting time.

## Coverage and results

- ROM-free event timeline: 44.1/48/96 kHz, 64-sample, two-second, three-second
  and mixed (including zero-length) successful partitions. Exact event offsets,
  same-offset pedal/Off ordering and future Note Off verified.
- Lag boundary: 95999/96000/96001/144000 skipped samples at 48 kHz, in one large
  block or 64-sample pieces. Exactly the limit survives; one sample over expires.
  Later zero/large successful callbacks cannot erase panic; fresh events recover.
- Full processor: 15 histories = three sample rates x five partition schedules
  (64, 1024, two seconds, three seconds, mixed small/large blocks). All produce
  audible controlled output, silent final tails and zero held/sustained ownership.
  Stereo sample differences versus the 64-sample reference must be < 1e-6.
- Six expiry cases: true contention, newly requested reset and reset already in
  progress, each with 96001/144000 skipped samples. A previously sounding note
  under sustain is reconciled, stale input is discarded and later fresh notes
  sound/release. Persistent voices/settings/parameters/dirty markers preserved.
- Ordinary C++ callback allocation/deallocation and finite audio checks apply
  throughout; not direct-C/aligned allocator or hard-realtime certification.

Targeted `vdx7_deferred_partition` plus required profile fixture: PASS (7.42 s).
ROM-ON `vdx7_all_tests` and ROM-OFF `vdx7_ci_checks` rebuilt successfully.
ROM-free runtime: 5/5 PASS (1.33 s). Full local registered run: 21/22 PASS in
248.21 s; the sole failure explicitly required desktop/display access for the
SAVE AS GUI test in the sandbox. That same `vdx7_processor` test was rerun with
desktop access and PASSED (8.18 s), with no source/assertion changes. Thus all
22 registered tests have passing results across these runs; do not report the
first sandbox run itself as exit-zero/22-of-22. New partition group: 6.59 s in
that full run. Two known-firmware characterization groups still document a bug,
not a repaired MONO mode.

Standalone queue/validation tests also PASS with AddressSanitizer and
UndefinedBehaviorSanitizer (Apple Clang, C++20, O1); no reported sanitizer error.
This is a component sanitizer run, NOT a full-plugin sanitizer claim.
The unchanged `--mono-note-zero-only` diagnostic was rerun and still FAILS
(exit 1, MIDI/held/MONO counts 0/1/16), outside the registered passing set.

Local arm64 VST3 rebuilt successfully. The existing Xcode-license signing-helper
warning required manual ad-hoc signing of the development build; strict deep
signature verification then passed. No installed bundle was replaced, and this
is not Developer ID/notarization or release packaging acceptance.

Run `vdx7_host_reset_tests /absolute/path/to/private/dx7.bin --deferred-partition-only`
or CTest `vdx7_deferred_partition`; its ownership assertions require the existing
v1.8 profile fixture. Public CI executes the enhanced ROM-free timeline test and
compiles the processor regression; it does not execute or redistribute the ROM.

## Limits

No actual DAW offline-render acceptance, full-plugin sanitizer, general event
capacity/host partition invariance or hard-realtime guarantee is claimed. A
large block with more than the fixed 256 deferred events may still hit the
separate intentional capacity limit; Q1 covers false age expiry within capacity.
Actual skipped samples still cause latency and may trigger the bounded fail-safe.

MONO note-zero is NOT fixed here. Its original acceptance diagnostic remains
unchanged and separate from passing known-firmware characterization. See
`docs/design/DESIGN_MONO_NOTE_ZERO_POLICY.md`; all other release gates remain open.
No GUI, installed plugin, core/ROM, main merge, tag or release changed.
