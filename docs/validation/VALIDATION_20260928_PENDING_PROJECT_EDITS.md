# AUDIT-20260928-N1 — edits while waiting for matching ROM

## Scope and reproduction

Baseline: `b58679c80d54c1cedaebf26a49817329e8b82997`.
Local tested implementation: `635ef709b4296837095f5ef97e1065b18dd08717`
(processor fix plus expanded pending-identity regression). Browser publication
uses separate commits; compare file content, not commit IDs, when reproducing.
Published source/test equivalent: `f45572a729f1707370638904468a8e9ca1030bff`
on `fix/pending-rom-edits-20260928`; fetched and compared with the locally
tested Source/Tests files with no differences. Later commits add documentation.
This N1 is the 2026-09-28 audit finding, not the historical host-reset N1.

The first regression addition was run against unchanged baseline processor code
on Windows x64, MSVC 2022 Release, pinned JUCE/dx7Lib dependencies and the
owner's local combined 48 KB v1.8 ROM. No REAPER or installed plugin was used.
The test makes isolated temporary copies, changing a factory voice name byte
to create a different content identity without modifying firmware. The copies
are removed by the test; no ROM data is committed or uploaded.

Baseline result: **FAIL (reproduced)**, CTest exit 8:

```text
vdx7_v18_profile: PASS
vdx7_pending_rom_content_identity: FAIL
FAIL: pending mismatch must preserve operator edit after matching ROM load
```

The failed sequence restores a saved project whose original ROM path is
missing while another valid ROM is loaded, edits a voice/operator parameter,
then loads matching ROM content. Ordinary edit-queue entries belonged to the
alternate engine and were discarded at install; they never entered the saved
project's deferred-edit record.

## Fix and policy

Explicit voice/operator parameter edits while project restore is pending target
the preserved project, consistent with the existing no-ROM deferred-edit policy.
A lock-free atomic routing mirror directs parameter callbacks to the existing
pending-value/dirty-mask storage. Engine-locked consumers leave those masks for
the deferred project capture. Incompatible-engine publication must not replace
the host parameter view. After matching installation, normal edit routing resumes.

No new host parameter, state-format replacement, audio-thread file I/O, blocking
parameter callback, firmware patch, or change to the approved GUI is introduced.
This is scoped to voice/operator parameter edits, not a new policy for bank
imports, program changes or every possible simultaneous state/parameter call.

## Regression coverage

The existing `vdx7_pending_rom_content_identity` test now also checks:

- stopped processing, active callbacks, and save/reopen while waiting;
- feedback and OP6 output-level edits after matching-ROM installation;
- unchanged incompatible engine and preserved host parameter values;
- normal editing after the pending project is installed;
- the previous same-content/new-path and legacy path-based restore cases.

## Verification record

- Baseline reproduction: **FAIL as expected**, described above.
- Initial fix build: **PASS**.
- Initial focused run: **2/2 PASS**, including required v1.8 fixture check.
- Expanded regression and related suite: **10/10 PASS**, 289.71 seconds.
  Includes v1.8 profile, host reset, reactivation, reset/history pairing,
  deferred partition, portamento, wheel delivery, direct ROM reload boundary,
  state/ROM identity and pending ROM content identity. The expanded pending
  identity regression passed in 2.47 seconds.
- Complete `vdx7_all_tests` build: **PASS**, Windows x64 Release.
- Remaining CTest group: **24/27 PASS, 3 FAIL**, exit 8, 746.69 seconds.
  The v1.8 fixture is counted in both groups: together the two groups exercise
  all 36 registered tests, not 37 unique tests. Original failures are processor
  bank-oracle assertion, MIDI-range timeout and corrected-processor timeout.
  All 10 ROM-free tests PASS. Subsequent processor correction/reruns below do
  not erase that original failed run. A completely green full suite is NOT
  claimed. Public Windows/macOS CI on this patch is **NOT RUN** in this record.
- REAPER/DAW acceptance and exact final RC: **NOT RUN** by owner scope.

### Additional test findings and explicit failures

The broader run exposed `vdx7_processor` failing at `bank file round trip`.
Source review shows the expected RAM was captured AFTER two deliberate live
edits in the immutable-export regression, while the file was exported BEFORE
those edits. This is a test-oracle defect, not evidence of an export defect.
Local commit `7e75ee7bfc3c63cb4ef74fa155caa3772002e4c9` captures the expected bank
at export time and adds a negative control that the subsequent edits differ.
The exact 4096-byte round-trip comparison remains intact.

