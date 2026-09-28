# 1.0 consolidated execution plan — current status 2026-09-28

Reviewed baseline: `fbea5ea167598f9b625eab9145f53b8700d9eb15` (main after #89).
PR #88 checks passed; PR #89 source `d834f4f` passed Windows run 36410161563,
macOS run 36410161589 and ASan/UBSan run 36410161474. Post-merge main Windows
run 36411273600 and macOS run 36411273595 also passed. These are development
checks, not exact-RC acceptance. Main is re-fetched before each publication;
incoming changes are preserved. This is a dated review checkpoint, not a claim
that moving main will always remain at this SHA.
This plan combines the original 1.0 host/audio/release gates with the
useful findings F1–F19 and the test-system review. Planning is not release
authorization. Older roadmap narratives and validation notes remain historical
records unless explicitly updated here; they are not instructions to reopen
completed GUI work.

## Active work ledger — audit 2026-09-28

Use full IDs `AUDIT-20260928-N1` through `AUDIT-20260928-N8`: older roadmap
audits reused N1/N2/etc. for DIFFERENT findings. Source review is not runtime
reproduction, and an implemented fix is not exact-RC acceptance.

| ID suffix | Finding / evidence class | Next action and completion evidence | Environment | Status |
| --- | --- | --- | --- | --- |
| N1 | Reproduced edit loss while a saved project waits for a matching ROM and another ROM is loaded | Stopped processing, running callbacks and re-save/reopen regressions; preserve feedback/operator edits, leave incompatible engine unchanged, resume normal routing | Local v1.8 ROM, Windows processor harness; no REAPER | MERGED #87; failing baseline and 10/10 related checks PASS; see [validation](../validation/VALIDATION_20260928_PENDING_PROJECT_EDITS.md); exact-RC acceptance remains open |
| N2 | Storage accepts semantically invalid unoccupied USER slots that processor import rejects | CRC-valid invalid fields in occupied and empty slots; reject all malformed packed voices transactionally | ROM-free storage test | REPRODUCED then FIXED locally; included in follow-up 7/7 PASS |
| N3 | Project RAM restore lacked packed voice semantic validation | Reject malformed VMEM before mutation; loaded/deferred ROM and modern/legacy state matrix | Processor harness and local ROM | REPRODUCED then FIXED locally; included in follow-up 7/7 PASS; no crash claim |
| N4 | Shared 16 KB/48 KB fixture claim did not match full direct-engine suite | Full-suite requirement narrowed to combined 48 KB v1.8; all local tests use profile fixture; product 16 KB support unchanged | Local ROM tests | IMPLEMENTED; combined profile PASS, 16 KB negative control fails clearly as expected |
| N5 | CI registration smoke checked only one selected ROM test | Full names, labels, fixture edges, timeouts and failure policy; seven checker negative controls | Configuration-only CI | IMPLEMENTED; actual local ROM-on 36 / ROM-off 10 inventories and checker PASS; PR #89 Windows/macOS/sanitizer checks PASS |
| N6 | Keyboard pitch-wheel return policy differs from mouse release | Owner explicitly chose existing keyboard value retention on 2026-09-28; HU/EN guides clarify distinction | Component/UI policy review | ACCEPTED POLICY, not a defect; no input behavior change |
| N7 | Invalid companion warning hid pending-ROM identity mismatch | Reproduce both conditions; preserve both warnings and pending project recovery | Local ROM processor harness | REPRODUCED then FIXED locally; included in follow-up 7/7 PASS |
| N8 | Moving main described using stale SHA | Dated reviewed baseline and aligned current documents; historical evidence keeps original SHAs | Documentation review | Initial fix MERGED #87; this checkpoint tracks post-#89 main `fbea5ea` |

N2–N7 follow-up evidence, full-suite status and publication state:
[non-host hardening validation](../validation/VALIDATION_20260928_NONHOST_HARDENING.md).
Local FIXED does not imply merged, remote CI PASS or accepted final RC.
Publication checkpoint: the 24-file follow-up was published as #88 with explicit
owner approval and is now merged. All three PR checks PASS. Its N2/N3/N7 fixes
and N4/N5 test-contract changes are therefore merged implementation, not merely
local fixes; the earlier evidence rows retain the actual local reproduction scope.
New non-host round: [reproducible source packaging](SOURCE_PACKAGING_1.0.md)
and HU/EN candidate instructions implemented. Two actual source ZIPs matched;
extracted-source Windows offline build, 10/10 CTest and six packaging tests PASS.
Independent Steinberg validation: 47 PASS, 0 FAIL. About opening/rendering PASS
at the observed size; further UI testing stopped when Windows locked.
See [exact provenance and limitations](../validation/VALIDATION_20260928_SOURCE_PACKAGING.md).
Post-merge main verification on `fbea5ea`: ROM-free CTest 10/10 PASS, packaging
unit tests 6/6 PASS, 36-test local-ROM-on registration inventory and seven
negative controls PASS. A 5,073-file corresponding-source ZIP was generated
and manifest-verified (SHA-256 `19c5ab95e1af36c15b19194a37603557b1975bcd5247d72f94b284ff8db113cd`).
This exact merge-commit ZIP was not extracted/rebuilt in this check. The
exact-candidate workflow remains NOT RUN; no stable publication.

Owner-reported acceptance from the supplied handoff dated 2026-09-28: the owner
reports REAPER testing with no known issue and considers the product releasable.
The earlier dated records in section 3 retain the known scope: macOS REAPER
testing (macOS 26.7) and Windows 10 x64 using Actions build #219 from
`29ab5e350019e32f64650b068afcc7709b409607`. The macOS tested SHA, binary hashes,
REAPER application versions and complete rate/block/instance matrix were not
recorded. Keep this OWNER-REPORTED, not an assistant-run PASS or proof of every
platform/matrix cell. The owner accepts Windows distribution without
publisher signing and macOS distribution with ad-hoc signing only (no Developer
ID/notarization). Show the resulting OS security-warning risk clearly; checksums
prove integrity, not publisher identity. This does not itself authorize release
publication.

Owner scope (2026-09-28): do NOT launch the installed REAPER, replace an installed
plugin or modify host projects during this non-host preparation. Preserve the
owner-reported acceptance separately from exact-SHA/platform evidence. No stable
tag, release or asset publication is authorized by this development work; before
publication show the final version, source SHA and asset list and confirm the
separate publication authorization.

Release-preparation work order (do not reopen completed N1–N8 without a new
reproduction):

1. Align HU/EN README and guides with the actual VST3 distribution, install paths,
   ROM requirement, known limits and accepted signing-warning policy. Keep dev
   labels until a stable package actually exists; do not relabel old screenshots.
2. Prepare bilingual 1.0.0 release notes and an honest tested-platform matrix.
   Keep owner-reported REAPER acceptance distinct from assistant-run evidence.
3. Implement and review the exact stable-build/source-package identity without
   changing plugin IDs, parameter IDs/order or project compatibility; add focused
   packaging tests for release mode before freezing any RC.
4. Freeze one exact candidate SHA, build and test it on required CI/platforms,
   inspect the matching packages and source archive, then record every PASS,
   FAIL, NOT RUN and accepted limitation against that SHA.
5. Present the exact version, SHA and asset list for owner review. Only after
   explicit publication authorization create a new tag/release; verify links and
   checksums afterward. Do not modify an existing tag or release.

N6 remains an accepted owner decision, not a speculative fix. The supplied
handoff is useful acceptance/policy context, not a new test result or publishing
authorization. Record exact source, command, platform, fixture scope and status
in linked validation reports.

Additional findings from this round's runtime checks (not audit N numbers):

- `TEST-20260928-BANK-ORACLE`: processor integration compared an old exported
  bank with live RAM after intentional snapshot-test edits. Move the expected
  snapshot to export time; keep the original byte-exact round-trip assertion
  and add a negative control proving that the later edits changed the bank.
  IMPLEMENTED; final complete processor executable rerun PASS after the
  independent pitch-fader assertion correction below.
- `TEST-20260928-TIMEOUTS`: `vdx7_midi_range` exceeded its existing 60-second
  CTest limit and `vdx7_mono_corrected_processor` exceeded 120 seconds on
  Windows. Sequential baseline reruns reproduced both timeouts; unchanged
  executables completed in 292.44 s (MIDI, prior round) and 130.88 s (MONO,
  isolated rerun). Windows-only budgets are now 600/300 s without reducing
  scenarios/assertions. See the follow-up validation for the complete rerun;
  these are exhaustive emulation tests, not realtime wall-clock guarantees.
  Follow-up full run: 35/36 PASS; aggregate host reset additionally hit its
  180 s limit. Its Windows-only budget is 360 s; retain the initial FAIL and
  the separate unchanged-scenario rerun PASS (176.16 s) in the follow-up report.
  All 36 unique cases now have a latest PASS across the full run and rerun,
  not one newly executed all-green full invocation.
- `TEST-20260928-PITCH-LAYOUT`: after fixing the bank oracle, the processor test
  fails at `pitch faders leave room for values`. The test assumes bottom <=490
  reference units; approved layout uses y=408, height=88 (bottom=496). FIXED
  test oracle: use JUCE's thumb-inset drawing area and check it ends before the
  value label at y=488. The renderer clamps the cap within that drawing area.
  Complete processor executable rerun PASS; production GUI remains unchanged.

These results and subsequent reruns are recorded in the linked N1 validation
report. These checks do not start REAPER.

## Closed implementation and owner-approved scope

- GUI appearance is final: PRs #68/#69 and the final logo/separator polish in
  #79 are merged. The owner approved and tested the #79 GUI, and its macOS and
  Windows Actions passed. This does not close remaining release host/platform
  acceptance.
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

- [x] F3/F4 — FIXED and merged as PR #76 (`3c91f67`): live SysEx admission and
  internal packed import use the shared semantic validator. Reserved-bit
  preservation and transactional failures are tested. The current main also
  includes the later pitch-wheel input fix, PR #77.
- [x] F1 — FIXED and merged as PR #77 (`29ab5e3`): unintended pitch-wheel
  scroll input is constrained without changing host automation or incoming
  MIDI pitch bend; the GUI regression is included. Current main includes it.
- [x] F2 — general wheel-automation host test PASS (owner-tested in REAPER):
  both Pitch and Mod wheels record mouse movement and MIDI-keyboard control;
  manually drawn wheel curves play back, including the Mod Wheel curve.
  No wheel-automation defect was observed in this test.
- [ ] F2a — optional mode-boundary characterization: record exact
  begin/value/end ordering and centre point in REAPER Write, Touch and Latch.
  This was not part of the owner-reported test above; keep it separate from the
  passing general wheel-automation result and make no speculative behavior
  change.

## 2. ROM and project-state integrity

- [x] F5 — REPRODUCED on baseline `29ab5e3`; fix and focused local-ROM tests
  are merged in PR #78 (`23e1503`). New state
  records SHA-256 of firmware plus the effective factory voice image (or an
  explicit no-factory marker), independent of path. Mismatches keep project RAM
  pending; matching content at a new path resumes restore. Legacy states with
  no identity remain path-based for backward compatibility; a follow-up source
  review fixed the missing loaded-path comparison, covered by a passing local
  ROM-backed regression. See
  `docs/validation/VALIDATION_1.0_ROM_CONTENT_IDENTITY.md`. Keep this distinct
  from the already-fixed save-generation/path pairing race.
- [x] F6 — REPRODUCED on baseline `29ab5e3`; targeted fix and ROM-backed
  regression are merged in PR #78 (`23e1503`). A voice edit
  in a fresh no-ROM instance was discarded on first ROM load. The chosen
  behavior preserves explicit edits over the newly loaded initial voice; a
  pending saved project's packed RAM remains authoritative. See
  `docs/validation/VALIDATION_1.0_NO_ROM_FIRST_EDIT.md`.
  Local verification for the current branch: Release Standalone/VST3/AU and
  `vdx7_ci_checks` compiled; all 10 ROM-free tests passed. On 2026-09-26 the
  owner-supplied v1.8 package was used locally: F5 profile, state interleaving,
  and pending-identity regressions passed; the full ROM-backed stability suite
  passed, including F6 first-load edit retention. The fixture remains outside
  the repository and CI artifacts.
  Public Windows and macOS CI both PASS on `36df778` (2026-09-26), including
  plugin builds, all 10 ROM-free tests, and local-ROM test registration smoke;
  Actions does not run the firmware-dependent regressions without the private
  fixture. After the prepare-time epoch fix, Windows run `36269491224` and
  macOS run `36269491357` also PASS on `fed4721` (2026-09-26), with builds,
  ROM-free regressions, registration smoke, and packaging. The full local ROM
  suite was rerun on merged main `d696e56`; see
  `docs/validation/VALIDATION_1.0_MAIN_D696E56_ROM_SUITE.md`.
  See both F5/F6 validation notes for scope and fixture boundaries.
- [~] F8/F9 — SOURCE-DERIVED CANDIDATES: scoped by API/source review on
  2026-09-27; no actionable deadlock or host-reproduced whole-restore defect
  found. JUCE documents APVTS `copyState()` and `replaceState()` as individually
  thread-safe but not real-time-safe; VST3 permits state calls while processing
  (UI thread in real-time use, processing thread in offline use), but neither
  source specifies that overlapping whole-state get/set calls form one atomic
  plugin-wide transaction. In this plugin, engine snapshots are detached under
  `engineMutex_` before APVTS copies or host notifications; `parameterChanged`
  only publishes bounded/atomic edits; and `setStateInformation` releases the
  engine lock before replacing APVTS state and before host-facing parameter
  synchronization. Existing reentrant-save and state/process interleaving
  regressions pass. Do not add a broad mutex based on an unsupported
  simultaneous-restore assumption. Reopen this candidate only if a supported
  host demonstrates overlapping get/set calls producing a user-visible mixed
  state or a reachable deadlock; any fix must preserve the real-time boundary.
  Sources: [JUCE APVTS](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html)
  and [Steinberg VST3 processing FAQ](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Processing.html).
  The clarified disposition is merged in PR #83 (`94081f9`); Windows and macOS
  Actions both passed on that source.
- [x] F10 — REPRODUCED and fixed: Settings Apply partially committed tuning
  before a pending-restore MONO failure. `applySettingsFromUi` now
  validates first and performs the fallible MONO operation before committing
  tuning/channel; a v1.8 ROM-backed regression reproduces the former behavior
  and verifies the corrected all-or-none failure result. `vdx7_ci_checks` and
  35/35 ROM-backed/ROM-free CTest cases pass locally (the desktop-dependent
  SAVE AS integration test is excluded). The fix is merged in PR #78 and was
  included in the full local suite on main `d696e56`. Details:
  `docs/validation/VALIDATION_1.0_SETTINGS_APPLY.md`.

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
- [x] Full merged-main local ROM suite: lifecycle/reset/reactivation, MIDI range,
  timing/stability/stress, ownership/history/overlap retirement, reset overflow,
  deferred partition, portamento, wheel delivery, direct reload, state-ROM
  identity and corrected MONO. On main `d696e56`, 35/35 CTest cases passed on
  macOS 26.7 with Apple clang 21.0.0 and CMake 4.4.3 using the owner-supplied
  v1.8 fixture locally. The desktop-dependent `vdx7_processor` SAVE AS test
  was attempted separately but stopped at its primary-display precondition in
  the command runner; the dialog checks remain NOT RUN. Full command and result:
  `docs/validation/VALIDATION_1.0_MAIN_D696E56_ROM_SUITE.md`. ROM stays local;
  public ROM-free CI is not firmware-runtime PASS. Repeat exact candidate
  testing if source changes after this main SHA.
- [x] Owner-reported REAPER PASS (2026-09-26): Native and Correct MONO note
  boundaries (11/12 and 120/121), Note Off, sustain and repeated-note behavior,
  automation and project reopen, transport, bypass, device restarts, physical
  MIDI, multiple instances and sample-rate/buffer checks all worked in the real
  REAPER test on macOS. Tested build SHA and full platform/rate matrix were not
  recorded; replay on the exact release-candidate SHA remains open.
- [x] Owner-reported Windows 10 x64 REAPER PASS (2026-09-26): tested with the
  Windows VST3 artifact from Actions run `Build Windows VST3 #219`, source
  `29ab5e350019e32f64650b068afcc7709b409607` (the main baseline). The user
  reports the tested functions behave as on macOS. REAPER version and detailed
  rate/block/instance coverage were not recorded. This predates the current
  branch's Windows Standalone CI addition and does not verify it.
- [ ] Operator/algorithm/feedback/master/pitch/mod automation; project reopen,
  missing/later ROM, USER/CUSTOM banks, SysEx import/export and dirty/clean state.
- [ ] Windows x64 and macOS REAPER matrix: 44.1/48/96 kHz, 64/128/256/512/1024
  samples where host-configurable; 1/4/8 instances, GUI open/closed, dense edits,
  physical MIDI and device restarts on the exact candidate SHA. Intel hardware
  acceptance or explicit limit.
- [ ] Retain original SRC frequency/aliasing/latency/listening checks, physical
  MIDI timing, audio allocation/locking audit and long-run overload/CPU tests.

## 4. Invisible GUI hardening and build coverage

- [ ] F12 — coverage gap: actual menu sizes 600×463, 900×694, 1200×925,
  1500×1156, 1800×1388. Add/adjust tests to exercise the actual five selectable
  sizes, including visible control bounds/overlap, editable fields, LCD,
  PERFORMANCE, Settings/About, tooltips, keyboard/footer and host window
  tracking. Include Windows/HiDPI. Preserve the approved appearance.
  The editor now disables arbitrary host/window resizing, and the GUI regression
  verifies no corner dragger plus in-bounds EDIT and PERFORMANCE views at all
  five fixed dimensions. It also checks all six named PERFORMANCE selectors,
  four controller ranges, twelve assignment switches, and panel-child bounds.
  A shared preset table now drives the Settings menu and has ROM-free tests for
  all five sizes and nearest-width selection. The About vector assets, child
  bounds and snapshot are also covered; the regression verifies the VDX7, GYR
  and signature vectors. Actual Settings dialog interaction, broader EDIT
  element/overlap checks, and Windows/HiDPI execution remain open. A local
  attempt to open the modal Settings window from the headless component test
  was unstable, so it is not counted as coverage or product evidence. The
  existing non-modal component/snapshot tests remain green.
  The 2026-09-28 Windows interactive Standalone follow-up was NOT RUN because
  the computer-use application launch approval timed out; no bypass attempted.
  After PR #83, all ten ROM-free tests passed locally on `94081f9`.
- [x] F11 — locally measured and implemented: remove five unused editor image
  loads, retaining all source artwork and used images. 0/1/4/8 fresh-process
  measurements (three repeats) and byte-identical before/after GUI snapshots
  are recorded in the follow-up validation. Eight-editor median sampled
  working set fell from 100.64 to 44.23 MiB; this is not DAW/audio CPU evidence.
- [x] F13 — historical manifest/spec now point to the current
  [runtime asset contract](../design/GUI_RUNTIME_ASSETS.md), with 1440×1110
  geometry, runtime/reference distinction and no promise of a full 2× pack.
- [x] F14 — the About regression requires all three vectors (VDX7, GYR and
  developer signature) and captures a rendered panel snapshot. The test passed
  in the GUI regression and merged cross-platform CI.
- [x] F18 — compile/link CI covers Standalone on Windows/macOS and AU on macOS.
  PR #79's macOS and Windows workflows passed; do not imply host acceptance
  from compilation.
- [x] F19 — optional ROM-free ASan/UBSan job for voice/SysEx/USER, deferred MIDI,
  latest display, bounded files, algorithms and status helper. Workflow added
  in this follow-up for seven non-GUI component tests on macOS; no ROM, no
  failure suppression. Run 36405070221 PASS on PR #88. LeakSanitizer is
  explicitly excluded on this platform; do not claim leak-test coverage.

## 5. Test-system audit follow-up

The supplied test-system review found no CMake syntax defect. It reports that
Windows/macOS configured and built the current graph and each ran ten ROM-free
CTest cases successfully. The items below are coverage/robustness work, not
evidence that the shipped instrument currently malfunctions.

### P1 — close misleading-green and important untested paths

- [x] CTest labels: apply `rom-free` consistently to every ROM-free test so
  `ctest -L rom-free` selects the complete ROM-free suite. Keep `gui` and other
  useful orthogonal labels where they already apply; verified locally with all
  ten registered ROM-free tests selected and passing.
- [x] Public-CI processor coverage: the ROM-free `vdx7_gui_header` target
  checks editor creation, no-ROM Save As/Algorithm disabled states, wheel input
  behavior, header rendering and white-key hover pixels at 1x/2x. Firmware
  integration remains separate and opt-in; no ROM data enters public CI.
- [x] USER-bank semantic corruption: checksum-valid, 7-bit-clean packed voice
  with an invalid semantic field is rejected transactionally (destination
  unchanged); covered by F3/F4 and verified in the local 10-test run.
- [x] Packed-VMEM invalid-field matrix: tests only fields whose accepted ranges
  are confirmed by the format/product contract; checksum-valid invalid imports
  are rejected and destination output remains unchanged.
- [x] GUI input coverage: ROM-free component-level tests verify pitch spring
  return on release, MOD retention, keyboard adjustment and pitch-scroll
  filtering in `vdx7_gui_header`. Scroll/trackpad and host automation gesture
  boundaries remain separate checks; preserve owner-reported REAPER evidence
  and do not call a missing test an observed product defect.

### P2 — make local, release and alternate build paths explicit

- [x] Give every CTest test an intentional timeout. ROM-free checks use
  20–60s; measured GUI/processor paths have wider limits; lifecycle/stress/soak
  keep their longer existing overrides. Inventory uses recent recorded runtime
  evidence (including ~9s GUI, ~26s portamento and ~45s corrected processor) to
  avoid tight wall-time limits. The ROM-on registration smoke confirmed all
  36 registered tests expose a timeout.
- [x] Add a no-execution CMake registration smoke to Windows/macOS CI:
  configure `VDX7_ENABLE_ROM_TESTS=ON` with a placeholder path, then inspect
  CTest's JSON listing. The new checker covers all 36 ROM-on and 10 ROM-off
  names, fixture edges, labels, timeouts and absence of disabled/skip policy,
  replacing the earlier single-test assertion. This is registration evidence,
  not ROM acceptance, and executes no firmware tests.
- [x] Compile smoke with `VDX7_RELEASE_BUILD=ON`, with no artifact publication:
  local macOS Release Standalone, VST3 and `vdx7_ci_checks` built; all 10
  ROM-free tests passed. This is a compile smoke only, not release or host
  acceptance.
- [x] Exercise supported compile targets: macOS CI includes Release
  Standalone, AU and VST3; Windows CI includes VST3 and Standalone. Both Actions
  passed on PR #79's exact source tree (runs `36303586325` and `36303586277`).
  Compilation is not host acceptance.
- [x] Exercise the corresponding-source/offline dependency path: a clean source
  snapshot with `third_party/JUCE` and `third_party/dx7Lib` populated from the
  documented pinned revisions configured with
  `FETCHCONTENT_FULLY_DISCONNECTED=ON`; VST3, AU, Standalone and CI-test targets
  built, and all 10 ROM-free tests passed. This validates the extracted source
  tree layout and offline build path, not archive publication or host acceptance.
- [x] Optional `pluginval` gate exercised locally: Windows 1.0.4, strictness 5,
  GUI enabled, default rate/block matrix, final detailed log SUCCESS. See the
  follow-up validation for command/tool hash and limitations. Repeat on exact
  RC. Separate Steinberg validator now PASS (47/47) on the extracted-source
  Windows build; see the source-packaging report for exact identity. Real host
  acceptance and final-RC repetition remain open.
- [x] Clarify the local-ROM contract in CMake and the HU/EN guide: one shared
  product accepts 16 KB firmware (optional sibling factory voices) or a 48 KB
  combined image, but the complete opt-in suite requires a combined 48 KB v1.8
  image. This corrects the previous broader fixture claim. All 26 local-ROM
  registrations are now covered by a complete inventory checker; placeholder
  CI never executes firmware tests.

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

- [x] F17 — [identity/package review](IDENTITY_AND_PACKAGE_1.0.md) documents
  numeric host 1.0.0 versus displayed 1.0.0-dev and exact-SHA artifact naming.
  Historical bundle/plugin IDs are preserved. Cosmetic prototype DESCRIPTION
  cleanup and any future rcN display option are separate candidate changes.
- [ ] Freeze one RC SHA; run public Windows/macOS checks, full local ROM suite,
  host matrix and exact-SHA candidate workflow. Retest relevant gates after fixes.
- [x] Document single USER-library limitation and supported formats/hosts:
  HU/EN guides and the identity/package review distinguish primary VST3,
  compile-only AU/Standalone coverage and dated host evidence. Exact RC host
  acceptance is still required; no new runtime support claim was made.
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

Next concrete order (2026-09-28; subject to the active audit ledger above):
1. PR #83 is merged and its Windows/macOS Actions passed; no further action is
   needed for that source-only clarification.
2. Close the remaining F12 Windows/HiDPI and interactive Settings/About GUI
   coverage gaps without changing the approved appearance. Do not force a
   modal UI check into a harness that cannot safely host it; use a suitable
   desktop test or record explicit owner verification.
3. Freeze an exact release-candidate SHA, then run the still-open host-dependent
   gates on that artifact: offline render comparison (F7), envelope/release and
   dense MIDI/contention characterization (F15/F16), and the missing REAPER
   matrix. Existing owner-reported macOS/Windows REAPER passes remain valid for
   their recorded baseline scopes, not as exact-candidate acceptance.
4. Complete packaging, licensing, naming, USER-library/host support, and
   signature/notarisation review before any release approval.

F5/F6 are already fixed and merged together in PR #78; do not list them as the
next implementation step. F2's general wheel-automation host test is accepted;
the optional Write/Touch/Latch boundary characterization can be done separately
and is not a failure or blocker for that result. Local ROM-free PASS is not
firmware-runtime or host acceptance. No main merge, tag, or release publication
is authorized by this plan.
