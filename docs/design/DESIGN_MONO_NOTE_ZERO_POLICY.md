# MONO note-zero: fidelity boundary and corrective-design decision

Current status (2026-09-24): the optional correction is integrated, selectable
and project-persisted; see `docs/validation/VALIDATION_MONO_PERSISTENCE.md` and
`docs/validation/VALIDATION_MONO_SETTINGS.md`. PR #47 is merged to `main` as `30a3ccbd`.
The common 12–120 input filter is active in both modes. macOS/Windows CI passed;
full local ROM-enabled and exact-boundary REAPER acceptance remain outstanding.
The native raw-firmware Note 0 limitation is still documented, not claimed fixed.

Settings decision (2026-09-24): Native firmware remains the recommended default
for ordinary use. Correct MONO Note 0 remains selectable as an advanced,
under-the-hood compatibility option; routine toggling is not recommended. Both
modes share the same Note 12–120 input range.

The following dated design and experiment notes are historical evidence, not a
current implementation inventory or release-approval statement.

2026-09-23. Design note, **not permission to patch firmware**. The default was
firmware-faithful operation; targeted corrective development was later approved.

Historical 2026-09-23 decision: investigate a separate correction while
preserving native/default behavior; do not automatically cycle modes, transpose
input, or patch arbitrary ROM/RAM. Later reports below describe the staged
implementation and are superseded by the current status above.

## What is established

The local known-v1.8 instruction trace identifies both zero-key allocation and
release failures; see `docs/validation/VALIDATION_MONO_INSTRUCTION_TRACE.md`. The failure also
occurs in the raw pinned core, without the plugin adapter. After one zero-note
pair, the next note can fail to release; after sixteen, its native allocation
can be rejected. These are not merely stale GUI values or reset bookkeeping.
Physical DX7 hardware / an independent CPU implementation have not confirmed it.

Required invariant: **an empty slot and an occupied slot whose MIDI key is zero
are different states**. Allocation, skipping empty slots, matching-key lookup,
release/counting and legato selection must use the same occupancy rule, not
infer it independently from a zero/nonzero pitch value.

Changing how this verified execution handles a zero key would be a compatibility
change, not an established correction to CPU instruction semantics. Investigating
independent CPU/peripheral accuracy remains valid, but a note-specific exception
must not be disguised as a CPU fix.

## Separate the two product choices

### Superseding product input-range decision — 2026-09-24

The user observed in REAPER that Note 0 can leave the native MONO engine silent;
Note 12 / C0 was the lowest manually tested note that did not trigger the lockup.
The user also found the 0–11 octave musically unnecessary for this DX7 product.
Therefore the accepted product input range is Note 12–120 inclusive in BOTH
compatibility modes. Note On, Note Off and velocity-zero Note On outside that
range are filtered before deferred storage and before engine delivery. No pitch
substitution or firmware/RAM modification is made. Note 121–127 are excluded
by the selected product range, not because a matching failure was reproduced.

This adapter guard is not a claim that native firmware Note 0 was fixed. The raw
firmware issue remains documented by engine-level tracing; the former plugin-
facing failing Note 0 acceptance check is superseded by range acceptance. The
engine-level optional correction remains available for compatibility. The merged
plugin guard prevents incoming MIDI from triggering Note 0 in either mode.
REAPER tests for
exact boundaries, both modes, held-note transitions and post-filter normal-note
recovery are still required.

1. **Native path (current authorized baseline within product range).** Preserve
   the ROM's behavior for accepted Notes 12–120 and explicitly document the
   out-of-range adapter filter. The raw native Note 0 defect remains documented
   and must not be re-labelled as fixed; its acceptance diagnostic continues to
   expose the firmware-level failure.
2. **Optional corrected path.** The selectable and persisted compatibility
   option is implemented and covered by targeted processor tests. Preserve its
   known-image guard and the established allocation/release/count/legato rules;
   unknown images must not receive an assumed RAM or instruction patch. Keep
   its engine-level Note 0 regression evidence distinct from the current product
   filter, which intentionally prevents Note 0 input in both modes. Do not
   modify/distribute ROM bytes as part of the present work.

An explicit user-requested native mode cycle can clear ownership in the measured
fixture, but interrupts/reconfigures voices. It is recovery, not transparent
correct playback, and must not run automatically after a zero-note message.
The ordinary plugin reset must not be advertised as a proven universal cure
for native MONO ownership either.

