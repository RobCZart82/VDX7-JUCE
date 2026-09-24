# MONO note-zero: local instruction trace and unrecovered continuation

2026-09-23. Follow-up to the supplied static review, on Draft #47.
Baseline local `0364874a8ce35f33f93928851f2d2e2e9b063488`, remote
`4acf72e875f6144bf1f65eb82ba6f8b6d746c72d`, matching tree
`d6e079afc36d0ee02dcd3a96a82c231a87298785`.

**A concrete mechanism is now observed in the actual local firmware execution,
not just inferred from counters or annotated assembly. The defect is NOT fixed.**
No production source, CPU/peripheral semantics, ROM, installed plugin or GUI
has changed. Keep #47 Draft; no merge/release acceptance is implied.

## Instruction-map verification and observer

The existing known-image preflight is mandatory. Firmware SHA256:
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Before tracing, 34 instruction sites in the loaded private image are decoded
and checked for mnemonic, addressing mode, instruction size and operand/branch
target against the [pinned annotated v1.8 source](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/879a25ee1ccc53f2f66b37285456fbf9c9a1f179/yamaha_dx7_rom_v1.8.asm#L6851).
The checks include allocation, key search, early return, clearing/decrementing,
target pitch and EGS release writes. No ROM file or bulk byte dump is committed.

`Tests/VDX7MonoTrace.h` is a test-only read-only observer around the raw pinned
dx7Lib's normal `run()` calls. It records cycle, PC/opcode, A/B/X/condition flags,
current note, selected two-byte voice entry, MONO count and EGS Key Off writes.
Storage is capped at 4096 selected instruction observations per phase; exceeding
the cap fails the test. Ordinary initialization/MIDI drives the machine; the
observer never changes its registers or memory. It is not a production hook,
an independent CPU implementation or a hard-realtime performance measurement.

## Observed allocation failure

Two Note Ons for pitch zero:

- At `0xD591`, the second free-slot check reads a zero key from an already
  active entry at `0x20B0` (entry value `0x0002`). The zero flag becomes set.
- The branch at `0xD593` chooses the free-slot path `0xD59B`.
- The store at `0xD59F` rewrites the same active entry; `0xD5A1` increments the
  MONO count from 1 to 2 despite only one active table entry.

Pitch-one positive control: the same active-slot check sees a nonzero key,
does not take the free-slot branch, and allocates the second entry at `0x20B2`.
Tests assert both the flag/branch result and the actual entry/count transitions.

## Observed release failure

After finding pitch zero, the key load at `0xD6B1` returns A=0. `TSTA` at
`0xD644` sets the zero flag; `0xD645` branches directly to the return at
`0xD666`. The table clear (`0xD64A`), counter decrement (`0xD651`) and EGS Key Off
are not executed. Two releases leave MIDI/held/MONO counts 0/1/2.

Pitch-one control returns A=1, falls through to `0xD647`, clears both entries,
decrements 2-to-1-to-0, and emits the final EGS Key Off. Final counts are 0/0/0.

Thus the loaded firmware's two zero-key decisions explain the observed mismatch
under this emulator. Physical hardware has NOT been tested, and this is not a
blanket proof of every CPU/peripheral instruction. However, it is no longer
accurate to say there is no identified failing code path in the local run.

## New note without recovery: previously untested consequence reproduced

Sixteen cases: seed pitch 0/1, repetitions 1/16, stacked Ons-then-Offs versus
sequential On/Off pairs, and ordinary Off versus velocity-zero On. Plus a clean
pitch-72 reference. Both raw emulator and processor use MONO with sustain off.
After seeding history, send pitch 72 On/Off without reset, program reload,
mode cycle or memory patch. Check raw and processor ownership, actual held key,
target pitch and the processor's controlled sine output.

| Prior history | Pitch 72 On | After pitch 72 Off |
|---|---|---|
| Clean / pitch-one controls | Correct held key and target pitch, audible reference match | Counts 0/0/0, output silent |
| One pitch-zero On/Off | Pitch 72 plays, MONO count rises from 1 to 2 | Counts 0/0/1; no EGS Key Off, output remains nonzero |
| Sixteen pitch-zero On/Offs | Count-16 branch rejects native allocation; old target pitch remains | Counts 0/1/16, no EGS Key Off, output remains nonzero |

In the saturated case, the actual executed branch at `0xD58D` jumps to
`0xD5F1` with B/count=16 and requested note=72; no `0xD59F` allocation store
occurs. **The firmware MIDI table still receives pitch 72.** Checking only that
table or a nonzero audio peak would falsely suggest successful new-note playback.

The processor's fast-release single-sine fixture has fixed tuning (+123), so
comparison is against its clean pitch-72 reference, not nominal equal-tempered
frequency. Last-quarter-second positive zero crossings yield approximately
533-537 Hz for accepted pitch 72 versus 8 Hz for the old retained low tone in
the saturated case. This is a coarse signal discriminator (about 4 Hz counting
resolution), not a precision pitch/SRC claim. Both raw and processor pitch
targets are compared with their own clean reference. MONO legato is allowed;
we do not incorrectly demand a new EGS Key On for every accepted legato note.

Measured post-release peaks in the new pitch-zero-history cases are about 0.0423,
while clean and pitch-one controls are silent. The low residual frequency is
not necessarily perceptually audible; this is measured signal/state evidence,
not a listening test or a claim for every preset. Sequential On/Off pairs and
both release encodings reproduce the same consequences.

## Tests and boundaries

Run `vdx7_host_reset_tests /absolute/path/to/private/dx7.bin --mono-trace-only`,
or CTest `vdx7_mono_trace_characterization`. It requires the v1.8 profile fixture
and carries the `known-firmware-characterization` label. PASS means the observed
failure path and controls match the documented expectations, NOT repaired
note-off behavior. The original `--mono-note-zero-only` acceptance diagnostic
is unchanged and remains outside the passing registered CTest set.

- Targeted trace plus all 16 continuation cases and clean reference: PASS.
- ROM-ON `vdx7_all_tests` and ROM-OFF `vdx7_ci_checks`: rebuilt successfully.
- ROM-free runtime: 5/5 PASS in 0.93 s.
- Full rebuilt registered local-ROM suite: **21/21 PASS in 245.72 s**;
  new trace/continuation group 13.13 s. Includes two explicitly labelled
  known-firmware characterization groups; it is not all-case release acceptance.
- Original `--mono-note-zero-only` acceptance diagnostic: still FAIL, exit 1,
  MIDI/held/MONO counts 0/1/16, unchanged assertion.

No arbitrary counter clearing, automatic mode change, note suppression,
transposition or firmware patch was implemented. Such behavior must not be
smuggled in as a CPU fix. The firmware-faithfulness policy still applies; any
compatibility alternative needs a separate design/decision covering allocation,
release, legato and subsequent notes, not just silencing output.

Physical-hardware/independent-emulator confirmation, broader MONO/portamento/
sustain cases and real DAW acceptance remain open. Q1 and the other separate
stabilization items in AUDIT_TRIAGE_2026-09-23.md are not solved by this work.
