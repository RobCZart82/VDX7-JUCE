# MONO note-zero: fidelity boundary and corrective-design decision

Latest integration: project-state persistence now retains explicit correction
intent, including missing-ROM saves and legacy-native fallback. See
`VALIDATION_MONO_PERSISTENCE.md`. GUI selection/status is now implemented in
`VALIDATION_MONO_SETTINGS.md`, with broader host acceptance pending; the following
historical design/experiment sections are not a current implementation inventory.

2026-09-23. Design note, **not an implemented fix or permission to patch firmware**.
The default remains firmware-faithful operation. Keep Draft #47;
MONO note-zero acceptance and release approval are still open.

User decision (2026-09-23 follow-up): targeted corrective development is approved,
while preserving the original/default path. This approves investigation and a
separately tested correction, not a particular implementation, automatic mode
cycling, silent pitch substitution, arbitrary ROM/RAM patches, or publication.
An engine-only pre-load opt-in is now implemented and exercised through the
processor; a user-selectable/persisted product mode is NOT complete. See
`VALIDATION_MONO_ENGINE_OPTIN.md`. The independent portamento
request/save fix is complete within its documented test scope; it does not close
MONO acceptance. The isolated branch-decision experiment described in
`VALIDATION_MONO_CANDIDATE.md` is the next design evidence, NOT a product fix.

## What is established

The local known-v1.8 instruction trace identifies both zero-key allocation and
release failures; see `VALIDATION_MONO_INSTRUCTION_TRACE.md`. The failure also
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

1. **Native path (current authorized baseline).** Preserve the ROM's behavior
   and explicitly document the limitation. Do not silently reject/transplant
   pitch zero or convert MONO to POLY. This choice does not turn the failing
   note-zero test into a passing release test. Shipping with this limitation
   would require explicit release acceptance; none is granted here.
2. **Optional corrected path (development direction approved; design pending).**
   Define a clearly named, persisted compatibility option with
   native behavior as the legacy-project/default fallback. A corrective design
   must address allocation occupancy AND found/not-found release status, count
   consistency, legato priority and subsequent notes. It must be scoped to a
   verified compatible image; unknown images must not receive an assumed RAM
   or instruction patch. The branch-decision hook has initial engine/processor
   validation; complete product-option/lifecycle acceptance is still pending.
   Do not modify/distribute ROM bytes as part of the present work.

An explicit user-requested native mode cycle can clear ownership in the measured
fixture, but interrupts/reconfigures voices. It is recovery, not transparent
correct playback, and must not run automatically after a zero-note message.
The ordinary plugin reset must not be advertised as a proven universal cure
for native MONO ownership either.

## Required acceptance before any corrected path could be called fixed

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
stays native. Live switching and option/state integration remain pending.

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

The existing unchanged `--mono-note-zero-only` production diagnostic is now
registered as `vdx7_mono_note_zero_acceptance`, labelled `release-blocker`.
No WILL_FAIL, skip or expected-bug conversion: the full local CTest invocation
must report failure while this product P1 remains unresolved. Public ROM-free
CI still cannot run it. A passing experiment cannot substitute for this gate.
The native FAIL exposes the pre-existing defect; it is not a regression caused
by the unlinked experiment. An expected rejection in an oracle-sensitivity test
is a different result and must never convert that native failure to PASS.
When a corrected product mode exists, test its explicit activation and native
fallback independently; never silently flip the expected defect into acceptance.

Before selecting this method for production, resolve optional-mode persistence,
safe changes with held notes, known/unknown-image guards, every CPU stepping path,
actual processor/save/restore/reset tests and callback overhead. A test-only
condition intervention is not authorization for an undisclosed production hook.

## Independent development alongside corrective design

Fix independent, reproduced wrapper errors without changing firmware policy.
The next such item, Q1 deferred-MIDI block-partition loss, is implemented and
tested separately in `VALIDATION_DEFERRED_PARTITION.md`. Q2/publication races,
state epochs, bypass/controller handling and host acceptance remain tracked in
`AUDIT_TRIAGE_2026-09-23.md`. None closes MONO note-zero by implication.
