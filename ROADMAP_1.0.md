# VDX7 1.0.0 release gate

Target: stable 1.0.0, not another public pre-beta. Work-in-progress builds are
not final releases. Do not publish or replace existing release assets until
the acceptance checklist is complete. Keep plugin IDs and existing parameter
indices compatible with saved projects.

## Stabilization

### Current next steps — 2026-09-24

Latest checkpoint: PR #55 roadmap refresh was merged to `main` as `2624097` on 2026-09-25; PRs #50–#54 contain the code/test changes summarized below. Private-ROM acceptance remains a separate gate.
`VALIDATION_MONO_SOAK.md` records the full 30-test run, separate desktop retry,
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
   see VALIDATION_GUI_RESTORE_ORACLE.md for the persistent-state oracle change.
3. Real DAW acceptance, remaining GUI finish/scale checks and release packaging.
   PR #47 is merged; publishing remains a separate approval.

### Audit follow-up — 2026-09-24 (reviewed against current main)

The audit package `VDX7_AUDIT_WORK_30a3ccbd.zip` examined main
`30a3ccbd` immediately before PR #48. PR #48 changed documentation and editor
wording only; PR #49 added roadmap documentation only. Since the audit, PR #50 corrected the reset-history test oracle and PR #52 fixed the malformed detune round-trip at component level. The audit's other processor, MIDI validation, keyboard queue and SysEx findings remain subject to their stated evidence limits on current main `675b553`. The audit's Linux component/model runs are useful
evidence, but are not a full JUCE, firmware or DAW acceptance run. Preserve each
finding's evidence class; do not promote a model result into a product pass.

Audit disposition refreshed against main `2624097` on 2026-09-25:
- N1 reset-history oracle correction merged in PR #50 (`675b553`). The ROM-enabled reset-history/full CTest acceptance remains NOT RUN; the merge fixes the test expectation, not a product audio defect.
- U2 malformed checksum-valid detune round-trip was fixed in PR #52 (`cc2f4aa`): reject detune nibble 15 and cover all six operators with a checksum-valid regression. Keep the finding scoped to malformed input; do not generalize it to ordinary factory patches or call it a checksum defect.
- T1 processor-boundary characterization and Note 127 guard-bypass sensitivity control merged in PR #53 (`7bb7af9`); synchronized macOS/Windows CI passed. The ROM-enabled characterization itself remains NOT RUN.
- U1 direct ROM reload/deferred-MIDI boundary fix and opt-in processor regression merged in PR #54 (`9d57069`); macOS/Windows Actions passed on the merge commit. The local v1.8 ROM regression and full ROM-enabled suite remain NOT RUN, so runtime acceptance is pending. N2 state/ROM identity mixing remains an unconfirmed processor-level candidate. U3 now has a reproduced deferred-queue component case; actual processor confirmation remains open.
- N3 bounded file-read and N4 public keyboard-queue admission are lower-priority
  hardening tasks.
- MIDI Note 12–120 policy is consistent in product admission; internal 0–127
  cleanup/release loops are intentional. The 2026-09-25 post-PR-54 roadmap refresh ran no tests; it records the merged commits and current NOT RUN boundary.
- N5 — host tail metadata remains a P2 validation candidate: `getTailLengthSeconds()`
  still returns 0.0 although voices can release after Note Off. Verify JUCE/host
  offline-render tail behavior and add a Note-Off release-tail regression before
  choosing a conservative non-zero or dynamic estimate. No host truncation has
  been reproduced yet; do not describe it as a confirmed audible defect.

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
- **N6 deferred-event capacity — component reproduction.** With the same
  64-sample initial timeline lag, 257 CC events in one 16,448-sample callback
  trigger one panic and deliver none; the same event sequence spread over 257
  64-sample callbacks delivers all 257 without panic. This shows a callback-
  partition-dependent component boundary at the 256-event capacity, not yet a
  full processor defect. Add a deterministic processor integration regression
  that enters deferral through real lock contention, compares equivalent event
  timelines/block partitions, and characterizes intentional panic/recovery
  semantics before changing storage or overflow policy.
- **U3 — conditional CC32 deferred pressure.** The probe admits CC32 bank values
  0–7 without factory-image context; 256 requests plus one supported note panic
  the deferred queue. The engine's later no-factory rejection is source-verified,
  not executed against firmware by this probe. Add the processor-level paired
  controls noted above; classify this as component-reproduced and integration-
  pending, not a confirmed user-visible bug.

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

