# Reset request retirement at stopped-device lifecycle boundaries

2026-09-23; base `98d61b8` (merged #46). Narrow fix only: does NOT resolve the
expanded-history fresh-note loss already present in the full host-reset test.

## Before / after

New production-path regression before the fix: FAIL in 5.98 s at
`stale host reset survived release/prepare`. Earlier reset cases passed.

`prepareToPlay` and `releaseResources` now atomically consume requests that are
already present when their engine-owned cleanup starts, and retire the old
audio-owned pending flag. They do this BEFORE cleanup, never after it. A reset
published after the exchange remains available for the next callback (or the
next full stopped-device cleanup). Public `reset()` stays nonblocking and does
not acquire the engine mutex. As before, lifecycle methods require the host to
stop audio callbacks; this does not introduce support for concurrent prepare
and processBlock calls.

## Targeted result

macOS arm64 Release, Apple clang 21, private local combined ROM, 48000 Hz / 64:
`vdx7_reactivation` PASS in 1.53 s. Four cases cover prepare alone and
release/prepare, each with an unobserved atomic request and an audio-observed
pending request held behind a contended engine mutex. Tests check request
retirement, persistent state, no redundant reset in subsequent callbacks,
audible fresh note, Note Off ownership release, and a genuinely new reset after
prepare still silencing the old tail. Large fixtures remain heap-owned.

The same executable accepts `--reactivation-only` for the separately registered
CTest case; the normal full host-reset test retains the reactivation checks and
the known failing expanded-history assertion. No threshold was relaxed, and the
global two-second deferred limit is unchanged.

## Scope / remaining evidence

No direct VST3 host interaction, Windows execution, sanitizer run or deterministic
injection DURING lifecycle cleanup was performed here. The concurrent reset
preservation follows exchange-before-cleanup ordering; post-prepare reset is
tested but is not proof of every host interleaving. Broader buffer/rate coverage
and real-host acceptance remain open. The installed VST3 is untouched.

All test targets built. Full local-ROM CTest with desktop access completed in
80.37 s: 11/12 PASS. The sole failure remains the existing #46 expanded-history
fresh-note loss (2092 ms reset, no new audible note). The new reactivation cases
and earlier reset cases pass inside that runner before its known failing case;
the separate four-case reactivation test also passes (0.76 s in the full run).
This is NOT a fully green suite or release acceptance.

Audit follow-ups, including the latest mute-gate finding and the older pending
portamento-save issue, are retained in `docs/archive/AUDIT_TRIAGE_2026-09-23.md`.
