# Release 1.0 audit triage (2026-09-23)

Latest report baseline: main `98d61b8` / Draft #47 `696fe791`; earlier
report baseline `659bc0b` (merged #45). Reports are evidence to check,
not automatic implementation instructions. Preserve firmware fidelity and
existing parameter IDs/state compatibility. Work through reviewed PRs; no tag
or release until separately approved. Never upload ROMs.

## Active reset work

### Current implementation (2026-09-24)

The correction is integrated, persisted and selectable in SETTINGS for the
verified v1.8 image. Native/default Note0 acceptance still fails; do not equate
the corrected path's success with native firmware repair or release acceptance.
See docs/validation/VALIDATION_MONO_SETTINGS.md and VALIDATION_MONO_PERSISTENCE.md.

Desktop processor regression now completes successfully in a separate run.
The historical raw6144-byte restore assertion was replaced by the persistent
contract with exhaustive single-byte negative controls; see
VALIDATION_GUI_RESTORE_ORACLE.md. Prior full-suite counts are not retroactively
changed. No production change in that follow-up.

Subsequent audit findings reproduced and targeted fixes implemented:
- A2 held-note snapshot restoration: VALIDATION_HELD_SNAPSHOT.md.
- A1 ignored CC admission: VALIDATION_IGNORED_CC_ADMISSION.md.
- B1 installation boundary: docs/validation/VALIDATION_STATE_INSTALL_BOUNDARY.md; shared
  collected-input control in VALIDATION_BYPASS_AND_COLLECTED_INPUT.md. Full
  public concurrent state-call/DAW coverage remains open.
- B2 bypass releases: docs/validation/VALIDATION_BYPASS_AND_COLLECTED_INPUT.md; suspended host
  callbacks are not covered by the active-bypass fix.
- B3 wheel rejection/flush: docs/validation/VALIDATION_WHEEL_DELIVERY.md; input delivery and
  newer physical MIDI priority verified, broader scheduling/DAW tests remain.

### Historical stages (not current feature availability)

- Initial MONO engine integration (2026-09-24): a known-image/map-guarded,
  pre-ROM-load opt-in now applies the shared six-site policy in both engine
  stepping paths. Ordinary plugin construction remains native. Actual processor
  tests cover repetition, legato return, audio pitch controls and release.
  At this stage no live switching or persisted/UI option existed; superseded
  by the current implementation above. Native P1 remains open.
  See VALIDATION_MONO_ENGINE_OPTIN.md. Earlier experiment-only status below is
  historical, superseded only within this limited integration scope.

- MONO candidate / gate follow-up: separate test-only branch-decision experiment
  exposes incomplete two-decision and lookup-only proposals; common occupancy
  rules must cover allocation, lookup, release and first/minimum/maximum legato
  scans. Note 0 playback is checked against native POLY pitch/audio and a +12
  patch control that distinguishes zero from one. At this historical stage no
  product correction existed; integration followed subsequently.
  The original unchanged failing production diagnostic is now registered in
  local CTest as `vdx7_mono_note_zero_acceptance` / `release-blocker`, so the
  combined local result no longer hides it behind passing characterization.
  Earlier all-PASS counts below are historical and excluded that gate.
  See VALIDATION_MONO_CANDIDATE.md. Native default/installed plugin unchanged;
  no ROM patch, upload, main merge or release acceptance.
- Portamento intent/O1 follow-up: reproduced display rollback 99 -> 37 and
  immediate-save loss 73 -> 0 before fixing. Engine-owned latest accepted intent
  now survives serial recovery/flush and supplies display/detached state capture;
  native CC5 retains rate computation. Later accepted physical CC5 supersedes it,
  rejected input does not. See docs/validation/VALIDATION_PORTAMENTO_INTENT.md for validation
  and limits. MONO implementation status is described in the current section.
- Q2 publication follow-up: real processor capture/public setter interleaving
  reproduced display 0 after the newer request 99. Guarded single-attempt frame
  publication now preserves concurrent UI edits, including same-value/ABA, with
  no audio retry/lock. Covers PERFORMANCE and tuning; 88 targeted schedules pass.
  See docs/validation/VALIDATION_PERFORMANCE_PUBLICATION.md for scope and validation results.
  Native CC5 pipeline still processes older time values in work RAM, but the
  independent intent follow-up now prevents that from rolling back display/save.
- Q1 follow-up: reproduced false age expiry in the real processor before fixing
  it (64/1024-sample recovery audible; one two-second recovery block silent).
  Successful deferred playback now credits its matching block when checking
  lag; contention/reset-drain callbacks do not. Actual two-second skipped-time
  limit remains. New small/large/mixed partition, threshold and reset-expiry
  regressions pass; see docs/validation/VALIDATION_DEFERRED_PARTITION.md for full validation.
  MONO policy/design boundary is recorded in docs/design/DESIGN_MONO_NOTE_ZERO_POLICY.md;
  corrective development is now authorized; no compatibility mode or firmware
  workaround has yet been implemented or validated.
- MONO instruction/continuation follow-up: the private known image's decoded
  instruction sites match the pinned annotation. Read-only execution observes
  active note-zero slot reuse at D591/D593/D59F, count increment D5A1, and
  zero-return early exit D644/D645 that skips clearing/decrement/EGS Off.
  New unrecovered continuation tests reproduce actual subsequent-note impact:
  one note-zero pair allows pitch 72 but its release leaves nonzero output and
  MONO count 1; sixteen pairs block its native allocation at D58D while the MIDI
  table still accepts it. Stacked and sequential pairs, both Off encodings agree.
  Pitch-one controls release correctly. See VALIDATION_MONO_INSTRUCTION_TRACE.md.
  Failing local execution path now identified; production fix and physical
  hardware confirmation remain OPEN. No automatic workaround/ROM patch.
  Rebuilt local registered suite 21/21 PASS (245.72 s), ROM-free 5/5 PASS
  (0.93 s). Original acceptance diagnostic remains FAIL, not included in 21.
- MONO boundary follow-up: 32 raw-core/processor combinations isolate native
  pitch-zero behavior from wrapper reset and release retirement. Both ordinary
  Note Off and velocity-zero Note On reproduce it; pitches 1/60/127 and POLY
  controls release normally, with 1/16 repetitions. An explicit normal
  POLY-to-MONO mode cycle recovers ownership and allows a fresh audible note
  to release. This is characterization, NOT a production fix or an automatic
  workaround. The original failing diagnostic remains intact. See
  docs/validation/VALIDATION_MONO_BOUNDARY_AND_CI.md; firmware policy, physical-hardware
  confirmation and broader MONO/reset combinations remain open.
  Full rebuilt registered suite 20/20 PASS (239.83 s), fresh ROM-free 5/5 PASS
  (3.05 s); separate original MONO-zero acceptance diagnostic still FAIL.
- R3 lifecycle drain: reproduced on the preceding #47 tree. Maximum release
  history followed by prepare left 16 old firmware MIDI/held entries and the
  immediate fresh note had zero ownership/audio. Lifecycle now shares the
  compact release/completion path with host reset. Bounded non-RT work must
  not discard unfinished releases; remaining cleanup resumes muted in callbacks.
  Six POLY/MONO public-lifecycle/forced-short-drain cases pass locally;
  rebuilt registered suite 18/18 PASS in 223.83 s (separate known-failing
  MONO pitch-zero diagnostic is NOT part of those 18). See
  docs/validation/VALIDATION_RESET_LIFECYCLE_DRAIN.md for exact scope and full-suite status.
- R1 gate/overflow: source ordering is confirmed, but audible leakage was NOT
  reproduced in six real-ROM cases (R4=1/99, L4=0/70/99). Same-offset Note On
  is demonstrably flushed, firmware never owns it, measured leaked peak is zero;
  valid fresh notes subsequently sound/release, including audible nonzero-L4
  positive controls. No speculative gate fix. Other schedules/ROMs remain open.
- Separate newly isolated compatibility edge: v1.8 MONO, 16 repeated MIDI
  pitch-zero On/Off pairs, WITHOUT reset, leave MIDI count 0 but one held
  firmware entry and MONO active count 16. Explicit `--mono-note-zero-only`
  diagnostic fails and is now registered as an explicit local release blocker;
  it must not be called fixed. The annotated native routine uses key zero as an empty-slot
  sentinel. The boundary follow-up above now covers 32 cases; a
  targeted corrective development is approved with native/default preserved,
  but a product implementation remains pending. Lifecycle MONO fixtures
  seed pitch-zero history in POLY, switch normally, then exercise 1..127 in MONO;
  retain the full 128*16 budget and assert empty firmware ownership before holds.
- Per-pitch overlap follow-up: reproduced the global-idle implementation's
  failure with one continuously held anchor and released neighboring history.
  Known-image POLY now protects both firmware ownership tables per pitch and
  retires only completed pitches when all input-stage guards pass. See
  docs/validation/VALIDATION_OVERLAP_RETIREMENT.md for fresh/history pairs and pending-release
  overflow tests. Full rebuilt local suite: 16/16 PASS in 220.72 s. This
  supersedes the idle-only limitation below, not the
  remaining MONO/unknown-ROM/continuous-input/real-host acceptance work.
- Guarded production follow-up: known-image POLY can retire fully completed
  history at the main dispatch boundary during normal playback. Matched idle
  history reset drops from 1512 to 1.333 ms at 48 kHz/64; fresh onset 50.667 ms.
  See VALIDATION_HISTORY_RETIREMENT.md. Original maximum-history tests remain
  on forced conservative fallback. Continuous held ownership, MONO, unknown
  ROMs, broader overload combinations and real-host latency acceptance remain OPEN.
- Firmware ownership follow-up: read-only MIDI/voice table observations and
  queue-stage reset tests added; see VALIDATION_FIRMWARE_OWNERSHIP.md. Adapter
  count zero can precede firmware release; empty MIDI ownership/input under
  sustain can coexist with sustained firmware voices. Both paired-history
  instances have zero entries in the observed tables before reset, yet latency
  differs. Safe completed-dispatch boundary, ROM applicability, MONO/overload
  counterexamples and production retirement remain open. No engine change.
- New paired diagnostic: no-history vs maximum-history instances have matched
  persistent settings, idle input and no adapter ownership; measured reset
  1.333 vs 1512 ms and fresh onset 50.667 vs 1561.333 ms at 48 kHz/64.
  Actual serial queue decoded: 0 vs 2048 Note Offs, 0 vs 4097 bytes. This is
  NOT a proof of identical firmware voice ownership. See
  docs/validation/VALIDATION_RESET_HISTORY_PAIR.md for callback wall timings and limitations.
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
- Reset request/pending flag survived release/prepare: addressed earlier on
  #47 by consuming old requests at lifecycle entry. Four public reactivation
  regressions cover unobserved/observed requests and release/prepare/prepare-only;
  concurrent requests arriving after the exchange are not erased at exit.
- Budget retirement after successful reset may help subsequent resets, but
  cannot alone fix the first expanded-history reset. EGS reconstruction alone
  is not proof that all firmware ownership/serial work is gone.
- Real VST3 wrapper reset, reset/state/ROM overlap, wider rate/buffer matrix,
  multiple instances and callback cost remain acceptance work.

## Other retained work (do not duplicate or silently drop)

- Q1 addressed for false age expiry within fixed queue capacity: real-processor
  reproduction and production fix above. The original component evidence
  (64 prior lag, 1500x64 deliver vs one 96000 dropping both) is retained as
  historical evidence. Actual DAW offline acceptance and separate event-capacity
  limits remain; not a full-plugin sanitizer or general partition-invariance claim.
- Q2 addressed for stale pre-captured display publication over newer UI edits:
  deterministic production-path failure before guard, then 88 passing schedules
  and a public-CI ROM-free concurrency regression. Not a permanent-engine-data
  loss fix, state-epoch fix, or guarantee against every native firmware transient.
- Portamento-time display/save: independently reproduced after Q2 and addressed
  by an accepted-intent model, not by guessing native acknowledgement from a
  value match or waiting longer in the UI/audio callback. Actual firmware time
  and derived rate are checked separately against direct native serial input.
  See VALIDATION_PORTAMENTO_INTENT.md. Native serial latency still exists.
- Bypass: missing explicit MIDI/lifecycle handling; reproduce release/sustain
  and controller state before implementing a shared muted processing path.
- Restore: both staging-before-install and old-epoch-before-lock windows need
  deterministic production-path tests. A lone extra atomic load is insufficient.
- Pitch/mod: rejected or subsequently flushed submission must not be cached as
  delivered. Preserve newer physical MIDI over stale GUI/automation snapshots.
- O1 immediate save during recovery: reproduced 73 -> 0 loss, now addressed by
  engine acceptance independent of FIFO availability and detached RAM snapshot
  projection. Includes immediate restore/resave and missing-ROM preservation.
  This does not close the separately tracked state-install/epoch races.
- Dirty-bank text must not hide critical edit-overflow/recovery instructions.
- Tail metadata: zero is not generally consistent with release/nonzero L4;
  measure actual host behavior, do not invent a fixed tail duration.
- POLY/MONO: include unsuccessful lock-held transactions in timing statistics.
- T1 addressed: the shared public-CI target now compiles all six integration
  executables, including the four previously omitted runners (processor,
  stability, MIDI-range and timing). Execution still requires explicit local
  ROM opt-in; public CI does not distribute or execute firmware.
- T2 addressed for the host-reset/ownership suite: `firmware-v1_8` label and
  required profile fixture distinguish its known-image assumptions. Wrong-image
  prerequisites fail explicitly, not silent PASS/SKIP. Unknown-ROM conservative
  fallback remains intentional; actual unknown-ROM runtime is still unvalidated.
- Allocation probes cover ordinary C++ allocation on the observed thread, not
  every allocator or a complete hard-realtime proof.
- Exact-SHA RC workflow, full local-ROM and actual macOS/Windows host acceptance
  remain release gates. Green ROM-free CI does not imply ROM-runtime PASS.

## Already addressed; preserve regressions

Base reset implementation (#44), fresh-note release tests (#45), malformed live
SysEx admission, live-bank master tuning, explicit ROM fixtures, program/edit
ordering and earlier publication fixes. Restore epoch exists but its concurrency
acceptance is not complete. Final GUI/Retina work follows correctness gates.
