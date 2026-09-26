# Reset history-cost comparison (2026-09-23)

Status: measurement stage only; P1 responsiveness remains OPEN. PR #47 must
remain Draft. No production engine/processor behavior changes in this round.

## Reproducible fixture

`vdx7_host_reset_tests <private-local-ROM> --history-pair-only`, also registered
as ROM-only CTest `vdx7_reset_history_pair`. Two independently initialized
processors use 48 kHz / 64 samples and the same synthetic fast-release patch.
One has no played-note history; the other receives 16 repeated Note Ons and
matching Note Offs at each of 128 pitches through public MIDI processing.
Input is paced; overload count must not increase.

Before reset both have no adapter-held notes, silence, no deferred input, empty
serial adapter/SCI/internal firmware rings and no pending sub-CPU handshake.
Persistent bank/patch, tuning, performance settings, parameters and dirty state
are compared across instances and before/after each reset. This is NOT proof of
identical firmware voice-slot ownership or byte-identical runtime state. Idle
queues and silence alone are NOT permission to discard release history.

A zero-sample callback observes the public reset request without audio-time
draining. A read-only test accessor copies the actual serial queue, checks its
status and every note/velocity pair, and counts decoded Note Offs. The next
callback supplies a fresh Note On. Existing immediate-input matrix tests remain
unchanged in sequence (they do not insert this observation callback).

## First direct run, macOS arm64 Release

After rebuilding the reset runner and regenerating CTest registration, the full
local suite passed **13/13 in 139.63 seconds**, including the existing reset
matrix (67.70 s), reactivation (0.77 s) and new paired diagnostic (2.92 s).
This functional/diagnostic PASS does not close P1 latency acceptance.

| Observable | No played-note history | Maximum history |
|---|---:|---:|
| Decoded queued Note Offs | 0 | 2048 |
| Actual queued serial bytes | 0 | 4097 |
| Reset completion, audio block-end time | 1.333 ms | 1512.000 ms |
| First audible fresh-note block-end time | 50.667 ms | 1561.333 ms |
| Zero-sample setup callback wall time | 0.417 us | 2.625 us |
| Maximum observed nonzero callback wall time | 345.959 us | 210.625 us |

Both fresh notes sounded and released; persistent settings matched. Timing is
quantized to 64-sample block ends, not sample-exact onset. Callback wall timing
uses steady_clock immediately around processBlock, excluding subsequent output
checks, over a ten-second simulated audio window including reset and playback.
The allocation probe remains enabled. These are single-run desktop observations,
not worst-case realtime guarantees, pure CPU time, or proof of a faster loaded
instance. Scheduler/instrumentation noise can change the maxima. No latency
threshold is asserted by this diagnostic PASS.

The roughly 1.51-second extra audio delay is independently observable from the
sub-millisecond callback wall times in this run. The 2-second deferred guard is
a fail-safe, NOT musical latency acceptance.

## Required next work, in order

1. Establish a documented firmware-observable retirement invariant during
   normal playback: adapter ownership alone, silence alone and FIFO-empty alone
   are insufficient. Validate firmware voice ownership and completed releases,
   including sustain and repeated notes. Do not infer safety from reset's DSP
   reconstruction or mute gate.
2. Implement retirement only after that proof, retaining running-status release
   encoding and all existing regressions. Clearing history only at successful
   reset completion does not solve the first history-heavy reset.
3. Add adversarial releases at adapter, SCI and internal-queue stages; sustain,
   repeats, overload and fresh MIDI during reset. Verify real voice release
   without output muting masking stale state.
4. Agree an independent numeric reset/onset latency target before marking the
   latency acceptance test green. Do not increase deferred timeout, compress
   extra firmware time into a callback or reopen mute prematurely to pass.
5. Repeat across rates/buffers and real VST3 hosts/platforms. Keep measurements
   distinct from functional test PASS and from release approval.

Firmware remains local and is not included in source or artifacts.