3. **N2 — reproduce state/ROM identity mixing before changing synchronization.**
   Add deterministic processor-level save-versus-ROM-load interleavings and
   assert that RAM and saved ROM identity belong to the same engine generation.
   The audit reproduced mixed pairs only in protocol models, not in JUCE. If the
   real processor test confirms the race, make snapshot capture and ROM identity
   publication generation-consistent. Keep XML/base64 work outside long audio
   engine-lock sections; do not treat an extra reader lock alone as a fix.

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

6. **U3 — conditional CC32 admission: component case reproduced; processor test pending.**
   The 2026-09-24 audit probe showed 256 factory-bank-select messages plus a
   supported note can overflow the deferred queue. Source inspection confirms
   the engine rejects CC32 bank selection when no factory image is loaded, but
   the shared host validator still admits values 0–7 before deferral. Add a
   deterministic processor test with lock contention and no factory image,
   plus a factory-image-present control. If confirmed end-to-end, filter only
   unserviceable requests before they consume deferred capacity; do not add a
   blocking engine lock to the input path.

7. **Lower-priority hardening:** N3, bound ROM/SysEx file reads before allocating
   full payload copies and verify failed imports do not mutate the loaded
   engine; N4, pre-admit events on the public keyboard API before its bounded
   queue. Keep N4 scoped to the programmatic API: the visible keyboard is
   36–96 and normal host MIDI is already filtered. Neither item is a reproduced
   normal-GUI crash.

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
  See `VALIDATION_1.0_BANK_SELECT.md` for implementation and local test scope.
  Existing CC0-ignore / immediate CC32 behavior remains unchanged; a different
  MSB/LSB policy needs an explicit compatibility decision and host acceptance.
- [x] Follow-up static-review validation: malformed MIDI status/length/data-byte
  rejection, system-common handling, checksum-valid live bank SysEx admission,
  and keyboard UI-held state after overflow. Malformed SysEx is rejected before
  it can exhaust deferred event/byte storage; the engine shares the admission
  validator. The processor integration renderer requires its explicit local ROM
  fixture. See `VALIDATION_1.0_MIDI_INPUT.md`.
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
  validated bank SysEx stays on its dedicated path. See `VALIDATION_1.0_MIDI_INPUT.md`.
- [x] Reproduce the reported overflow + lost UI Note Off scenario: local test
  confirms release and no mirror-triggered note. No speculative UI reset applied.
  Broader simultaneous-held-key/host-contention acceptance remains open.

- [x] Deferred multi-block MIDI timeline, restart cleanup and consistent program
  normalization implemented; see `VALIDATION_1.0_MIDI_LIFECYCLE.md` for policy
  and local tests. Actual host transport acceptance remains open.
  Q1 follow-up corrects false age expiry in large successful callbacks, with
  real-processor before/after reproduction, waveform comparison and true-delay
  expiry tests. The two-second actual-lag limit remains; see
  `VALIDATION_DEFERRED_PARTITION.md`. Queue-capacity and real-host limits remain.

- [x] Ordered program/bank/voice/operator edit queue with local save/audio
  regressions; see `VALIDATION_1.0_EDIT_ORDER.md` for scope and overload policy.

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
  engine lock; editorless/reentrant state tests (see `VALIDATION_1.0_HOST_PUBLICATION.md`).
- [x] Coalesce frequent non-disruptive PERFORMANCE/SETTINGS writes outside
  `engineMutex_`: controller range/assignments, pitch-bend range/step, master
  tuning, portamento mode, glissando and bounded portamento time. A held-lock regression and a
  concurrent 1,000-write/audio test confirm prompt UI snapshots, save-time
  commit and zero measured contention from this path. See
  `VALIDATION_1.0_CONTENTION.md`.
- [ ] Keep the deliberate POLY/MONO firmware transaction in the real
  GUI/audio-overlap audit. It must retain its firmware reset and note-release
  behavior; do not move it to the callback without a bounded transaction design
  and host evidence.
