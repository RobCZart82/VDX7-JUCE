# Per-pitch retirement during overlapping playback

Follow-up to `VALIDATION_HISTORY_RETIREMENT.md`, on the same locally validated
v1.8 firmware/POLY path. #47 remains Draft; this does not close P1 or authorize
release publication. Firmware bytes and the installed plugin are not changed.

## Reproduce first, then change production

The new `--overlap-only` regression was first built/run against the preceding
global-idle implementation. It failed with:

`FAIL: released neighboring pitches retain history while anchor is held`

The no-history anchor-only control passed first (44100/64: 2.90249 ms reset,
53.6961 ms onset). This is a verified regression in the earlier retirement
coverage, not a conclusion inferred from a green build.

## Bounded production change

Keep all prior recognized-image, main-loop-boundary, POLY, parser/error,
input-queue, sub-CPU handshake, sustain and recovery guards. Adapter sustain
still blocks all retirement. A neighboring held note no longer blocks it.

Read both firmware ownership tables at the guarded boundary and protect each
pitch present in either table. A pitch's budget is cleared only when its host
held count is also zero. For a pitch still owned anywhere, preserve the whole
previous high-water budget (do not reduce it to the apparent current count).
An out-of-range active voice record aborts retirement conservatively. The
operation uses fixed-size storage; it does not write firmware RAM or alter
envelopes, normal MIDI delivery, reset mute timing or the two-second fail-safe.

The RAM interpretation is based on the
[annotated v1.8 source](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm)
and checked with the local-ROM tests. It is not applied to unknown ROMs or MONO.

## New ROM-only test: vdx7_overlap_retirement

- Six fresh/history pairs: 44100/48000/96000 Hz x 64/256 samples. Hold pitch 60
  throughout, then play/release every other MIDI pitch 16 times, grouped by
  pitch. This includes the 17th simultaneous input exceeding the 16-entry MIDI
  ownership capacity. Verify that the anchor survives and that all neighboring
  ownership is released; do not assume every submitted Note On was allocated.
- Before reset: firmware has exactly one MIDI-owned/held voice, no sustained
  voice; all neighboring budgets are zero and the anchor budget is one.
- Observe public reset on a zero-sample callback and inspect the actual serial
  queue: exactly `80 3C 00`, one anchor release. Immediate fresh pitch 72 must
  become audible, replace old ownership, and release normally. Persistent bank,
  patch, performance, APVTS and dirty/export settings must remain unchanged.
- Relative bound: history may add no more than two blocks to reset completion
  or audible onset versus the fresh anchor-only instance.
- Six pending-release overflow cases: adapter/SCI/internal input stage, each
  with sustain off/on. First retire a repeated neighboring pitch, then queue
  the anchor release and force overflow at the selected stage. Its release
  budget must survive until recovery. Check actual firmware ownership, silence,
  fresh-note onset/release and persistent state without calling reset.
- Callback/reset ordinary C++ allocation probes remain enabled. These do not
  cover every allocator and are not a hard-realtime proof.

Initial direct overlapping-reset measurements (audio block-end milliseconds):

| Rate/block | History reset | History onset | Fresh reset | Fresh onset |
|---|---:|---:|---:|---:|
| 44100/64 | 2.902 | 52.245 | 2.902 | 53.696 |
| 44100/256 | 5.805 | 58.050 | 5.805 | 58.050 |
| 48000/64 | 1.333 | 50.667 | 2.667 | 53.333 |
| 48000/256 | 5.333 | 58.667 | 5.333 | 58.667 |
| 96000/64 | 1.333 | 50.000 | 2.000 | 50.667 |
| 96000/256 | 2.667 | 53.333 | 2.667 | 53.333 |

These are fixture results, not real-host acceptance, universal latency limits
or controlled callback wall-time measurements. The roughly 50 ms fresh-note
onset remains in this fixture and is not declared musically accepted.

## Remaining boundaries

Final rebuilt local macOS arm64 Release suite: **16/16 PASS in 220.72 s**.
The new overlap/overflow group passed in 24.92 s; prior maximum-history reset,
ownership, sustain, lifecycle, capacity/overflow and state tests also passed.
The local VST3 linked. Its existing Xcode-license signing-helper warning was
handled by explicitly ad-hoc signing the local build, followed by successful
strict codesign verification. This is not distribution signing/notarization;
the installed VST3 is untouched. Public ROM-free CI compiles the integration
test executable but cannot substitute for these private-ROM runtime tests.

Unknown ROMs and MONO/legato keep conservative history. Broader voice stealing,
continuous input that never reaches an empty receive boundary, long-held
sustain, reset/state/ROM races, the separately reported overload/unmute case,
controlled callback profiling and actual macOS/Windows host acceptance remain
open. Prior conservative-history tests and ownership/stage/sustain regressions
are retained. This change covers the stated overlap/overflow cases only.