## Required acceptance before any corrected path could be called fixed

The engine-level Note 0 pitch/release criteria below remain applicable to the
correction implementation in its direct processor/engine acceptance tests. The
2026-09-24 product contract supersedes Note 0 playback as a DAW-facing feature:
the plugin boundary must now reject Note 0–11 and 121–127 in both modes, while
allowing and releasing Notes 12–120. Keep both acceptance layers explicit.

- Preserve the native failing diagnostic and read-only characterization as
  references; add separate corrected-path tests, never weaken their assertions.
- Note 0 must genuinely sound with its original key identity and the target
  pitch/audio of a native reference, not merely clean up successfully. Use an
  additional +12-transposed patch control to distinguish Note 0 from Note 1:
  their neutral-transpose targets coincide at the bottom of this firmware's
  pitch table in this specific test fixture, not a claim for every voice/hardware.
  This is a patch fixture, never corrective input-event transposition.
- Prove the pitch oracle is sensitive: with that +12 patch, unchanged Note 0
  must pass; only the test input changed to Note 1, while still expecting Note 0,
  must fail the SAME rendered-audio comparison and tolerance. Key/target metadata
  mismatch or a setup failure cannot substitute for frequency rejection. This
  negative control is test-only and never rewrites production MIDI.
  Run this same positive/negative pair through the explicitly enabled corrected
  processor path, not only the isolated experiment. Record tested commit/tree,
  expected and both measured frequencies, patch transposition and tolerance.
  Report negative-control PASS (expected audio rejection) separately from the
  mutant pitch-check FAIL and the still-independent native product acceptance.
- Notes 0/1/60/127, normal Off and velocity-zero On, sequential and stacked
  repeats, 1/16 and over-capacity histories; inspect actual ownership and counts.
- Mixed-note legato, release order, voice replacement, sustain and portamento;
  measure the requested pitch, not only nonzero audio or MIDI-table acceptance.
- Subsequent notes without reset must sound at the requested pitch and release.
- Reset, mode changes, save/restore, option changes with held notes, absent or
  incompatible ROM; no state/parameter compatibility regression.
- No unbounded/allocating audio work; full local ROM regressions, platform CI
  and real-host validation. Independent hardware evidence stays distinct.

## Experimental scope and visible acceptance gate

Integration (2026-09-24): the experiment and engine share the pure six-site
decision helper, `VDX7MonoCorrection.h`, covered by ROM-free CI. Engine use is
explicitly opt-in before ROM load and guarded by image/map verification.
The processor test opts in before initialization; ordinary plugin construction
stays native. This paragraph records the earlier engine integration stage;
live switching and option/state integration were completed in later rounds.

`Tests/VDX7MonoCandidateTests.cpp` is a separately compiled test executable, not
linked into VDX7. With explicit known-image preflight it overrides only the Z
condition immediately before selected native branches. It never rewrites ROM
bytes, note values, occupancy entries or counters. This is intentionally changed
execution in a disposable machine, NOT native CPU semantics or a deployable fix.

The two originally highlighted decisions are insufficient: clearing the first
zero slot makes the next lookup stop at that empty slot, ignoring a later active
zero. Correcting that lookup still leaves legato's first/minimum/maximum searches
using zero pitch as emptiness. All these consumers need one occupancy contract.
See the experiment's negative controls before its complete candidate tests.

Historical reports below describe the old plugin-facing Note 0 failure and
`--mono-note-zero-only` test. Superseding 2026-09-24 behavior filters pitches
outside 12–120 in both modes; current local product acceptance is
`vdx7_supported_note_range_acceptance`. The raw firmware failure remains covered
by separate engine-level characterization and has not been turned into a pass.

Before selecting this method for production, resolve optional-mode persistence,
safe changes with held notes, known/unknown-image guards, every CPU stepping path,
actual processor/save/restore/reset tests and callback overhead. A test-only
condition intervention is not authorization for an undisclosed production hook.

## Independent development alongside corrective design

Fix independent, reproduced wrapper errors without changing firmware policy.
The next such item, Q1 deferred-MIDI block-partition loss, is implemented and
tested separately in `docs/validation/VALIDATION_DEFERRED_PARTITION.md`. Q2/publication races,
state epochs, bypass/controller handling and host acceptance remain tracked in
`docs/archive/AUDIT_TRIAGE_2026-09-23.md`. None closes MONO note-zero by implication.
