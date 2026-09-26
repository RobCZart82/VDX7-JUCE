# Lifecycle release completion and overflow-gate follow-up

2026-09-23, follow-up to the user-supplied main/#47 audit. Keep #47 Draft.
No merge, release, firmware upload or installed-plugin replacement is implied.

## R3: failure observed before the production change

Baseline: local `4d19859`, remote #47 `696fe791`, matching tree
`73a5bc8a87919509c6ad1e4e8a3bd25db10bff30`.

The new real-processor test disables the known-image retirement optimization
on the local known ROM to exercise the conservative path. This is NOT testing
an unknown ROM. Play/release all 128 pitches with multiplicity 16, then hold
16 instances of pitch 127, at the end of the release batch. Call public
`prepareToPlay(48000, 64)` and immediately send pitch 72.

Before the fix:

- Immediately after prepare: 16 old MIDI ownership entries remain.
- After one second of processing fresh input: peak 0, pitch-72 ownership 0,
  pitch-127 ownership 16, firmware MIDI/held/sustain counts 16/16/0.
- The acceptance assertion fails. This is actual fresh-note loss, not just a
  discrepancy between host-side counters.

The old lifecycle path drained only 250 ms of emulated audio, then flushed
remaining serial data and aborted handshake flags. The full explicit-status
release batch requires more time.

## Scoped production change

Share release staging with public host reset. Lifecycle retains its minimum
one release per pitch, plus the conservative high-water repeated-note budget.
Use running status (maximum 4097 bytes, still 2048 Note Offs), and reuse the
existing serial/SCI/internal-ring/sub-CPU completion checks before rebuilding
EGS and reloading the current program. Do not clear firmware ownership RAM.

Non-RT lifecycle can advance up to two seconds of emulated audio, capped at
384000 samples; it stops early upon completion. This bound is NOT a completion
criterion. If unfinished, keep the reset pending and muted, preserving the
remaining input and handshake. The existing processor path then defers fresh
MIDI and advances at most the callback's sample budget. No increase to the
two-second deferred-MIDI age limit or per-callback audio-time budget.

## New registered local-ROM tests

`vdx7_expanded_lifecycle` / `--expanded-lifecycle-only`:

- POLY and MONO, each with public prepare-only, public release+prepare, and
  an explicitly forced one-block shared-engine drain to test continuation.
  The third path is a friend test seam, NOT a public prepare/release invocation.
- Every case retains the 128x16 conservative history and 16 held pitch-127
  entries. Empty firmware ownership is required before the held-note fixture.
- Immediate fresh pitch 72 must sound, exclusively own the firmware voice,
  release, and become silent; preserve packed bank, program, tuning, global
  settings, APVTS, dirty/export state, and overload count.
- On/Off observation windows use the existing two-second limit. The forced
  short drain delays BOTH events by about 1.5 s; a one-second Off deadline
  incorrectly precedes its scheduled delivery and is not the acceptance bound.
- Ordinary C++ callback allocation/deallocation and finite audio checks remain.

Direct run (desktop observations, not controlled performance/WCET acceptance):

| Mode / path | Old ownership after lifecycle | Lifecycle wall ms | Fresh onset ms |
|---|---:|---:|---:|
| POLY / prepare | 0 | 95.89 | 49.33 |
| POLY / release+prepare | 0 | 193.64 | 49.33 |
| POLY / forced short drain | 16, still pending | 0.09 | 1565.33 |
| MONO / prepare | 0 | 95.24 | 48.00 |
| MONO / release+prepare | 0 | 197.31 | 49.33 |
| MONO / forced short drain | 16, still pending | 0.09 | 1568.00 |

All six subsequently have MIDI/held/sustain counts 1/1/0 for pitch 72, then
0/0/0 after release. The stopped-device work duration and fallback latency
still need actual-host acceptance; this does not close P1 responsiveness.

`vdx7_reset_gate_overflow` / `--gate-overflow-only`:

Six R4=1/99 x L4=0/70/99 cases. Reset an audible old note, verify silence and
empty firmware ownership, then submit fresh Note On plus 3000 same-offset
pitch events on a zero-sample callback. Inspect the resulting serial queue:
only recovery Note Off packets remain, not the fresh Note On. Across two
seconds, firmware never owns that note and leaked peak is zero. A subsequent
valid note sounds/releases. For R4=99 and nonzero L4, audible post-release output
is required as a positive control. Persistent settings remain unchanged.

These cases pass BEFORE and AFTER the lifecycle change. R1 source ordering
is real, but its proposed audible consequence was not reproduced here. No
gate behavior was changed to satisfy a flag-only model. Nonzero-block bursts,
partial serial dispatch, other ROMs and broader schedules are not covered.

## Separate MONO pitch-zero finding (not hidden by a green suite)

The first all-MONO lifecycle fixture was already corrupted BEFORE lifecycle.
An isolated `--mono-note-zero-only` diagnostic, with no public reset or device
reactivation after entering MONO, reproduces it: 16 pitch-zero Note Ons then
16 Note Offs leave MIDI ownership 0, held count 1 and MONO active count 16.
The diagnostic returns failure and is intentionally not registered as a
passing CTest acceptance test. It remains available for the follow-up.

The local v1.8 behavior is consistent with the native
[MONO add/remove routines](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm):
key zero is used as an empty-slot sentinel; removal returns zero for this key.
Do not patch the firmware or arbitrarily clear its counter to hide this edge.
Compatibility policy and wider boundary tests remain open.

For the independent lifecycle test, pitch-zero history is generated in POLY,
then a normal firmware mode change precedes pitches 1..127 in MONO. Thus the
maximum adapter history remains intact while the starting firmware state is
validated, rather than silently excluding pitch zero from release capacity.

## Build and acceptance status

Local macOS arm64 Release, explicitly supplied private combined ROM. Firmware
SHA256: `6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
No ROM bytes are in source control. All integration targets were rebuilt,
including the four not yet aggregated by public ROM-free CI. Local VST3 built;
the existing Xcode-license signing-helper warning required explicit ad-hoc
signing, then strict signature verification passed. This is not notarization.
The installed VST3 remains untouched.

Full registered suite: **18/18 PASS in 223.83 s**. New lifecycle group: 11.22 s;
gate/overflow group: 2.17 s. The standalone MONO pitch-zero diagnostic remains
a known failing case OUTSIDE those 18 tests; this is not an all-cases-pass claim.
Real macOS/Windows DAW acceptance, unknown-ROM runtime, callback profiling,
continuous-input/long-sustain latency and reset/state/ROM races remain open.