- [ ] Complete remaining audio-thread allocation/locking audit and contention stress test.
- [x] Guard firmware serial/controller saturation; validate recovery with a
  60-second simulated load and callback ordinary-C++ heap probe
  (`VALIDATION_1.0_MIDI_OVERLOAD.md`). Direct-C/aligned heap profiling and live
  host soak acceptance remain open; this is not a hard-realtime guarantee.
- [x] Remove keyboard-state locking/listeners from audio; bounded UI MIDI handoff
  and 12-configuration local stress grid (see `VALIDATION_1.0_REALTIME_BASELINE.md`).
  Full allocation/overload audit and host acceptance are still open.
- [x] Q2 stale PERFORMANCE/tuning display publication: guard the commit against
  newer UI edits (including same-value/ABA) without audio waiting/retry. Real
  processor before/after reproduction and public-CI helper regression; see
  `VALIDATION_PERFORMANCE_PUBLICATION.md`. Restore/write epochs remain separate
  open work; the portamento follow-up below is independently reproduced/tested.
- [x] Retain the latest accepted portamento setting separately from transient
  firmware work RAM. Immediate save during recovery, queued CC5 display rollback,
  flushed-command replay and later accepted physical CC5 are covered by
  `VALIDATION_PORTAMENTO_INTENT.md`. Native CC5 still computes the actual rate;
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
  See `VALIDATION_MONO_BOUNDARY_AND_CI.md` and `AUDIT_TRIAGE_2026-09-23.md`.
  Follow-up: local instruction trace now identifies the failing allocation and
  release branches. Subsequent-note loss/retained output without recovery is
  reproduced, including sequential On/Off pairs and both Off encodings. This is
  diagnosis/test coverage, not a production fix or hardware confirmation; see
  `VALIDATION_MONO_INSTRUCTION_TRACE.md`.
  Native/corrected-path options and required acceptance are separated in
  `DESIGN_MONO_NOTE_ZERO_POLICY.md`. Targeted corrective development is now
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
  ownership groups have a separately labelled, required firmware prerequisite;
  public CI executes six ROM-free tests, including the Q2 publication helper.
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

- [ ] PERFORMANCE/SETTINGS scope implemented and tested: pitch range, controller
      assignments, MIDI input channel and tuning, preserving state compatibility.
- [ ] Decide mono/portamento scope from firmware capabilities and host tests.
- [ ] Measure resampling/aliasing; quality implementation accepted against references.
  Milestone 3B adds band-limited SRC, selected spectral limits, exact reported
  latency and CPU diagnostics (`VALIDATION_1.0_BANDLIMITED_SRC.md`). Physical
  reference/host listening and latency-compensation acceptance remain open.
- [ ] Current documentation matches all shipping features and limitations.

## Agreed GUI direction — 2026-09-20 (not implemented)

First implementation pass: warm enclosure/panels, yellow-green LCD, matte knobs,
restrained active buttons, ON/OFF performance switches, GYR mark in ABOUT and
green/yellow/red meters. See `VALIDATION_1.0_WARM_GUI.md`. Remaining checklist
items below still require final visual and host acceptance; this is not a final skin.

Second visual slice groups PERFORMANCE into PLAY MODE, PITCH BEND and PORTAMENTO
cards, retaining every firmware-backed choice and the four controller panels.
Short field labels and grouped control bounds are checked at three editor sizes.

The three GUI visual concepts remain layout references, not a finished skin.
The approved hardware-inspired material/colour direction is specified below.
The following functional/layout requirements were explicitly agreed with the
user. Record them now; implementation follows the stabilization work.

- [ ] Make the LCD the single bank/patch navigation centre: bank selector,
  direct program selector, current program number/name and modified-state `*`.
- [x] LCD left/right arrows provide the same previous/next preset behavior as
  the existing header quick switch. Remove that header switch and its duplicate
  preset display. Keep the LCD and navigation available in EDIT, PERFORMANCE
  and UTILITY views; view switching must not hide them.
- [x] Place an explicitly numbered algorithm dropdown (1–32) beside the
  algorithm diagram. Selection updates the diagram immediately. Remove the
  duplicate GLOBAL ALGO encoder, while retaining the existing underlying host
  parameter ID/index and saved-project/automation compatibility.
- [ ] Output level-meter LEDs run predominantly GREEN from the bottom upward,
  then YELLOW near the top and RED at the very top. No blue/cyan lower LEDs.
  Exact level thresholds and clipping indication remain implementation details
  to specify and validate; the concepts do not define a calibrated meter scale.

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
  `VALIDATION_1.0_WHEEL_LOGO_POLISH.md`.
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

