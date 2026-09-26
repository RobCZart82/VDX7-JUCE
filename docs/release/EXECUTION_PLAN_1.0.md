# 1.0 consolidated execution plan — 2026-09-26

Baseline: main `af763f140c648418d9c95de40ea5a3c38dea149e` (#75 merged atop
`174de0491423f99799107d21a552309ac76a9031`). The supplied test-system audit
records 10/10 ROM-free CTest tests on Windows and macOS for its earlier baseline.
This plan combines the original 1.0 host/audio/release gates with the useful
findings F1–F19 and the 2026-09-26 test-system review. Planning is not new
runtime evidence or release authorization. Older roadmap narratives remain
historical records, not instructions to reopen completed GUI work.

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

## Completed infrastructure change — local-ROM CTest failure semantics

- PR #75 merged as `af763f1`; it removes `SKIP_RETURN_CODE 77` from the five
  local-ROM CTest tests.
  Those tests are registered only after an existing ROM path is supplied, so
  an explicit but unloadable fixture must fail instead of becoming SKIPPED.
- A configuration-only check using an existing non-ROM file confirmed that
  the local-ROM tests register without a `SKIP_RETURN_CODE` property. No ROM
  test was executed, and no new Actions result was verified in this update.
  Merge does not replace the product-validation work below.

## 1. Input validation and wheel semantics

- [x] F3/F4 — CONFIRMED validation gaps on baseline main; candidate applied to
  current main `af763f1` as commit `6b929c9`, with live SysEx admission and the
  internal packed import boundary using the shared semantic validator. Added a
  CMake link dependency for the ROM-free live-admission regression. Current
  local `vdx7_ci_checks` build and ROM-free CTest: 10/10 PASS. Public Actions,
  ROM-backed execution, and host acceptance remain pending; reserved-bit
  preservation and transactional failure tests are included.
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
  1500×1156, 1800×1388. The present GUI header pixel test uses 1080/1440/1800
  reference-canvas widths and labels them 75/100/125%; this is not the full
  Settings preset matrix. Add/adjust tests to exercise the actual five selectable
  sizes, including visible control bounds/overlap, editable fields, LCD,
  PERFORMANCE, Settings/About, tooltips, keyboard/footer and host window
  tracking. Include Windows/HiDPI. Preserve the approved appearance.
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

## 5. Test-system audit follow-up

The supplied test-system review found no CMake syntax defect. It reports that
Windows/macOS configured and built the current graph and each ran ten ROM-free
CTest cases successfully. The items below are coverage/robustness work, not
evidence that the shipped instrument currently malfunctions.

### P1 — close misleading-green and important untested paths

- [ ] CTest labels: apply `rom-free` consistently to every ROM-free test so
  `ctest -L rom-free` cannot silently select only a subset. Keep `gui` and other
  useful orthogonal labels where they already apply.
- [ ] Public-CI processor coverage: split the ROM-independent checks at the
  start of `VDX7ProcessorTests.cpp` (editor creation, no-ROM control states,
  hover/render checks) into a ROM-free CTest target. Keep firmware/processor
  integration separate and opt-in; do not leak ROM data into public CI.
- [x] USER-bank semantic corruption: checksum-valid, 7-bit-clean packed voice
  with an invalid semantic field is rejected transactionally (destination
  unchanged); covered by F3/F4 and verified in the local 10-test run.
- [x] Packed-VMEM invalid-field matrix: tests only fields whose accepted ranges
  are confirmed by the format/product contract; checksum-valid invalid imports
  are rejected and destination output remains unchanged.
- [ ] GUI input coverage: add ROM-free component-level tests for pitch spring
  return on release and MOD retention. Scroll/trackpad and host automation
  gesture boundaries remain separate checks; preserve owner-reported REAPER
  evidence and do not call a missing test an observed product defect.

### P2 — make local, release and alternate build paths explicit

- [ ] Give every CTest test an intentional timeout; retain longer per-test
  overrides for soak/lifecycle cases. First inventory expected runtimes to avoid
  flaky limits.
- [ ] Add a no-execution CMake registration smoke (`VDX7_ENABLE_ROM_TESTS=ON`
  with an existing dummy path, then `ctest -N`) to CI or a documented local
  check. It validates CMake test names/fixtures only; it is not ROM acceptance.
- [ ] Compile smoke with `VDX7_RELEASE_BUILD=ON`, with no artifact publication.
- [ ] Exercise supported compile targets: Standalone on Windows/macOS and AU
  on macOS if AU remains supported. Compilation is not host acceptance.
- [ ] Exercise the corresponding-source/offline dependency path using the
  packaged `third_party/` sources with network disabled.
- [ ] Add `pluginval`/VST3 validation as an optional RC gate; retain real REAPER
  and other-host acceptance as separate required evidence.
- [ ] Clarify the local-ROM contract: distinguish firmware-only, combined
  firmware-plus-factory-voices, and the pinned v1.8 fixture. Either declare the
  full suite's exact required fixture or label requirements per test.

### P3 — naming and source-quality polish

- [ ] `vdx7_all_tests` currently builds test executables but does not execute
  them; docs explain that CTest must follow. Consider renaming the target or
  adding a separate build-and-run target without changing current commands
  silently.
- [ ] Consider a consistent warning interface target for project/test sources
  (not third-party JUCE files). Keep `-Werror` out of release-critical builds
  until cross-platform warning cleanliness is established.

### Already dispositioned by the review

- The current CI invocation runs all registered tests with `--no-tests=error`;
  the zero-tests/vacuous-green concern is already guarded.
- Integration executables compiling on public CI is useful, but does not mean
  their ROM-dependent runtime tests ran. Keep compile and firmware-runtime
  status separate.
- The reported 10/10 public CTest result is a ROM-free result, not the full local
  suite, exact-SHA private-ROM acceptance, REAPER acceptance, or release PASS.

## 6. Exact candidate and release — original gates retained

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

Next concrete development order: obtain public Windows/macOS Actions for the
F3/F4 candidate; then reproduce pitch scrolling with the MOD control case.
After those scoped steps, close the remaining P1 coverage items and continue
the original host/audio acceptance matrix. Local ROM-free PASS is not firmware
runtime or host acceptance. No main merge, tag, or release publication is
authorized by this plan.