Intermediate processor target build: **PASS**. First executable rerun: **FAIL**
(exit 1), now at `pitch faders leave room for values`, after the bank assertions
passed. The approved layout defines the fader at y=408, height=88; the test
assumes its bottom <=490 reference units. Source inspection of pinned JUCE
`LookAndFeel_V2::getSliderLayout` and `Slider::paint` confirmed that the renderer
receives a drawing rectangle inset by the thumb radius. VDX7 clamps the cap
inside that rectangle. Local commit `033004b19ff9d7aed4affde3012b9767a61d145a`
checks the drawing-area bottom against value-label y=488 (plus pixel rounding),
instead of comparing the component hit area's bottom to 490.

Final processor build and complete direct executable rerun: **PASS**, exit 0.
Final `ctest -R ^vdx7_processor$` rerun: **1/1 PASS**, 24.14 seconds overall
(test body 24.12 seconds), under the unchanged CTest limit. Latest per-test
outcomes are therefore 34 PASS and 2 unresolved timeout FAIL across 36 unique
tests, combining the original groups with this rerun; not a clean single run.
This covers all assertions including the two corrected oracles, 32 algorithm
diagrams at three sizes, state/RAM/legacy restore, file export/import, failed I/O,
controller settings, and finite audible rendering at 44.1/48/96 kHz with
64/128/256-sample blocks. No production GUI code was changed. This is processor
harness evidence, not a DAW or manual file-chooser acceptance run.

The broader run also timed out `vdx7_midi_range` at its existing 60-second limit
and `vdx7_mono_corrected_processor` at 120 seconds. Keep both **FAIL (timeout)**
visible pending investigation. MIDI range is a direct-engine
test; this patch changes processor routing, not `VDX7Engine` or that test's code.
This observation does not prove the timeout is harmless or that it predates the
patch. No timeout or CTest skip policy was relaxed. Other development checks
overlapped parts of the broad run, so these durations are not controlled CPU
performance measurements. Repeat the timed-out tests with an idle machine;
if still slow, measure completion and agree a justified test budget separately.

Additional MIDI-range diagnostic: the unchanged executable completed with
**PASS**, exit 0, in **292.44 seconds**, without CTest's 60-second cutoff. It
completed all 128 notes plus pitch/tuning/bend/portamento assertions. Parts of
this diagnostic overlapped other tests; it is not an idle-machine benchmark.
This narrows the observed failure to the time budget on this run but does NOT
turn the original CTest timeout into PASS. Corrected-MONO timeout still needs
a completion diagnostic. Keep both CTest timing gates open.

Local compiler startup initially failed because reused MSBuild workers inherited
duplicate `Path`/`PATH` entries. A deduplicated child environment and single-node,
non-reused MSBuild invocation allowed the build. No system/user environment was
changed. An existing C4805 warning in `VDX7MonoTrace.h` remains; build PASS does
not mean warning-free compilation.

Commands (run from a configured checkout; substitute a private local ROM path):

```text
cmake --build <build> --config Release --parallel 1 --target vdx7_host_reset_tests -- /nr:false
cmake --build <build> --config Release --parallel 1 --target vdx7_all_tests -- /nr:false
ctest --test-dir <build> -C Release --output-on-failure --no-tests=error -R ^vdx7_pending_rom_content_identity$
ctest --test-dir <build> -C Release --output-on-failure --no-tests=error -R ^vdx7_processor$
```

CTest groups used `-R ^vdx7_(pending_rom_content_identity|state_rom_identity|direct_rom_reload_boundary|host_reset|reactivation|reset_history_pair|deferred_partition|portamento|wheel_delivery)$`
and then `-R ^vdx7_ -E` with that same expression. Fixture dependencies add the
v1.8 profile to both groups. Reproduction configuration enables
`VDX7_ENABLE_ROM_TESTS=ON` and sets `VDX7_TEST_ROM_FILE` to the private combined
48 KB v1.8 file. Dependencies are pinned JUCE
`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8` and retromulator
`d5473776a0449d60a997b91bdc888598a33265ac`; compiler is MSVC 2022, x64 Release.

Local evidence logs (outside the repository, in the task's `outputs/` folder):
`N1_baseline_reproduction.log`, `N1_related_regressions.log`,
`N1_all_tests_build.log`, `N1_remaining_regressions.log`,
`N1_processor_layout_rerun.log`, and `N1_processor_final_ctest.log`.
MIDI diagnostic output is in `N1_midi_range_diagnostic.log` (elapsed time and
exit status also recorded in this report).
ROM-containing fixtures and build artifacts are not published.

The active work ledger and remaining release gates are in
[the execution plan](../release/EXECUTION_PLAN_1.0.md).