These are recorded design requirements, not completed implementation.

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

- [ ] LCD previous/next arrow buttons must look like graphics drawn by the LCD,
  matching the BANK and PATCH dropdowns: yellow-green display background, dark
  arrows and restrained dark outlines. No raised hardware-button treatment,
  metallic bezel or external glowing turquoise button skin. Keep both arrows
  inside the display layout with clear spacing from the selectors and patch text.
  Provide readable LCD-style hover, pressed and keyboard-focus states, adequate
  hit areas, and retain the existing previous/next preset behaviour across views.

- [ ] Header wordmark text is exactly `VDX7 Mk 1.`. Align the bottom of the
  `Mk 1.` lettering with the VDX7 wordmark, as in the corrected vector draft.
- [ ] Place `HARDWARE EMULATION` and, underneath it, `Original firmware required.`
  beside the logo. The logo block, the two-line text block and the right-hand
  action-button row share a common lower alignment guide. Keep button bottoms
  aligned, button heights consistent, and labels vertically centred within buttons.
- [ ] Use consistent horizontal/vertical guides, spacing and panel padding
  throughout the GUI. No action button may intrude into the LCD, its frame or
  navigation controls; neither visible bounds nor interactive hit areas may overlap.
- [ ] Add a horizontal separator between the two rotary-control rows in the
  operator section. Span the full width of that rotary-control subsection,
  respecting equal left/right padding; do not cross into the neighbouring envelope.
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

## GUI/function integration chapters — approved 2026-09-20

- [x] SETTINGS host MIDI input channel: OMNI or 1-16, legacy/missing-field
  projects default to OMNI. Switching releases old notes/sustain and discards
  stale delayed input; UI keyboard and bank SysEx remain global. Project and
  missing-ROM resave regressions added. See `VALIDATION_1.0_MIDI_CHANNEL.md`.
  Physical controller/DAW acceptance remains open before final GUI polish.

- [x] First SETTINGS slice: master tuning -256..255 using the core's existing
  firmware RAM tuning API, project recall, bounds and measured pitch regression.
  SETTINGS opens a functional Apply/Cancel dialog; these are native units, not
  cents. MIDI input remains OMNI for compatibility. Channel filtering is the
  next separate slice and must preserve legacy project behavior by default.
  See `VALIDATION_1.0_MASTER_TUNE.md`; final skin/host acceptance remain open.

The PERFORMANCE concept based on visual concept 3 is the agreed layout direction,
not a specification of implemented functionality or controller default values.
Implement real bindings, persistence and round-trip tests alongside each control.
The existing skin remains in use during the first functional integration chapter.

