# VDX7 1.0.0 release gate

Target: stable 1.0.0, not another public pre-beta. Work-in-progress builds are
not final releases. Do not publish or replace existing release assets until
the acceptance checklist is complete. Keep plugin IDs and existing parameter
indices compatible with saved projects.

## Current execution authority — 2026-09-26

Use [the consolidated 1.0 execution plan](EXECUTION_PLAN_1.0.md) for current
priority, closed GUI scope, audit F1–F19 disposition, and test-system follow-up
against main `af763f1` (#75 merged atop #74). It preserves the original release gates
and local unmerged validation work.
Older dated sections below are historical evidence; their stale GUI checkboxes
or "next chapter" wording must not reopen the approved design. The RC checklist
remains the final release gate, not a publication authorization.

## Stabilization

### Historical next steps — 2026-09-25 (superseded by consolidated plan)

Latest checkpoint: PR #63 was merged to `main` as `cbbd213` after macOS and
Windows Actions passed. It adds the approved three header accent lines and a
ROM-free pixel regression at 75/100/125% editor sizes. The operator-row
separator was confirmed already implemented and its stale unchecked item was
corrected in that PR. Four additional ROM-free source-level test executables
were manually compiled and passed on source commit `3d6218797a1af2fd09e8484fe0b42ae2f3f956e4`;
see `docs/validation/VALIDATION_1.0_ROM_FREE_TESTS_2026-09-25.md`. This was not a CMake/CTest,
plugin, GUI-host or firmware-backed run.
Private-ROM acceptance remains a separate gate.
`docs/validation/VALIDATION_MONO_SOAK.md` records the full 30-test run, separate desktop retry,
dual-instance soak and remaining host boundaries. The user approved retaining
both Native and Correct modes. Native remains the recommended default; Correct
is an advanced compatibility setting. No 1.0.0 publication is authorized.

New product input policy (2026-09-24): filter MIDI Note events outside the
inclusive 12–120 range in both MONO modes. This prevents the observed native
Note 0 lockup and excludes the unused upper tail by product choice without
transposing pitches or patching firmware. Processor/ROM-free boundary tests were
added and macOS/Windows CI passed on the merged head. Full local ROM-enabled
suite and targeted REAPER tests at Notes 11/12 and 120/121 remain outstanding,
including release, sustain, transport and subsequent normal-register notes.

1. Complete the local ROM-enabled regression and exact-boundary REAPER acceptance.
   Keep raw-firmware Note 0 characterization distinct from product range
   acceptance; report unavailable desktop prerequisites as NOT RUN.
2. Broaden concurrent public restore, mixed controller/overflow timing and host
   bypass/suspension coverage. Desktop GUI/processor follow-up now passes;
   see docs/validation/VALIDATION_GUI_RESTORE_ORACLE.md for the persistent-state oracle change.
3. Real DAW acceptance, remaining GUI finish/scale checks and release packaging.
   PR #47 is merged; publishing remains a separate approval.

### Current status authority

The milestone narratives further down this file were written over several
weeks. Their unchecked boxes are not all current: some describe work later
implemented in source or covered by a validation report. Use this section,
`docs/release/RELEASE_CHECKLIST_1.0_RC.md`, and the linked validation records as the current
release status; reconcile historical checkboxes instead of treating every old
unchecked item as an unfinished feature.

Implemented in the current source; release acceptance is separate:
- Native and advanced Correct MONO settings remain available; both filter
  Note On/Off outside inclusive Notes 12–120 before deferred/engine delivery.
- PERFORMANCE/SETTINGS controls, project persistence, USER-bank Save As,
  LCD bank/program navigation, numbered algorithm selection and the three GUI
  views are implemented. Multiple named USER banks are not supported; decide
  whether to ship that as a documented 1.0 limitation.
- The LCD-style previous/next arrows, exact `VDX7 Mk 1.` vector wordmark,
  two-line header copy, common lower alignment, green/yellow/red meters, operator
  row divider and three section-tone header accents exist in the current source.
  The last two details were confirmed/added in PR #63.

Still open and release-relevant:
- Run the complete ROM-free and opt-in v1.8 ROM CTest suites on one exact
  release-candidate SHA, preserving logs and separate known-firmware
  characterization from product acceptance.
- Perform REAPER Note 11/12 and 120/121 boundary tests in both Settings modes,
  including every Note Off encoding, sustain, repeated notes, transport/loop,
  and a subsequent supported note. Then run the full host/platform matrix below.
- Complete real GUI/audio lock-overlap, suspension/bypass, allocation and
  deferred-MIDI timing acceptance; test SRC frequency/aliasing/latency and
  measure offline-render release tails before deciding whether zero host-tail
  metadata needs a change. No release-tail truncation is currently confirmed.
- Finish whole-interface visual/HiDPI acceptance and package/legal/version
  gates. The current four manually compiled component tests and green PR builds
  are not substitutes for these items.

### Audit follow-up — 2026-09-24 (reviewed against current main)

The audit package `VDX7_AUDIT_WORK_30a3ccbd.zip` examined main
`30a3ccbd` immediately before PR #48. PR #48 changed documentation and editor
wording only; PR #49 added roadmap documentation only. Since the audit, PR #50 corrected the reset-history test oracle and PR #52 fixed the malformed detune round-trip at component level. The audit's other processor, MIDI validation, keyboard queue and SysEx findings remain subject to their stated evidence limits on current main `675b553`. The audit's Linux component/model runs are useful
evidence, but are not a full JUCE, firmware or DAW acceptance run. Preserve each
finding's evidence class; do not promote a model result into a product pass.

Audit disposition refreshed against main `176b757` on 2026-09-25:
- N1 reset-history oracle correction merged in PR #50 (`675b553`). The ROM-enabled reset-history/full CTest acceptance remains NOT RUN; the merge fixes the test expectation, not a product audio defect.
- U2 malformed checksum-valid detune round-trip was fixed in PR #52 (`cc2f4aa`); PR #57 (`44ea97a`) extends the same validation to live bulk admission and single/bank export, with all 192 voice/operator slots covered by sanitizer tests. ROM-backed processor-path acceptance remains NOT RUN. Keep the finding scoped to malformed input; do not generalize it to ordinary factory patches or call it a checksum defect.
- T1 processor-boundary characterization and Note 127 guard-bypass sensitivity control merged in PR #53 (`7bb7af9`); synchronized macOS/Windows CI passed. The ROM-enabled characterization itself remains NOT RUN.
- U1 direct ROM reload/deferred-MIDI boundary fix and opt-in processor regression merged in PR #54 (`9d57069`); macOS/Windows Actions passed on the merge commit. The local v1.8 ROM regression and full ROM-enabled suite remain NOT RUN, so runtime acceptance is pending. N2 state/ROM identity mixing remains an unconfirmed processor-level candidate. U3's conditional CC32 admission fix and paired processor regression merged in PR #58 (`312a221`); ROM-backed execution remains pending.
- N6 deferred-event capacity/partition characterization merged in PR #59 (`176b757`). Its opt-in processor test is present; local ROM-backed execution remains NOT RUN. It documents the bounded queue policy and does not justify increasing capacity.
- N3 bounded ROM/SysEx file reading merged in PR #60 (`f57b072`) with a
  ROM-free regression; local CMake/CTest execution remains NOT RUN.
- N4 public keyboard-queue admission now filters unsupported programmatic
  keyboard events before queue capacity is consumed. See
  `docs/validation/VALIDATION_1.0_KEYBOARD_RANGE_ADMISSION.md`; PR #61 macOS/Windows CI passed
  and the change is merged in main `210b9b9`. ROM-backed processor acceptance
  remains NOT RUN.
- N2 save/ROM identity: the engine-generation-bound path snapshot and
  deterministic opt-in processor interleaving regression merged in PR #62
  (`9d1970a`). Local ROM-backed execution is still NOT RUN here
  (CMake/CTest unavailable); run it with the complete local ROM suite before
  declaring runtime acceptance.
- MIDI Note 12–120 policy is consistent in product admission; internal 0–127
  cleanup/release loops are intentional. ROM-backed and DAW acceptance items
  remain NOT RUN unless separately recorded in their validation reports.
- N5 — host tail metadata remains a P2 validation candidate: `getTailLengthSeconds()`
  still returns 0.0 although voices can release after Note Off. Verify JUCE/host
  offline-render tail behavior and add a Note-Off release-tail regression before
  choosing a conservative non-zero or dynamic estimate. No host truncation has
  been reproduced yet; do not describe it as a confirmed audible defect.

### Audit disposition refreshed against main `b849285` — 2026-09-25

The two highest-priority production findings in the supplied deep audit are
already fixed in PR #67; do not reopen them without a new reproducer:

- **Export dialog stale-content — FIXED.** `chooseExport()` captures an
  immutable patch/bank SysEx snapshot before opening the asynchronous file
  chooser, then writes that exact snapshot. `VDX7ProcessorTests` models a host
  parameter edit and audio flush during the chooser interval for both patch and
  bank export, verifies the exported bytes remain the captured bytes, and keeps
  the subsequent edit marked unexported.
- **Critical status hidden by dirty-bank text — FIXED.** The editor delegates
  status selection to `VDX7StatusPresentation::choose()`, which gives critical
  queue-overflow/MIDI-recovery messages precedence over dirty-bank and ordinary
  informational text. `VDX7StatusPriorityTests` covers queue overflow with
  dirty and clean banks, MIDI overload with a dirty bank, and both fallback
  presentation cases.

Remaining items from that audit are validation candidates, not confirmed
production defects:

- **Pitch-wheel spring-return automation — HOST-DEPENDENT / NOT RUN here.**
  Verify Write, Touch and Latch gesture recording in REAPER before changing the
  JUCE mouse-up/parameter-notification order.
- **Offline-render engine-lock contention — SOURCE-DERIVED CANDIDATE.** The
  deterministic processor fixture asserts that a callback which misses
  `engineMutex_` returns a silent block (3 × 256 samples, 16 ms at 48 kHz).
  That is the deliberate realtime non-blocking behavior; it does not establish
  that an offline REAPER render actually encounters contention or becomes
  nondeterministic. Compare repeated offline renders and a deterministic
  contention fixture before considering a distinct offline synchronization
  contract. Do not replace the try-lock with a blocking lock without a deadlock
  analysis.
- **Deferred-MIDI partition sensitivity — CHARACTERIZED.** PR #59 records the
  bounded queue behavior; measure realistic host pressure before proposing an
  architectural change or larger capacity.
- **Tail metadata — P2 VALIDATION CANDIDATE.** `getTailLengthSeconds()` remains
  zero. Measure actual release/render-tail behavior in REAPER before changing
  the host metadata; no truncation has been reproduced.

This disposition is a source/test review of the merged main tree, not a new
local build, CTest, REAPER automation run, or offline-render acceptance run.

### Targeted audit reproduction — 2026-09-25 (based on 7bb7af9)

The supplied `VDX7_audit_repro_7bb7af95.zip` is a characterization probe, not
an acceptance suite: exit code 0 means the baseline observations (including
undesirable behavior) reproduced. Its six recorded source blob hashes match
current main for `VDX7DeferredMidi.h`, `VDX7MidiValidation.h`,
`VDX7Sysex.cpp/.h`, and `VDX7VoiceData.cpp/.h`. I independently compiled and
ran the probe against the current source with Apple Clang 21 + ASan/UBSan; all
stated observations reproduced, and no sanitizer diagnostic occurred. This was
not a JUCE/plugin, firmware, DAW, or full CTest run.

- **U2 live-path follow-up — targeted code/tests added; processor acceptance pending.**
  PR #52 rejects invalid detune nibble 15 during file decode, but the live-bank
  validator checked framing/checksum only, and the encoder could emit malformed
  packed input. This follow-up shares a packed-voice detune validator across
  live admission and SysEx decode/encode. Standalone sanitizer tests reject all
  192 voice/operator placements at both live admission and bank export, plus
  malformed single-voice export; valid round-trip controls pass. The actual
  processor live-bank path still needs an integration regression. Keep this
  distinct from the already-merged file-import fix; no firmware or factory-
  patch defect is implied.
- **N6 deferred-event capacity — processor regression added; ROM execution pending.**
  The component probe showed that 257 CC events after a 64-sample lag overflow
  when batched into one 16,448-sample callback, while time-partitioned rendering
  drains them successfully. An opt-in processor test now compares this same
  time-spaced stream after actual mutex contention: an over-capacity callback
  must drop the batch atomically, release an already-held note, avoid stale
  replay, and accept fresh MIDI afterward; 257 successful 64-sample callbacks
  must deliver every CC in order without panic. This characterizes the bounded
  256-event safety policy and partition sensitivity; it does not yet establish
  a product defect or justify changing the capacity. The ROM-backed processor
  test is added but not run locally.
- **U3 — conditional CC32 deferred pressure — fix and paired processor regression added.**
  The probe admitted CC32 bank values 0–7 without factory-image context; 256
  such messages plus one supported note panicked the deferred queue. Host-event
  admission now consults the processor's atomic factory-image availability
  snapshot, so impossible CC32 requests are discarded before consuming delayed
  capacity; engine-side rejection remains as defense in depth. A ROM-backed
  processor regression compares the no-factory 256-request flood against a
  factory-present 255-request control, each followed by a note, under actual
  engine-lock contention. The standalone validator control passes locally;
  the ROM-backed processor test is added but not run here, so end-to-end
  acceptance remains pending.

Controls in the probe passed: all 15 valid detune nibbles and ordinary bank
round-trip; 12,288 note/channel/status/velocity combinations; 10,000 seeded
malformed-message probes. The original Linux Clang/GCC runs and this independent
macOS sanitizer run are component evidence only.

1. **N1 — reset-history oracle correction merged (PR #50, `675b553`).**
   The test now sends the full 0–127 proposal through the public processor but
   expects only the 109 admitted notes (12–120), with the documented release
   counts and decoded pitches. The code/test correction is merged; run
   `vdx7_reset_history_pair` and the complete local ROM-enabled CTest suite on
   one recorded SHA before treating the local acceptance as complete. Those ROM
   runs remain NOT RUN.

2. **T1 — production-filter characterization merged (PR #53, `7bb7af9`).**
   Excluded MIDI events now enter the real processor in continuation and
   boundary histories, with a direct-engine Note 127 sensitivity control.
   Synchronized macOS/Windows CI passed. The local ROM-enabled characterization
   remains NOT RUN; preserve it as a validation item, not an unimplemented code change.

3. **N2 — save/ROM identity generation consistency.** A deterministic
   processor-level save-versus-ROM-load interleaving now checks that the saved
   ROM path belongs to the same engine generation as the detached RAM snapshot.
   The processor records the successfully installed image path under
   `engineMutex_` and captures it with RAM; it does not extend the lock over
   XML/base64 work. The ROM-backed regression must still run locally before
   calling this candidate runtime-confirmed or accepted. Keep the earlier
   protocol-model evidence distinct from processor execution.

4. **U1 — direct ROM reload / deferred MIDI fix merged (PR #54, `9d57069`).**
   A processor regression creates real engine-lock contention with deferred
   input, performs a successful direct ROM reload, checks stale input is dropped,
   and verifies a fresh Note 72 plays and releases. macOS/Windows Actions passed
   on the merge commit. Local v1.8 ROM execution and the full ROM suite remain
   NOT RUN; keep runtime acceptance open and distinct from project-state restore.

5. **U2 — packed-detune malformed-input fix merged (PR #52, `cc2f4aa`); live/export
   follow-up prepared.** The current follow-up rejects nibble 15 in live bank
   admission and in single-voice/bank SysEx encoding, with all 192 slots covered
   by standalone sanitizer tests. Full processor-route acceptance remains open.
   Keep the finding scoped to malformed input, not ordinary factory patches or
   checksum correctness.

6. **U3 — conditional CC32 deferred admission — targeted fix and processor test added.**
   The shared host validator now accepts factory-bank CC32 only when the
   processor's atomic capability snapshot says factory voices are available.
   This avoids taking the engine lock in the audio input path; the engine keeps
   its existing guard. The ROM-backed regression tests 256 filtered CC32
   requests plus a note without factory voices, and 255 valid CC32 requests
   plus a note with factory voices, both under real mutex contention. Local
   full-ROM execution remains pending.

7. **Lower-priority hardening:** N3 bounded import reads merged in PR #60;
   processor-level failed-import preservation remains to verify with ROM.
   N4 pre-admission for the programmatic keyboard API merged in PR #61
   (`210b9b9`). Keep it scoped to that API: the visible keyboard is 36–96 and
   normal host MIDI is already filtered. Neither item is a reported normal-GUI
   crash.

8. **Release validation:** after fixes, run the full local suite on the final
   source SHA and retain separate test logs. Recheck REAPER boundaries 11/12 and
   120/121 in both modes, including release, sustain, transport and a subsequent
   supported-register note. CI-green, prior general-use REAPER testing and the
   audit's component probes are not substitutes for these exact boundary and
   ROM-enabled checks. Report unavailable Windows/Intel/physical-MIDI coverage
   as NOT RUN, not PASS.

The current product decisions remain unchanged: both modes accept Notes 12–120;
Native firmware is the recommended default; Correct MONO Note 0 remains an
advanced compatibility option. The raw Note 0 firmware limitation remains
distinct from product-range behavior. No merge, tag or 1.0.0 publication is
authorized by this audit follow-up.

The feature history below records earlier milestones, not the current execution
order. Current audit disposition is in AUDIT_TRIAGE_2026-09-23.md.

- [x] Priority bug fix: reject unsupported CC32 bank values instead of modulo-8
  wrapping. Only a successful accepted factory-bank load may clear unexported
  voice markers or publish a bank-change result. Test 0/7 and ignored 8/15/127,
  preserving patch RAM and dirty flags in edited CUSTOM/USER working copies.
  See `docs/validation/VALIDATION_1.0_BANK_SELECT.md` for implementation and local test scope.
  Existing CC0-ignore / immediate CC32 behavior remains unchanged; a different
  MSB/LSB policy needs an explicit compatibility decision and host acceptance.
- [x] Follow-up static-review validation: malformed MIDI status/length/data-byte
  rejection, system-common handling, checksum-valid live bank SysEx admission,
  and keyboard UI-held state after overflow. Malformed SysEx is rejected before
  it can exhaust deferred event/byte storage; the engine shares the admission
  validator. The processor integration renderer requires its explicit local ROM
  fixture. See `docs/validation/VALIDATION_1.0_MIDI_INPUT.md`.
  Remaining acceptance: live-host bulk-SysEx stress and broader UI contention.
  Historical review note: keyboard UI-held state after overflow was reproduced.
  Reproduce before claiming stuck audio; mirroring suppresses MIDI feedback and
  UI note-off already clears held state before queue insertion. Keep mixed host
  MIDI/automation ordering and independent SRC/PDC acceptance in the test scope.
  Review of report dated 2026-09-20: missing play methods used mixed revisions;
  APVTS ValueTree stores denormalised values, so do NOT apply its suggested
  XML normalisation. Existing ordered-edit and impulse tests must be retained.

Next chapter order: verified CC32 fix -> complete host-MIDI and live-SysEx
admission validation -> SETTINGS channel/tuning -> remaining GUI visual
integration and host acceptance. Reclassify reported risks as confirmed bugs only
with source evidence or reproduction; do not apply the rejected APVTS
normalisation or remove implemented play controls. PR/CI/user-merge gates remain.

- [x] Shared complete channel-message validation before engine/GUI mutation and
  deferred storage; ROM-free exhaustive status/length/data-byte tests and local
  integration regressions. Unsupported system common/realtime traffic is ignored;
  validated bank SysEx stays on its dedicated path. See `docs/validation/VALIDATION_1.0_MIDI_INPUT.md`.
- [x] Reproduce the reported overflow + lost UI Note Off scenario: local test
  confirms release and no mirror-triggered note. No speculative UI reset applied.
  Broader simultaneous-held-key/host-contention acceptance remains open.

- [x] Deferred multi-block MIDI timeline, restart cleanup and consistent program
  normalization implemented; see `docs/validation/VALIDATION_1.0_MIDI_LIFECYCLE.md` for policy
  and local tests. Actual host transport acceptance remains open.
  Q1 follow-up corrects false age expiry in large successful callbacks, with
  real-processor before/after reproduction, waveform comparison and true-delay
  expiry tests. The two-second actual-lag limit remains; see
  `docs/validation/VALIDATION_DEFERRED_PARTITION.md`. Queue-capacity and real-host limits remain.

- [x] Ordered program/bank/voice/operator edit queue with local save/audio
  regressions; see `docs/validation/VALIDATION_1.0_EDIT_ORDER.md` for scope and overload policy.

- [x] Missing-ROM deferred project restore, including re-save/restart (local automated tests).
- [x] Valid project-state restore creates an audio-owned MIDI timeline boundary:
  it releases old notes and discards host/UI events deferred before the restore,
  without the state thread mutating audio-owned storage. Local lock-contention
  regression covers one sounding note plus one deferred Note On.
- [x] Invalid-ROM rejection preserves RAM and pending UI edits; firmware-only reload tested.
- [x] CC64/65 thresholds and CC11 expression regression tests.
- [x] Preserve MIDI note-offs during engine transactions; bounded overflow recovery tested.
- [x] Validated live bank SysEx with coherent GUI/host state (local tests).
- [x] Preserve master tuning across validated live 32-voice bank SysEx import.
  The bank contains voice RAM only, so an import must retain the separate
  project-level firmware tuning value. Engine regression covers -256, -1, 0,
  +1 and +255; live-host bulk-SysEx acceptance remains open.
- [x] Remove 512-sample lookahead; measure onset and host-block invariance.
- [ ] Physical MIDI timing, dense chords and automation acceptance in hosts.
- [x] Move direct voice-parameter notifications outside the audio callback and
  engine lock; editorless/reentrant state tests (see `docs/validation/VALIDATION_1.0_HOST_PUBLICATION.md`).
- [x] Coalesce frequent non-disruptive PERFORMANCE/SETTINGS writes outside
  `engineMutex_`: controller range/assignments, pitch-bend range/step, master
  tuning, portamento mode, glissando and bounded portamento time. A held-lock regression and a
  concurrent 1,000-write/audio test confirm prompt UI snapshots, save-time
  commit and zero measured contention from this path. See
  `docs/validation/VALIDATION_1.0_CONTENTION.md`.
- [ ] Keep the deliberate POLY/MONO firmware transaction in the real
  GUI/audio-overlap audit. It must retain its firmware reset and note-release
  behavior; do not move it to the callback without a bounded transaction design
  and host evidence.
- [ ] Complete remaining audio-thread allocation/locking audit and contention stress test.
- [x] Guard firmware serial/controller saturation; validate recovery with a
  60-second simulated load and callback ordinary-C++ heap probe
  (`docs/validation/VALIDATION_1.0_MIDI_OVERLOAD.md`). Direct-C/aligned heap profiling and live
  host soak acceptance remain open; this is not a hard-realtime guarantee.
- [x] Remove keyboard-state locking/listeners from audio; bounded UI MIDI handoff
  and 12-configuration local stress grid (see `docs/validation/VALIDATION_1.0_REALTIME_BASELINE.md`).
  Full allocation/overload audit and host acceptance are still open.
- [x] Q2 stale PERFORMANCE/tuning display publication: guard the commit against
  newer UI edits (including same-value/ABA) without audio waiting/retry. Real
  processor before/after reproduction and public-CI helper regression; see
  `docs/validation/VALIDATION_PERFORMANCE_PUBLICATION.md`. Restore/write epochs remain separate
  open work; the portamento follow-up below is independently reproduced/tested.
- [x] Retain the latest accepted portamento setting separately from transient
  firmware work RAM. Immediate save during recovery, queued CC5 display rollback,
  flushed-command replay and later accepted physical CC5 are covered by
  `docs/validation/VALIDATION_PORTAMENTO_INTENT.md`. Native CC5 still computes the actual rate;
  there is no synthetic acknowledgement or extra callback render budget.
- [x] Historical full 0–127 note range and pitch/release regressions (local ROM).
  Superseding product policy (2026-09-24): filter Note On/Off outside 12–120
  before deferred storage and engine delivery in both modes; 12/120 are inclusive.
  Processor and deferred-queue tests cover boundaries; actual REAPER acceptance
  remains a release check. This is an explicit product range limit, not a claim
  that the raw firmware issue was fixed.
  Scope correction (2026-09-23): this is not exhaustive POLY/MONO coverage.
  Native v1.8 MONO pitch 0 retains ownership after release in the raw emulator
  and processor. The new 32-case boundary characterization documents it; the
  original failing diagnostic is preserved, not counted as a passing release
  test. Firmware-fidelity policy and broader MONO/reset acceptance remain open.
  See `docs/validation/VALIDATION_MONO_BOUNDARY_AND_CI.md` and `docs/archive/AUDIT_TRIAGE_2026-09-23.md`.
  Follow-up: local instruction trace now identifies the failing allocation and
  release branches. Subsequent-note loss/retained output without recovery is
  reproduced, including sequential On/Off pairs and both Off encodings. This is
  diagnosis/test coverage, not a production fix or hardware confirmation; see
  `docs/validation/VALIDATION_MONO_INSTRUCTION_TRACE.md`.
  Native/corrected-path options and required acceptance are separated in
  `docs/design/DESIGN_MONO_NOTE_ZERO_POLICY.md`. Targeted corrective development is now
  approved. The earlier engine-only stage is superseded: the optional correction
  is integrated, persisted and selectable in SETTINGS. Default firmware fidelity
  is unchanged; full corrected-mode product acceptance remains open.
- [x] Prevent the observed Note 0 MONO lockup at the product boundary by filtering
  Note 0–11 and 121–127 in both modes; retain raw firmware characterization.
  The selectable correction, project persistence and candidate evidence remain
  separately documented in `VALIDATION_MONO_*`. Reopen only for a reproduced
  bypass or regression; broader corrected-mode host acceptance remains.
  REAPER validation at Note 11/12 and 120/121 is still required before release.
- [x] Configure PR ROM-free CI and opt-in local ROM integration CTest gate.
  All integration runners now compile through the shared CI target. Known-v1.8
  ownership groups have a separately labelled, required firmware prerequisite.
  The current CMake configuration registers ten ROM-free CTest cases; CI build
  success does not establish that the private-ROM cases ran.
- [x] Run the development CI configuration on GitHub: macOS and Windows passed
  on `5164fabd36c8fdd745e272fc1f493c0c352c1ced`. Every subsequent change needs
  fresh checks; this does not constitute RC or host acceptance.

## Features and quality

User decision (2026-09-20): milestones through 4 may be developed, with PR/CI
gates between chapters. Milestone 4 uses firmware-faithful behavior as its base;
modern extensions require a separate decision. Subsequent user approval extends
development to the agreed GUI/function integration and Save As workflow below.
Release publication remains outside this authorization. Each chapter still needs
fresh PR checks and user merge before the next chapter.

- [x] PERFORMANCE/SETTINGS scope implemented with project-state and compatibility
      regressions: pitch-bend range/step, controller assignments, MIDI channel
      filtering and master tuning. Physical host/controller acceptance remains open.
- [x] Product scope decision: Native firmware is the recommended default and
      Correct MONO Note 0 remains an advanced option; both modes support only
      Notes 12–120. Firmware/REAPER acceptance is still open.
- [ ] Measure resampling/aliasing; quality implementation accepted against references.
  Milestone 3B adds band-limited SRC, selected spectral limits, exact reported
  latency and CPU diagnostics (`docs/validation/VALIDATION_1.0_BANDLIMITED_SRC.md`). Physical
  reference/host listening and latency-compensation acceptance remain open.
- [ ] Current documentation matches all shipping features and limitations.

## Agreed GUI direction — 2026-09-20 (partly implemented; acceptance pending)

First implementation pass: warm enclosure/panels, yellow-green LCD, matte knobs,
restrained active buttons, ON/OFF performance switches, GYR mark in ABOUT and
green/yellow/red meters. See `docs/validation/VALIDATION_1.0_WARM_GUI.md`. Remaining checklist
items below still require final visual and host acceptance; this is not a final skin.

Second visual slice groups PERFORMANCE into PLAY MODE, PITCH BEND and PORTAMENTO
cards, retaining every firmware-backed choice and the four controller panels.
Short field labels and grouped control bounds are checked at three editor sizes.

The three GUI visual concepts remain layout references, not a finished skin.
The approved hardware-inspired material/colour direction is specified below.
The requirements below remain the design contract. Implementation checkboxes
are updated where verified in source; real-host and HiDPI acceptance remains
separate.

- [x] Make the LCD the single bank/patch navigation centre: bank selector,
  direct program selector, current program number/name and modified-state `*`.
- [x] LCD left/right arrows provide the same previous/next preset behavior as
  the existing header quick switch. Remove that header switch and its duplicate
  preset display. Keep the LCD and navigation available in EDIT, PERFORMANCE
  and UTILITY views; view switching must not hide them.
- [x] Place an explicitly numbered algorithm dropdown (1–32) beside the
  algorithm diagram. Selection updates the diagram immediately. Remove the
  duplicate GLOBAL ALGO encoder, while retaining the existing underlying host
  parameter ID/index and saved-project/automation compatibility.
- [x] Output level-meter LEDs run predominantly GREEN from the bottom upward,
  then YELLOW near the top and RED at the very top. No blue/cyan lower LEDs.
  The current renderer implements green/yellow/red segments; calibrated meter
  thresholds and clipping interpretation still need host/listening acceptance.

Magyar összefoglaló: állandó LCD-s bank-/hangszínkezelés bal–jobb léptetéssel;
a felső gyorsváltó megszűnik. Az algoritmusábra mellett 1–32-es lenyíló lista
váltja a GLOBAL ALGO tekerőt. A szintmérő alulról nagyrészt zöld, majd sárga,
legfelül piros; kék alsó LED-ek nélkül. A hardverfotók alapján elfogadott
anyag- és színvilág alább szerepel. Az első funkcionális GUI-bekötési kör
szintén alább található; a végleges
grafikai átdolgozás és a PERFORMANCE-oldal még külön feladat.

### Final visual direction — hardware references approved 2026-09-20

User-supplied `concept-1.jpg` and `concept-4.jpg` are visual references only;
do not copy the photos, their branding or cropped surface textures into the
plug-in or redistribute them as project assets. The direction is an original
VDX7 interface with a restrained vintage hardware feel, not a replica front panel.

- [ ] Warm brown-charcoal enclosure with a fine-grained matte finish instead of
  the current blue-metal appearance. Use subtle depth and natural shading;
  avoid artificial wear, heavy chrome, excessive neon or bloom.
- [ ] Yellow-green LCD with dark, high-contrast characters, a slightly recessed
  black surround and a readable character-display feel. Preserve the modern
  bank/program selectors, patch name/number, modified indicator and arrow buttons;
  do not reproduce the photographed display's text or impose its limited layout.
- [ ] Restrained turquoise, lavender and salmon button surfaces assigned
  consistently by function group. States must also be recognisable through
  labels, position or shape, not colour alone.
- [ ] Off-white, readable labels and simple panel-divider lines. Test typography
  and control states at supported small sizes as well as Retina/HiDPI scales.
- [x] Black ribbed wheels and slider caps, natural ivory-white keys, subtle
  highlights and clear press/hover/focus states. The wheel cylinder and its cyan
  indicator now share the same value-driven movement; no aged/grimy texture.
- [x] Original VDX7 wordmark, icons and newly drawn interface assets. The own
  VDX7 Mk I. SVG uses a restrained metallic scanline treatment; no Yamaha logo,
  original product lettering or traced front-panel artwork is used. See
  `docs/validation/VALIDATION_1.0_WHEEL_LOGO_POLISH.md`.
- [ ] Preserve the approved EDIT/PERFORMANCE structure and functional LCD,
  SAVE AS..., algorithm, output and keyboard controls across views. The colour
  redesign must not remove, duplicate or disconnect implemented functionality.
- [ ] Level-meter LEDs remain predominantly green from the bottom upward,
  then yellow and red at the top; no cyan/blue lower segments. Meter colours
  remain distinct from the turquoise used for controls.

Magyar irányelv: meleg barnás-antracit, finoman szemcsés matt ház; sárgászöld
LCD fekete kerettel; visszafogott türkiz, levendula és lazac gombszínek;
törtfehér feliratok, fekete bordázott kezelőszervek, elefántcsontszínű billentyűk.
Saját VDX7-arculat és újrarajzolt elemek, eredeti márkajelzés és fotókivágások
nélkül. Ez elfogadott tervezési irány, nem már elkészült GUI vagy jogi minősítés.

### Header alignment and section dividers — approved 2026-09-20

These design requirements are partly implemented; refer to the checklist below
for implementation status and keep final visual/host acceptance open.

### Operator space and vector control finish — implementation chapter

- [x] Expand the operator panel downward by 40 reference units and move the
  keyboard section and footer down by the same amount. Preserve the outer size
  for now, reducing the unused bottom margin rather than stretching the window.
- [x] Increase envelope slider height from 116 to 169 reference units and extend
  the neighbouring graph from 164 to 239. Align graph bottom with value-row bottom.
  Enlarge operator knobs and separate rows with consistent vertical clearance.
- [x] Add vector knob bevels/knurling, native fader tracks/ticks/caps and restrained
  panel/graph gradients. Existing parameter bindings and audio behaviour unchanged.
- [ ] Complete final header/logo/About integration, material texture, percentage
  size controls and outer-margin fitting after visual acceptance. This chapter
  is not a declaration that the entire skin or Retina/Windows acceptance is final.

### Header and LCD checklist

Vector header implementation: embedded SVG `VDX7 Mk 1.` with a closer Mk 1.
suffix, metal gradient and no font/bitmap dependency. Actual logo bottom and
lower text ink edge align to the visible bottom edge of header buttons. Full
separator below; OUTPUT top panel moved down 10 units to clear the separator.
The accepted operator/keyboard layout is unchanged. About follow-up is recorded below.

About follow-up: SAWSTAR-inspired information layout implemented with VDX7's warm
palette, embedded VDX7/GYR vectors, developer credit, actual development version,
clickable source URL, retained component licenses and ROM requirement. Separate
owned dialog content avoids dependence on the editor's lifetime. Visual approval,
physical HiDPI and Windows host checks remain; no stable-version claim is made.

- [x] Narrow the LCD and its black frame horizontally by moving their left edge
  rightward while retaining the current height and right edge. Reserve a clear
  gap between the UTILITY button and the black frame; all three view buttons
  must fit entirely to the left. Reflow the LCD arrows, patch text and selectors
  within the reduced width without overlaps or reduced text readability. Verify
  the gap and content bounds at every supported UI size. Do not shrink vertically.
  Implemented: left edges +30 reference units, right edges and heights unchanged;
  28-unit UTILITY/frame clearance. Local GUI tests pass at 960/1440/1600 widths.
  Planned percentage-size settings and real-host HiDPI acceptance remain open.

- [x] LCD previous/next arrow buttons use LCD-colour backgrounds, dark arrows and
  restrained outlines, with hover/pressed/focus states and retained navigation.
  Verify hit targets and visual clearance at supported sizes/hosts.

- [x] Header wordmark text is exactly `VDX7 Mk 1.`. Align the bottom of the
  `Mk 1.` lettering with the VDX7 wordmark, as in the corrected vector draft.
- [x] Place `HARDWARE EMULATION` and, underneath it, `Original firmware required.`
  beside the logo. The logo block, the two-line text block and the right-hand
  action-button row share a common lower alignment guide. Keep button bottoms
  aligned, button heights consistent, and labels vertically centred within buttons.
  Source geometry aligns their visible lower edges; final visual/host confirmation
  remains part of the GUI acceptance gate.
- [ ] Use consistent horizontal/vertical guides, spacing and panel padding
  throughout the GUI. No action button may intrude into the LCD, its frame or
  navigation controls; neither visible bounds nor interactive hit areas may overlap.
- [x] Add a horizontal separator between the two rotary-control rows in the
  operator section. Span the full width of that rotary-control subsection,
  respecting equal left/right padding; do not cross into the neighbouring envelope.
  Already present at reference y=720 in the section-divider tone; the 632-unit
  span matches the two row bounds and stops before the adjacent envelope.
- [x] Add three thin horizontal decorative lines across the header top in the
  same restrained tone as the section dividers. Implemented at y=22/34/46 over
  the shared 34-unit inset, with a ROM-free pixel regression at 75/100/125% sizes.
- [ ] Every section divider spans its entire associated section's inner width,
  not just half of it. Use consistent inset, thickness and contrast, and leave
  clearance around labels and controls.
- [ ] Verify these alignments and non-overlap constraints in EDIT, PERFORMANCE
  and UTILITY at planned 75%, 100% and 125% UI sizes and Retina/HiDPI display scales.

Magyar elfogadási feltétel: a logó, a mellette lévő kétsoros tájékoztató és a
jobb oldali gombsor közös alsó igazítási vonalra üljön. Ne legyen LCD-re rálógó
vagy egymást fedő gomb. Az operátor két potmétersora közé teljes al-szekciószélességű
elválasztó kerüljön; minden szeparátor az érintett szekció teljes belső szélességén
fusson végig, egységes margókkal.

## GUI/function integration — status reconciled 2026-09-25

- [x] SETTINGS host MIDI input channel: OMNI or 1-16, legacy/missing-field
  projects default to OMNI. Switching releases old notes/sustain and discards
  stale delayed input; UI keyboard and bank SysEx remain global. Project and
  missing-ROM resave regressions added. See `docs/validation/VALIDATION_1.0_MIDI_CHANNEL.md`.
  Physical controller/DAW acceptance remains open before final GUI polish.

- [x] First SETTINGS slice: master tuning -256..255 using the core's existing
  firmware RAM tuning API, project recall, bounds and measured pitch regression.
  SETTINGS opens a functional Apply/Cancel dialog; these are native units, not
  cents. MIDI input remains OMNI for compatibility. Channel filtering is the
  next separate slice and must preserve legacy project behavior by default.
  See `docs/validation/VALIDATION_1.0_MASTER_TUNE.md`; final skin/host acceptance remain open.

The approved PERFORMANCE layout and controls are implemented with project
persistence; SETTINGS provides input-channel filtering and master tuning. The
existing 148 host parameter IDs/indices are preserved. USER-bank Save As uses a
single persistent 32-slot library, explicit overwrite confirmation and LCD
load-copy; multiple named libraries are not implemented. SysEx export remains
separate from project-level PERFORMANCE state.

The LCD bank/program selectors and previous/next arrows, algorithm dropdown,
output controls, Save As and keyboard remain available as designed. The GUI's
warm hardware-inspired direction, vector VDX7/GYR identity and About content are
implemented to varying degrees, but full visual, host, HiDPI and Windows
acceptance remains open. Retain the agreed design requirements above as the
visual contract; do not treat the original concept images as production assets.

## Reviewed audit follow-up (2026-09-21)

Work in separate reviewed PRs; no final release/tag or firmware upload.

- [x] Share normal-CI/RC build targets so USER-bank and resampling tests cannot
  be omitted by one workflow. Stress runner is compile-only without local ROM.
- [x] Align candidate macOS architecture with Universal CI; update current HU/EN
  usage docs and label the original development snapshot as historical.
- [x] Deterministic reserved-but-unpublished edit-queue transaction regression;
  reservation cutoff prevents stale edits crossing restore/import boundaries
  without waiting for a paused producer. See VALIDATION_1.0_EDIT_BOUNDARY.md.
- [x] Reproduce same-pitch/channel/retrigger ownership against local firmware:
  repeated notes survive one Note Off. Replace pitch booleans with bounded
  multiplicity and release every tracked repeat. See VALIDATION_1.0_REPEATED_NOTES.md.
- [x] Local POLY/MONO capacity and queued-off overflow regression: 32 notes,
  repeated/distinct pitches, all host channels, sustain, silence after recovery
  and an audible fresh note that can be released. Fixed recovery release budget
  lost by flushed pending offs. See VALIDATION_1.0_MIDI_CAPACITY.md.
  Wrapper bookkeeping remains conservative, not an exact firmware allocator;
  real-host acceptance and lock-contention measurements remain open.
- [ ] Measure engine-lock contention and POLY/MONO wall-clock duration. Rendered
  sample duration is not measured lock time; the processor timer is conditional.
  Initial instrumentation now reports total contended blocks/samples, the
  longest contiguous contended sample run, and the last/peak lock-held duration
  of direct POLY/MONO transactions; the deterministic lock probe verifies each
  applicable diagnostic. See VALIDATION_1.0_CONTENTION.md. Real GUI/audio
  overlap measurement and mitigation remain open. Do not treat diagnostics as
  a dropout fix.
  PERFORMANCE periodic reads, including legacy-shaped controller/play/bend
  accessors, now decode one coherent lock-free display snapshot instead of
  acquiring engine locks (docs/validation/VALIDATION_1.0_PERFORMANCE_SNAPSHOT.md and
  docs/validation/VALIDATION_1.0_CONTENTION.md). Settings writes, voice publication and
  ROM/state transactions and the POLY/MONO reset remain separate contention paths; complete real-host
  continuity acceptance is still open.
  State serialization now encodes detached snapshots outside engineMutex_;
  see VALIDATION_1.0_STATE_LOCK_SCOPE.md. Capture and restore still require locks.
- [ ] Profile deferred-MIDI copying before claiming a performance defect.
- [x] Increase GLOBAL pitch-envelope fader height within its existing section;
  see VALIDATION_1.0_GLOBAL_FADERS.md.
- [x] Clip white-key hover tint to the visible key body and add visual regression;
  see VALIDATION_1.0_GLOBAL_FADERS.md.
- [x] Final GUI surface and layout approved by the owner on 2026-09-26:
  footer frame, chassis screws, pitch-envelope positioning and recessed keyboard
  with a short lower fade. The owner tested the macOS arm64 `7dfcade` development
  build in REAPER (VST3) and Standalone and approved its appearance; earlier
  Retina visual acceptance was also owner-reported. Preserve this design.
- [ ] Complete cross-platform/host GUI acceptance; owner visual approval does
  not replace Windows, Intel Mac or full release acceptance below.

## Publication gate

- [ ] M1 REAPER acceptance: multiple instances, automation, transport stop,
      missing ROM, state restore, SysEx, offline render, 44.1/48/96 kHz,
      64/128/256/512 sample buffers.
- [ ] Windows REAPER acceptance and runtime prerequisites verified.
- [ ] Physical Intel Mac acceptance, or explicitly do not claim Intel support.
- [ ] All distributed plugin formats have host validation.
- [ ] Version 1.0.0 in one source of truth; unique exact source commit/tag.
- [ ] Verified signatures, corresponding sources, notices, checksum manifest.
- [ ] No Yamaha firmware/factory ROM in source or binary release archives.
- [ ] Final release notes and package installation instructions approved.
- [ ] Publish stable release only after the preceding gates; never clobber old assets.

The detailed milestones above include historical project context; do not use
their older branch/commit descriptions as the current checkout or release SHA.
