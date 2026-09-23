# N1 host reset — DRAFT implementation proposal

Baseline: `50a90bd29d710e90c91fd7102f270f5f3e4f19cc`.
This is a source proposal, NOT a validated release or a completed host acceptance.

## Problem and intended behavior

The processor inherits JUCE's no-op `AudioProcessor::reset()`. The pinned VST3
wrapper invokes this entry point on `setProcessing(false)`, independently of
`releaseResources()`. Held voices and stale deferred input therefore need their
own reset path.

The proposal makes `reset()` publish an atomic request and clear atomic meters.
No engine mutex, firmware warm-up, file access, GUI call, or host notification is
performed by that entry point. The audio owner observes the request before
queue processing, and checks again after acquiring the engine try-lock. Input
already collected by a callback that overlaps a late reset is discarded.

The engine's existing non-RT `resetMidiLifecycle()` is NOT called from reset or
from the new callback path. Instead, a bounded release batch is submitted and
the firmware is advanced with at most the current callback's sample budget.
Output remains silent while the release batch drains. Adapter RX, the SCI receive
register, the existing firmware-ring indicators, and the sub-CPU handshake are
included in the proposed completion check. Then EGS/OPS/filter/SRC history is
reinitialized in existing storage and the current program is reloaded. A runtime
output gate remains closed until a fresh accepted Note On, including for patches
with nonzero L4. Old firmware input never reopens that gate directly.

The packed voice bank, program/bank identity, parameter IDs, global settings,
and export markers are intended to survive. Queued pre-observation UI keyboard
input is discarded. A physically held GUI key must be released/repressed.
Fresh host MIDI on a callback after reset returns is deferred during cleanup,
subject to the existing finite deferred-queue capacity and two-second lag policy.
This can add reset-related onset delay; it is not a lossless/unlimited queue.

## Required review before merge

- Confirm the firmware completion predicate is sufficient on the supported ROM.
  The predicate reuses the existing mode-transaction ring locations, but a source
  review alone does not establish completion of every firmware handler.
- Run the new integration test against the old source first (it uses only APIs
  already present there), then against the proposed implementation.
- Measure reset drain duration and fresh-note latency, including a long-lived
  instance whose conservative repeated-note release budget has grown.
- Test notes already in the SCI register/internal firmware queue, sustained notes,
  repeated pitches, repeated reset requests, and live state/ROM changes while a
  reset is being retired. The supplied test does not cover every combination.
- Verify Windows/macOS compiler and sanitizer behavior and actual VST3 host reset.
- A simultaneous reset after the callback's final observation takes effect on
  the next callback. This does not promise to retract samples already rendered.

## Validation status

Current main checked on 2026-09-23: `50a90bd29d710e90c91fd7102f270f5f3e4f19cc`.
The original N1 ZIP was recovered locally and its changes were applied to that
baseline. The earlier reconstructed local draft is not the published patch.

| Check | Status | Evidence / limitation |
| --- | --- | --- |
| Current main matches package baseline | PASS | GitHub compare reports identical, zero commits ahead/behind |
| Original package anchors apply uniquely | PASS | All replacement anchors matched once |
| Source whitespace check | PASS | git diff --check |
| Local full build | NOT RUN | CMake and C++ compiler not available in the local environment |
| Baseline host-reset reproduction | NOT RUN | Requires compiler and explicit user-supplied ROM |
| New host-reset integration test | NOT RUN | Requires compiler and explicit user-supplied ROM |
| Local ROM-free regression suite | NOT RUN | Build toolchain unavailable |
| Full local-ROM suite | NOT RUN | No ROM supplied for this task |
| GitHub Windows/macOS build and ROM-free tests | NOT RUN | Pending Draft PR CI; consult PR checks |
| REAPER/VST3 reset acceptance and fresh-note latency | NOT RUN | Requires local host and compatible ROM |
| Ordinary C++ allocation probe | NOT RUN | Included in the new integration test; not executed |

A static review and clean diff do not establish that N1 is fixed. Firmware drain
completion, repeated/dense notes, long tails and runtime acceptance remain open.

No tag, release, firmware, installed plugin, or dependency source is changed by
this proposal. Bypass handling and the other audit findings are outside this PR.
