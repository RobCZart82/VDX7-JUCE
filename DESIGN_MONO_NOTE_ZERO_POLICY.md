# MONO note-zero: fidelity boundary and corrective-design decision

2026-09-23. Design note, **not an implemented fix or permission to patch firmware**.
The default remains firmware-faithful operation. Keep Draft #47;
MONO note-zero acceptance and release approval are still open.

User decision (2026-09-23 follow-up): targeted corrective development is approved,
while preserving the original/default path. This approves investigation and a
separately tested correction, not a particular implementation, automatic mode
cycling, silent pitch substitution, arbitrary ROM/RAM patches, or publication.
No corrected mode is implemented yet. The independent portamento request/save
fix is developed first; it does not close MONO acceptance.

## What is established

The local known-v1.8 instruction trace identifies both zero-key allocation and
release failures; see `VALIDATION_MONO_INSTRUCTION_TRACE.md`. The failure also
occurs in the raw pinned core, without the plugin adapter. After one zero-note
pair, the next note can fail to release; after sixteen, its native allocation
can be rejected. These are not merely stale GUI values or reset bookkeeping.
Physical DX7 hardware / an independent CPU implementation have not confirmed it.

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
   or instruction patch. An implementation method has not yet been selected or
   validated. Do not modify/distribute ROM bytes as part of the present work.

An explicit user-requested native mode cycle can clear ownership in the measured
fixture, but interrupts/reconfigures voices. It is recovery, not transparent
correct playback, and must not run automatically after a zero-note message.
The ordinary plugin reset must not be advertised as a proven universal cure
for native MONO ownership either.

## Required acceptance before any corrected path could be called fixed

- Preserve the native failing diagnostic and read-only characterization as
  references; add separate corrected-path tests, never weaken their assertions.
- Notes 0/1/60/127, normal Off and velocity-zero On, sequential and stacked
  repeats, 1/16 and over-capacity histories; inspect actual ownership and counts.
- Mixed-note legato, release order, voice replacement, sustain and portamento;
  measure the requested pitch, not only nonzero audio or MIDI-table acceptance.
- Subsequent notes without reset must sound at the requested pitch and release.
- Reset, mode changes, save/restore, option changes with held notes, absent or
  incompatible ROM; no state/parameter compatibility regression.
- No unbounded/allocating audio work; full local ROM regressions, platform CI
  and real-host validation. Independent hardware evidence stays distinct.

## Independent development alongside corrective design

Fix independent, reproduced wrapper errors without changing firmware policy.
The next such item, Q1 deferred-MIDI block-partition loss, is implemented and
tested separately in `VALIDATION_DEFERRED_PARTITION.md`. Q2/publication races,
state epochs, bypass/controller handling and host acceptance remain tracked in
`AUDIT_TRIAGE_2026-09-23.md`. None closes MONO note-zero by implication.
