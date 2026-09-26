# 1.0 consolidated execution plan — 2026-09-26

Baseline: main `b12bd121cd52c31e9558c4d87df44b197f827c76`.
This plan combines the original release gates with the useful findings F1–F19
from the latest supplied audit. Planning is not new runtime evidence or release
authorization. This is the current execution order; older roadmap narratives
remain historical records, not instructions to reopen completed GUI work.

## Closed implementation and owner-approved scope

- GUI appearance is final: PRs #68/#69, including the recessed keyboard and
  short lower fade. Owner-reported REAPER VST3, Standalone and Retina visual
  checks are retained; Windows/Intel/complete host acceptance is separate.
- Repository presentation and the three owner-supplied screenshots are merged
  in #70. English/Hungarian overviews and detailed guides exist; final package
  instructions and release notes still need candidate-specific review.
- Immutable export snapshots and critical-status priority are FIXED (#67).
  Keep byte-identical export acknowledgement and their regressions.
- Prior detune, bounded reads, factory-CC32 admission, supported keyboard notes,
  direct ROM-reload MIDI boundary and state-save ROM-generation pairing fixes
  stay closed unless a new regression is reproduced. Their final-RC runtime
  acceptance is not implied by merged source.
- Owner-reported pitch/mod automation recording and playback, GUI wheel
  following and pitch drag return-to-centre succeeded. This does not establish
  scroll semantics or every Write/Touch/Latch gesture boundary.

## 1. Input validation and wheel semantics

- [ ] F3/F4 — CONFIRMED validation gaps on baseline main. Reuse local commit
  `79b68afa6818b07d8a139a898a6c6f763c241c58` on
  `codex/audit-validation-gaps`, not a fresh rewrite. Its record reports a
  historical 10/10 local ROM-free run; current-main/public CI acceptance is pending.
  Rebase/apply it, reproduce baseline failures, and verify shared effective-field
  validation for USER load/save, VCED/VMEM, live admission, export and internal
  packed import. Preserve reserved bits and transactional failure behavior.
- [ ] F1 — SOURCE-DERIVED CANDIDATE: reproduce pitch scroll/trackpad nonzero
  retention with a MOD control case. Check keyboard/accessibility input too.
  If confirmed, constrain only unintended GUI input; never reset host automation
  or incoming MIDI pitch bend indiscriminately. Add regression before fixing.
- [ ] F2 — source ordering confirmed, audible/automation defect HOST-DEPENDENT.
  Record actual begin/value/end ordering and centre point in REAPER Write,
  Touch and Latch. Preserve prior owner evidence; no speculative behavior change.

## 2. ROM and project-state integrity

- [ ] F5 — CONFIRMED missing content-identity guard; alternate-ROM runtime
  failure NOT RUN. Test missing saved path with another ROM already loaded,
  changed file contents at the same path, and identical contents at a new path.
  Decide identity scope (firmware and any relevant factory-bank dependency).
  If warranted, add optional content identity plus backward-compatible legacy
  policy; preserve pending RAM on mismatch and accept the matching ROM later.
  Keep this distinct from the already-fixed save-generation/path pairing race.
- [ ] F6 — SOURCE-DERIVED CANDIDATE: brand-new no-ROM instance, host voice edit,
  first ROM load. Compare against existing missing-ROM project-restore tests.
  Decide preserve/apply versus explicit unavailable-edit behavior before fixing.
- [ ] F8/F9 — SOURCE-DERIVED CANDIDATES: establish supported concurrent/reentrant
  state-call contract, then barrier-test whole restore and engine/APVTS lock
  order. Demonstrate a reachable inversion before claiming deadlock. Never
  solve it with an unanalysed audio-thread blocking lock.
- [ ] F10 — SOURCE-DERIVED CANDIDATE: force a Settings Apply failure after
  earlier fields changed; verify whether partial application is observable.
  If reproduced, define atomicity/rollback or explicit partial-result behavior.

## 3. Original audio and DAW acceptance — still required

- [ ] F7 — HOST-DEPENDENT offline contention: reuse existing deterministic
  silent-block characterization, then compare repeated online, offline 1x and
  full-speed renders. Capture hashes, null difference, first differing sample,
  peak/RMS and silent blocks. Equal initial state is essential. If reproduced,
  design a separate offline synchronization contract, not a blind blocking lock.
- [ ] F15 — CHARACTERIZATION: short/slow release, nonzero L4, sustain, pitch
  envelope, stop and region end. Change zero tail metadata only with evidence;
  do not invent an arbitrary fixed tail duration.
- [ ] F16 — CHARACTERIZATION: dense CC/pitch/automation, sustain and Note Off,
  large blocks and contention, then fresh Note On/Off recovery. Keep bounded
  256-event/65536-byte policy; larger capacity alone is not an architectural fix.
- [ ] Full exact-SHA local ROM suite: lifecycle/reset/reactivation, MIDI range,
  timing/stability/stress, ownership/history/overlap retirement, reset overflow,
  deferred partition, portamento, wheel delivery, direct reload, state-ROM
  identity and corrected MONO. Record SHA, OS/compiler, command, rates and result.
  ROM stays local; public ROM-free CI is not firmware-runtime PASS.
- [ ] REAPER Native and Correct MONO: 11 reject, 12 accept, 120 accept, 121 reject;
  both Note Off encodings, repeated notes, sustain, program changes, loop/seek,
  stop/start, bypass and suspension, followed by a fresh supported note.
- [ ] Operator/algorithm/feedback/master/pitch/mod automation; project reopen,
  missing/later ROM, USER/CUSTOM banks, SysEx import/export and dirty/clean state.
- [ ] Windows x64 and macOS REAPER matrix: 44.1/48/96 kHz, 64/128/256/512/1024
  samples where host-configurable; 1/4/8 instances, GUI open/closed, dense edits,
  physical MIDI and device restarts. Intel hardware acceptance or explicit limit.
- [ ] Retain original SRC frequency/aliasing/latency/listening checks, physical
  MIDI timing, audio allocation/locking audit and long-run overload/CPU tests.

## 4. Invisible GUI hardening and build coverage

- [ ] F12 — coverage gap: actual menu sizes 600×463, 900×694, 1200×925,
  1500×1156, 1800×1388. Check applicable visible control bounds/overlap, editable
  fields, LCD, PERFORMANCE, Settings/About, tooltips, keyboard/footer and host
  window tracking. Include Windows/HiDPI. Preserve the approved appearance.
- [ ] F11 — unused runtime image loads confirmed; performance magnitude NOT
  MEASURED. Measure 0/1/4/8 editors (RSS and creation time), remove only proven
  unused loads/resources, compare screenshots and behavior before/after.
  Keep design/reference files where useful; no blanket raster deletion.
- [ ] F13 — reconcile historical raster manifest with vector-first production
  geometry (1440×1110 reference canvas); distinguish reference assets from
  runtime assets and do not promise a nonexistent full 2× pack.
- [ ] F14 — include developer signature in required About vector validation
  and render checks alongside VDX7 and GYR artwork.
- [ ] F18 — add compile/link CI for Standalone on Windows/macOS and AU on macOS
  if retained as supported targets; do not imply host acceptance from compilation.
- [ ] F19 — optional ROM-free ASan/UBSan job for voice/SysEx/USER, deferred MIDI,
  latest display, bounded files, algorithms and status helper.

## 5. Exact candidate and release — original gates retained

- [ ] F17 — document numeric host 1.0.0 versus displayed 1.0.0-dev/RC identity;
  review prototype description/bundle naming without casually changing IDs.
- [ ] Freeze one RC SHA; run public Windows/macOS checks, full local ROM suite,
  host matrix and exact-SHA candidate workflow. Retest relevant gates after fixes.
- [ ] Decide/document single USER-library limitation and supported formats/hosts.
- [ ] Verify package signatures/notarisation status, matching complete source,
  pinned dependencies, licences/notices, checksums, HU/EN installation guidance
  and release notes. Inspect for ROMs, secrets, local paths and build caches.
- [ ] Separate approval before merge/publication as applicable; never bypass
  required checks, auto-tag or auto-publish a stable release from this plan.

## Guardrails and evidence

Preserve 148 parameter IDs/order, plugin identity, legacy project state, Native
default/optional Correct mode, Note 12–120, bounded MIDI, state epochs, USER
compare-before-overwrite and immutable export. No new callback allocation or
filesystem I/O. No ROM in commits or artifacts. No general refactor or GUI redesign.

Each implementation round: exact baseline → failing regression/reproduction →
minimal fix → targeted/full relevant tests → Windows/macOS CI → reviewed merge.
Use CONFIRMED, REPRODUCED, SOURCE-DERIVED CANDIDATE, HOST-DEPENDENT,
CHARACTERIZATION, NOT RUN, FIXED and PASS accurately. PASS requires execution;
source inspection, prior runs and owner reports retain their specific scope.

Next concrete development round: finish the existing packed-voice validation
patch, then reproduce pitch scrolling. This planning update performs neither fix.
