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
- The old-source/new-source integration comparison is complete; see results below.
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
| Local Windows Release VST3 and all test targets build | PASS | VS Build Tools 2022 17.14.41, MSVC 19.44.35229, SDK 10.0.26100.0, CMake 3.31.6-msvc6 |
| Baseline host-reset reproduction | PASS | Test-only commit 7ad444628e2f1db639f2c1024c8d2a66630e6f57: expected CTest FAIL in 0.39 s, `host reset left held audio or a release tail` |
| New host-reset integration test | PASS | Implementation commit 90b2c840cc2ab62690f1f3fe54b474e60c582b60: 8.28 s; 44100/48000/96000 Hz at 64/256 samples, nonzero L4, contention, stale/fresh input and state preservation |
| Local ROM-free regression subset | PASS | Five tests passed |
| Full local-ROM suite | FAIL | Combined result: 9/11 pass; existing vdx7_stability and vdx7_processor runners overflow Windows stack (0xC00000FD); both reproduced on the test-only baseline as well |
| MIDI range, timing and stress regressions | PASS | 257.30 s, 5.66 s and 29.93 s respectively |
| GitHub Windows/macOS build and ROM-free tests | PASS | Implementation commit: Windows run 35848462256; macOS run 35848462314 |
| REAPER/VST3 reset acceptance and fresh-note latency | NOT RUN | Requires local host and compatible ROM |
| Ordinary C++ allocation probe | PASS | Included in the passing host-reset integration test |
| Sanitizers and macOS local-ROM runtime | NOT RUN | Not executed |

The old-source failure and fixed-source pass establish the targeted regression
using the user's local combined ROM; no firmware was uploaded. The full suite
is not green, and the direct-API test does not replace real-host acceptance,
long-lived-instance latency measurements or exhaustive firmware/race testing.
No unrelated test or engine edits were made to conceal the existing stack overflow.

Reproduction: configure separate clean worktrees at the two commits above with
VDX7_ENABLE_ROM_TESTS=ON and VDX7_TEST_ROM_FILE pointing to a private local ROM.
Build Release target vdx7_host_reset_tests and run:

```text
ctest --test-dir <build> -C Release --output-on-failure --no-tests=error -R ^vdx7_host_reset$
```

On the fixed tree also build VDX7_VST3 and vdx7_all_tests, then run the remaining
tests with `-E ^vdx7_host_reset$`. The ROM path is deliberately not recorded here.

No tag, release, firmware, installed plugin, or dependency source is changed by
this proposal. Bypass handling and the other audit findings are outside this PR.
