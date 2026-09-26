# Firmware ownership observations (2026-09-23)

P1 reset latency remains OPEN. This round adds read-only test observation and
regressions, NOT production history retirement. Preserve #47 as Draft.

## Basis and scope

The [AJXS v1.8 annotated disassembly](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm)
documents separate MIDI-event and voice-status arrays: 16 MIDI entries at
0x2168 (active bit 7) and 16 two-byte voice records at 0x20b0 (held bit 1,
sustain bit 0 in the status byte). The MIDI removal path clears its entry
before invoking voice removal; sustain can keep a voice active after the MIDI
entry clears. These facts motivate testing both arrays, not just the adapter.

The test observes those addresses without modifying firmware or RAM. It checks
transitions against public engine MIDI input. This validates the observed map
for the local fixture, NOT every supported/custom firmware image.
Local firmware payload SHA-256 (first 16384 bytes, no ROM uploaded):
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.

## Added checks

CTest `vdx7_firmware_ownership`, or reset runner `--ownership-only`:

- 12 normal-playback cases: notes 0/60/127, one/sixteen repeats, sustain off/on.
  Held MIDI and voice counts match before release. Immediately after queued
  Note Offs the adapter count is already zero (without sustain), while both
  firmware tables still retain the held notes. This is a concrete counterexample
  to adapter-count-only retirement.
- After releases settle with sustain held, the MIDI table is empty and input
  queues are idle, but all voices remain marked sustained. Pedal release clears
  them. No reset, output mute change or EGS reconstruction masks this check.
- One-sample observations must witness release data in all three stages:
  adapter, SCI receive register, and internal receive ring/pending flag.
- Six reset cases: sixteen repeats, sustain off/on, reset at each stage. SCI
  cases require adapter-empty; internal-stage cases require adapter AND SCI
  empty. Thus these are distinct late-input fences, not merely three labels on
  one full queue. An immediate fresh note must sound and leave exactly one
  firmware MIDI/held entry and no sustained voices. Its Note Off must clear both
  tables and sound; persistent settings must be unchanged.
- The existing paired-history test now checks zero held/MIDI/sustained entries
  in BOTH instances before reset. Its measured latency discrepancy persists.

## What this does not establish

Validation: rebuilt macOS arm64 Release reset runner; full local suite
**14/14 PASS in 144.94 seconds**. Ownership runner (18 scenarios) passed in
3.63 seconds, paired history in 2.93 seconds, existing host-reset matrix in
68.85 seconds. No old assertion was removed. No installed plugin was replaced.

These snapshots are not instruction-level proof of a safe retirement point.
Ownership writes, parser completion, interrupt work and EGS key-event delivery
can be separated in time. The documented map must not silently become a
universal production assumption. Tests use POLY; MONO/legato, voice stealing,
overload, parser error paths and other ROM variants still need coverage.
The fresh-note test checks cardinality, audible output and release, not an
exhaustive pitch/voice allocation trace. Callback timing is not profiled here.

## Next implementation gate

Identify a completed firmware dispatch boundary and verify its applicability
to supported ROMs. The annotated v1.8 main-loop entry at 0xc708 is a candidate
to investigate, not an enabled or verified retirement hook. At that boundary,
combine fully consumed input, no pending
controller/pedal handshake, and closed MIDI/voice ownership. Prove that no
already-consumed input can subsequently re-create ownership. Only then retire
history during normal playback, with conservative fallback for unknown ROMs.
Do not use an arbitrary wait, silence threshold, FIFO-empty alone, or reset-end
budget clear as the proof. Keep release tails separate from held ownership:
the reset still has to stop tails after safely processing input.

The 2-second deferred fail-safe and callback firmware-time budget are unchanged.
Numeric musical-latency acceptance remains a separate open decision.
