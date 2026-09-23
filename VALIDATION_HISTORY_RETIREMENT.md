# Guarded normal-playback history retirement

Scope: known local v1.8 firmware, POLY, fully released input/ownership. This is
a bounded production improvement, not closure of all reset-latency acceptance.

## Compatibility and execution boundary

The [annotated v1.8 disassembly](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm)
places the main dispatch loop at 0xc708. MIDI parsing returns to that loop after
its note handlers; parser byte count distinguishes incomplete messages.
The engine checks immediately before executing the next instruction at this
boundary, not while a handler is partway through updating ownership.

All conditions must hold: recognized firmware; POLY; no host reset or overflow
recovery; no adapter-held notes/sustain; empty adapter, SCI and firmware receive
queues; no incomplete parser message or MIDI error; no sub-CPU queue/handshake;
released pedal state; no MIDI-active, held or sustained firmware voice entries.
Then only the host-side historical release budget is cleared. Firmware RAM,
envelopes, audio, controller state and normal Note Off delivery are unchanged.
Release tails may continue normally; a later reset still clears DSP tails.

The complete 16 KiB image is fingerprinted once at ROM load using FNV-1a-64
`20dd25e47a496ba0`. This is a compatibility checksum, not a cryptographic security
boundary. The matching image has SHA-256
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
The user-provided package's firmware hash matches the installed local test ROM.
No archive was installed/extracted into the repository and no firmware is
embedded or uploaded. Unknown/modified images retain conservative behavior.

## Tests and initial measurements

The original expanded-history matrix remains on an explicitly forced
conservative test path, including its 2048 Note Off/4097-byte checks. This keeps
fallback protection instead of deleting the old regression to obtain green CI.
The default known-image path is tested separately through normal MIDI playback.

New ROM-only `vdx7_history_retirement` compares fresh vs 128 pitches x16 released
history at 44.1/48/96 kHz and 64/256 samples. Both firmware ownership tables must
be clear; retirement must happen BEFORE reset. Actual reset queue must contain
zero redundant releases. A relative regression bound requires history not to
add more than two blocks to fresh-instance reset or audible onset. This is not
a product-wide numeric latency acceptance target.

Initial direct run, audio block-end milliseconds:

| Rate/block | History reset | History onset | Fresh onset |
|---|---:|---:|---:|
| 44100/64 | 1.451 | 52.245 | 50.794 |
| 44100/256 | 5.805 | 58.050 | 58.050 |
| 48000/64 | 1.333 | 50.667 | 50.667 |
| 48000/256 | 5.333 | 58.667 | 58.667 |
| 96000/64 | 0.667 | 50.000 | 50.000 |
| 96000/256 | 2.667 | 53.333 | 53.333 |

Previously the matched history case at 48000/64 took 1512 ms reset and 1561.333
ms audible onset. The remaining roughly 50 ms onset also exists in the fresh
fixture; it is not eliminated or declared musically accepted here.

Firmware profile tests reject null/short images and one-bit modifications at
the start/middle/end. A MONO playback test requires history retention. Existing
ownership tests now assert the budget cannot disappear while held, sustained,
queued, in SCI, or in internal processing, but does retire after normal release.

## Remaining limitations

Final local validation: all integration targets rebuilt; full macOS arm64
Release suite **15/15 PASS in 190.19 s**, including conservative reset matrix
(74.39 s), ownership/in-flight guards (3.82 s), and retirement/profile/MONO
checks (29.16 s). The local VST3 linked successfully. The Xcode license warning
still affects the signing helper; the local build bundle was explicitly ad-hoc
signed and passed strict codesign verification. Installed VST3 was untouched.
This is not notarization, distribution signing, or real-host validation.
The local ROM's main-loop instruction layout was also checked against the
documented entry/back-branch and call sequence; it matched.

This only retires history at a completely idle ownership boundary, not per-note
while other voices remain held. Continuous overlapping performances, unknown
ROMs, MONO/legato, voice stealing, overload combinations and real DAW acceptance
still need work. No timeout or callback audio-time budget was raised, and no
early unmute was introduced. Running-status reset encoding is preserved.

Initial wall-time measurements overlapped a build and included a 32.4 ms desktop
outlier on a fresh fixture. They are not realtime WCET/CPU guarantees; controlled
callback profiling remains open. Do not conflate audio delay with wall timing.
Keep #47 Draft pending remaining review and acceptance.
