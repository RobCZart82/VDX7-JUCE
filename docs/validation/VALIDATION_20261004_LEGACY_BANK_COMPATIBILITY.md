# Archived DX7 bank compatibility

The owner requested a software-side fix for archived ROM3A, ROM3B and ROM4A
SysEx banks, without modifying downloaded patch files or removing malformed-data
checks. This change separates canonical editor ranges from a narrowly bounded
storage/import policy. It does not establish Yamaha provenance for archive files.

## Accepted stored values

Canonical ranges remain unchanged. Two additional stored-value cases are allowed:

| Parameter | Additional accepted value |
| --- | --- |
| Operator EG rates R1–R4 and levels L1–L4 | 127 only |
| Operator fine frequency | 100 only |

Other out-of-range values are rejected, including operator EG 100–126 and fine
101–127. Output level, breakpoint/depth, pitch EG, LFO, transpose and detune
limits are not widened. Every packed voice now checks its own seven-bit bytes,
including the name, even when called outside the SysEx decoder.

Header, exact message length, channel, seven-bit data and checksum checks remain.
Failed imports do not replace existing or pending state. Reserved VMEM bits are
retained, subject to the existing seven-bit transport rule.

## Data preservation

The shared packed-voice validator applies to file import, combined/companion
factory images, project RAM validation, USER storage and export. Bank import and
export retain the original packed bytes. Single-voice VCED import/export retains
the two supported legacy parameter cases without passing them through the
editor's 0–99 clamping getters/setters. VCED cannot represent VMEM reserved bits;
therefore its packed-byte round trip is not an oracle for reserved-bit identity.

Passive GUI/host parameter publication remains canonical and does not alter raw
RAM. Explicit edits still use canonical ranges and change only edited fields.
No GUI layout, editor parameter range, DSP algorithm or factory input file changes
are part of this fix.

## Local file evidence

All eight owner-provided ROM1A–ROM4B files are 4104-byte bank messages. The fixed
production decoder accepts all eight, and bank re-encoding retains every byte
(with the existing export channel convention). Truncated and checksum-corrupted
variants remain rejected. Input file SHA256 values were unchanged after testing.

The four previously rejected values are:

| Bank and slot | Voice | Field | Raw value |
| --- | --- | --- | --- |
| ROM3A 20 | TIMPANI | OP2 EG L3 | 127 |
| ROM3B 02 | E.GRAND 1 | OP6 EG R1 | 127 |
| ROM3B 15 | 60-S ORGAN | OP1 fine | 100 |
| ROM4A 08 | HORNS | OP5 EG L4 | 127 |

Prior diagnostic trials used the identified original v1.8 firmware and a fresh
engine for each voice. All 96 voices in these three banks produced finite,
nonzero output for the selected C4 note trial. ASan/UBSan reported no error.
Changing the exceptional values to 99 changed firmware registers; two selected
audio comparisons also changed. This supports raw preservation, not automatic
normalisation or a comprehensive hardware-fidelity claim.

## Regression coverage

Public fixtures are synthetic and contain no Yamaha firmware or factory patches.
They cover every operator/EG field, fine-frequency boundary, all 32 bank slots,
all device channels, raw single/bank export, canonical passive reads and explicit
editing. Negative cases retain checks for other semantic fields, high-bit bytes,
bad checksum and truncation. USER tests cover persistent raw parameter bytes and
single-voice export. Processor tests cover pending project state without firmware,
and local-ROM import, passive publication, export, restore and transactional
rejection after a successful legacy import.

The local firmware-dependent focused processor runner is:

```sh
vdx7_processor_tests --legacy-bank-compatibility /path/to/local/compatible/ROM
```

The full local processor test also executes this coverage. The ROM-free pending
state check runs inside the existing public `vdx7_pre_rom_state` registration.

## Local verification results — 2026-10-04

Base: main `c8726eedfa54ca1dbacefd028fc25b767bb93de2`. Tests were run against
the changes in this development round, not the previous b7fce05 binary package.
Platform: Apple Silicon macOS. Release and instrumented RelWithDebInfo builds
used the same private local v1.8 fixture; no installed plug-in was replaced and
REAPER was not started. ASan/UBSan used `detect_leaks=0` (LeakSanitizer is not
covered), `halt_on_error=1` and frame pointers.

| Check | Result and limits |
| --- | --- |
| Release CTest, ROM-free plus local-ROM tests | PASS 44/44; 314.76 s, parallel 2, desktop access for SAVE AS |
| Python regression tests | PASS 77/77; 13.925 s |
| Required ROM-free ASan/UBSan component set | PASS 8/8; 4.59 s, same selection as the public sanitizer workflow |
| Focused ASan/UBSan legacy processor runner | PASS: pending state, import, passive sync, exports, restore and negative import |
| Full processor ASan/UBSan rerun with desktop access | PASS: v1.8 fixture plus processor, 2/2; 37.43 s |
| Standalone voice/SysEx ASan/UBSan tests | PASS, including exhaustive stored-value boundaries |
| Original bank files, fixed instrumented decoder | PASS 8/8; byte-preserving complete-bank export and negative transport controls |
| Windows/macOS GitHub checks for the new PR head | Pending; local macOS results are not Windows CI evidence |
| Final package/source verification and owner REAPER acceptance | Not run for the new candidate |

The optional full-ROM instrumented CTest trial completed at 38/44 in 965.52 s,
parallel 2, and is **not a full PASS**: MIDI
range (90 s), host reset (180 s), portamento (120 s), corrected processor (120 s)
and the MONO experiment (180 s) reached existing CTest deadlines. The initial
processor run failed its explicit desktop-access check in the sandbox; rerunning
that exact test with desktop access passed. All five time-limited tests passed
in the full Release run above. No CMake timeout or assertion was relaxed.
Passing normal tests or a focused sanitizer test does not turn timed-out
instrumented cases into sanitizer PASS. Required public ROM-free sanitizer
coverage remains separate from this extended private-ROM trial.

## Release boundary

This supersedes the blanket semantic rejection policy for these specific legacy
values described in the retained October 2 bank integrity report. That report
remains historical evidence and has not been rewritten. Strict malformed-data
and state-preservation protections remain in force.

This fix changes production code relative to the prior b7fce05 candidate. New
Windows/macOS builds, package/checksum/source verification and the owner's exact
final-candidate REAPER tests are required before publishing 1.0.1. Existing
accepted packages must not be described as containing this fix or silently
replaced. No release or tag is published by this development round.
