# Release 1.0 audit triage (2026-09-23)

Latest report baseline: `659bc0b` (merged #45). Reports are evidence to check,
not automatic implementation instructions. Preserve firmware fidelity and
existing parameter IDs/state compatibility. Work through reviewed PRs; no tag
or release until separately approved. Never upload ROMs.

## Active reset work

- New paired diagnostic: no-history vs maximum-history instances have matched
  persistent settings, idle input and no adapter ownership; measured reset
  1.333 vs 1512 ms and fresh onset 50.667 vs 1561.333 ms at 48 kHz/64.
  Actual serial queue decoded: 0 vs 2048 Note Offs, 0 vs 4097 bytes. This is
  NOT a proof of identical firmware voice ownership. See
  VALIDATION_RESET_HISTORY_PAIR.md for callback wall timings and limitations.
  P1 remains open: prove retirement during normal processing, then add
  queue-stage/sustain/repeat/overload counterexamples and a numeric latency
  acceptance target independent of the two-second fail-safe.
- #46: expanded lifetime release history demonstrably exceeds the 2-second
  deferred age limit. At 48 kHz/64, 128 pitches x16 yields 2092 ms cleanup and
  loses an immediate fresh note. Originally a Draft failing regression, merged
  as #46 into `98d61b8`; it remains a failing acceptance test, NOT a completed fix.
  Raising the global limit to 3 seconds was diagnostic only and was reverted.
  Follow-up on #47: running-status release encoding makes the unchanged loss
  assertion pass across 44.1/48/96 kHz x64/128/256/512; full local suite 12/12.
  See VALIDATION_RESET_RUNNING_STATUS.md. Worst-case reset is still about 1.5 s;
  responsiveness and real-host acceptance remain open, not release-ready.
- NEW: reset request/pending flag survives release/prepare. Reproduce both an
  unobserved request and a request already observed by an audio callback. Full
  device lifecycle should retire old requests without erasing a newer reset.
- NEW: fresh Note On can unmute reset output before a later overload flushes
  that note. Source ordering confirmed; audible consequence still NOT RUN.
  Use nonzero L4 and a same-sample burst, then a valid post-recovery note.
- Budget retirement after successful reset may help subsequent resets, but
  cannot alone fix the first expanded-history reset. EGS reconstruction alone
  is not proof that all firmware ownership/serial work is gone.
- Real VST3 wrapper reset, reset/state/ROM overlap, wider rate/buffer matrix,
  multiple instances and callback cost remain acceptance work.

## Other retained work (do not duplicate or silently drop)

- Bypass: missing explicit MIDI/lifecycle handling; reproduce release/sustain
  and controller state before implementing a shared muted processing path.
- Restore: both staging-before-install and old-epoch-before-lock windows need
  deterministic production-path tests. A lone extra atomic load is insufficient.
- Pitch/mod: rejected or subsequently flushed submission must not be cached as
  delivered. Preserve newer physical MIDI over stale GUI/automation snapshots.
- O1 immediate save during recovery: preserve accepted pending portamento time.
  This was omitted from the latest report, NOT demonstrated fixed.
- Dirty-bank text must not hide critical edit-overflow/recovery instructions.
- Tail metadata: zero is not generally consistent with release/nonzero L4;
  measure actual host behavior, do not invent a fixed tail duration.
- POLY/MONO: include unsuccessful lock-held transactions in timing statistics.
- Public CI: compile all integration executables without distributing firmware.
- Allocation probes cover ordinary C++ allocation on the observed thread, not
  every allocator or a complete hard-realtime proof.
- Exact-SHA RC workflow, full local-ROM and actual macOS/Windows host acceptance
  remain release gates. Green ROM-free CI does not imply ROM-runtime PASS.

## Already addressed; preserve regressions

Base reset implementation (#44), fresh-note release tests (#45), malformed live
SysEx admission, live-bank master tuning, explicit ROM fixtures, program/edit
ordering and earlier publication fixes. Restore epoch exists but its concurrency
acceptance is not complete. Final GUI/Retina work follows correctness gates.