1. **Navigation/export wiring (merged PR #11):** move preset arrows to the LCD,
   replace the algorithm encoder with its numbered dropdown using the unchanged
   host parameter, expose existing single-voice and 32-voice SysEx export through
   a persistent SAVE AS... header button. This is file export, NOT yet a USER
   library. Existing UTILITY export remains available. Factory ROM is never written.
2. **Firmware-backed PERFORMANCE:** establish parameter ranges and behavior from
   the core/firmware, then connect the approved play mode, pitch bend, portamento
   and four controller-assignment panels. No decorative active controls. Keep LCD,
   algorithm, output, Save As and keyboard available across views. Test both GUI
   and MIDI paths, project restore and compatibility with the 148 existing IDs/indices.
   SETTINGS MIDI channel/tuning are included in this functional work.
   First slice: four controller range/assignment panels connected to firmware
   battery RAM, persistent project recall and EDIT/PERFORMANCE switching.
   No new host automation IDs; these are message-thread global controls.
   Pitch-bend range/step now have firmware-backed PERFORMANCE selectors (0-12)
   and project persistence. The extra wrapper pitch offset was removed: zero
   range is respected and nonzero step follows the firmware's own quantisation.
   Next functional slice adds POLY/MONO, mode-dependent portamento choices,
   glissando and time 0-99. Mode switching ends notes through native firmware CC
   processing; time updates/recalled time refresh the firmware's derived rate.
   Physical pedal/host acceptance and SETTINGS channel/tuning remain open.
3. **USER preset storage / Save As:** default destination is a USER bank + slot,
   with name entry and explicit occupied-slot overwrite confirmation. Preserve
   factory originals; make saved USER banks selectable from the LCD. Also support
   single-patch export and whole-bank export. Separate voice payload from global
   PERFORMANCE settings; ordinary voice SysEx must not silently claim to contain
   controller/global state. DAW project save remains independent. Verify exact
   export/import, cancellation, failed writes, changes while dialogs are open,
   missing files, bank switching and session restart before marking complete.
   Approved save logic: ONE header SAVE AS... button opens a shared dialog with
   **Patch -> USER bank** as the default, **Export Patch...** and **Export Bank...**
   as explicit alternatives. A persistent custom bank can be populated one patch
   at a time; saving a patch replaces only the chosen slot. Ask for the patch name
   and destination slot, show the existing occupant before overwrite confirmation,
   and retain all other slots. Factory originals remain unchanged.
   UTILITY is for editing/organisation (rename, operator copy/paste, later bank
   organisation), not a second save workflow. Remove its duplicate export items
   only when the complete shared Save As dialog replaces them. No additional
   Save Patch / Save Bank header buttons. Global PERFORMANCE settings remain
   project state, never silently included in voice SysEx.
   First storage slice implements a versioned, checked 32-slot bank file and
   conflict-aware patch updates. This backend is tested independently without
   ROM. The next integration slice connects the backend to a shared SAVE AS dialog
   (default USER action, name, 32 destination slots and overwrite confirmation),
   a per-user library folder, and LCD `USER (load copy)` selection. Loading opens
   an independent CUSTOM working copy, selecting its first occupied slot; edits
   never automatically overwrite the library file. Save-copy retains working-bank
   dirty markers conservatively. UTILITY now contains editing tools only.
   Remaining acceptance: real DAW interaction, Windows GUI validation, multi-instance
   library usage, and final visual styling. Multiple named USER banks are not yet implemented.
4. **Visual integration and acceptance:** combine concept 3's approved layout
   with the hardware-inspired material/colour direction above for
   EDIT/PERFORMANCE/UTILITY, green-yellow-red meters, resize/readability
   checks and real host interaction checks. Generated mockup is reference only,
   not a screenshot of the shipping plugin. No release until all release gates pass.

- [x] First-stage SAVE AS... button exposes real voice/bank SysEx file export.
- [ ] USER bank library, destination slot and non-destructive Save As workflow.
- [x] USER-bank storage foundation: persistent 32-slot file, single-slot update,
  overwrite/conflict protection, checked temporary-file replacement and ROM-free CI tests.
- [x] Initial USER GUI wiring: shared Save As dialog, immutable patch capture,
  per-user USER bank, LCD load-copy action and processor/session regression tests.
- [ ] PERFORMANCE panel layout with firmware-backed bindings and project recall.
- [x] First PERFORMANCE slice: four controller ranges and 12 assignment switches,
  project/missing-ROM recall, real MIDI/audio regression and persistent LCD/header.
- [ ] Final hardware-inspired visual treatment, original VDX7 identity and host usability acceptance.

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
  acquiring engine locks (VALIDATION_1.0_PERFORMANCE_SNAPSHOT.md and
  VALIDATION_1.0_CONTENTION.md). Settings writes, voice publication and
  ROM/state transactions and the POLY/MONO reset remain separate contention paths; complete real-host
  continuity acceptance is still open.
  State serialization now encodes detached snapshots outside engineMutex_;
  see VALIDATION_1.0_STATE_LOCK_SCOPE.md. Capture and restore still require locks.
- [ ] Profile deferred-MIDI copying before claiming a performance defect.
- [x] Increase GLOBAL pitch-envelope fader height within its existing section;
  see VALIDATION_1.0_GLOBAL_FADERS.md.
- [x] Clip white-key hover tint to the visible key body and add visual regression;
  see VALIDATION_1.0_GLOBAL_FADERS.md.
- [ ] Final surface polish and real-host/HiDPI acceptance. Preserve the approved
  About and header alignment; their visual arrangement has owner approval.

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

The installed plugin and the original local development checkout are preserved.
Local full-range development commit 80ebf54 and reviewed main 483daf7 have been
merged into the isolated codex/1.0-stabilization branch.
