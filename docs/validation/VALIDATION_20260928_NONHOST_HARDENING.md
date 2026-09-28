# Non-host release hardening — 2026-09-28

Baseline: `811f3a3ccaedbda3310134407000b0bf36c08504`, after PR #87 merged.
PR #87 Windows run 36397313250 and macOS run 36397313480 both completed PASS.
Those are prior-source results, not acceptance of this follow-up patch.
The merged baseline itself also has Windows run 36398489910 and macOS run
36398490090 PASS (verified from the live Actions API on 2026-09-28).
No REAPER, installed-plugin replacement, public ROM, tag or release is involved.

## Windows test budgets

Sequential rerun without concurrent compilation or other agent-run tests:
`vdx7_midi_range` FAIL (60.02 s timeout), profile PASS,
`vdx7_mono_corrected_processor` FAIL (120.02 s timeout).
Direct unchanged MONO executable then completed PASS, exit 0, in 130.88 s.
The preceding round's unchanged MIDI executable completed PASS in 292.44 s.
These are CPU emulation matrices, not wall-time realtime-performance assertions.
Windows-only limits are 600 s for MIDI range and 300 s for corrected MONO,
approximately twice the observed completion times. Other platforms keep their
existing limits. No samples, scenarios, assertions or failure/skip policy are
removed. Final CTest verification is recorded below when complete.

The first full follow-up run additionally hit `vdx7_host_reset`'s 180.02 s
budget while progressing through the combined history matrix (last output:
96 kHz / 256 samples); no assertion failure was printed. Record this as FAIL,
not PASS. With the Windows-only 360 s budget, the unchanged-scenario rerun
completed PASS in 176.16 s; sample-based reset deadlines remain in the test.
For context, the preceding round's `N1_related_regressions.log` recorded the
same aggregate case PASS in 169.56 s, already close to its 180 s limit. This
comparison is not a controlled performance benchmark or proof of no regression.

## Dated audit policies

- N2: validate semantic voice ranges in all 32 USER slots, since full-bank
  import copies all 32 slots. Occupancy continues to control patch-name checks.
  Reject malformed files transactionally; do not normalize or overwrite them.
- N3: validate the 32 packed voices in project RAM before any state mutation.
  Preserve reserved bits and firmware working RAM. Reject invalid packed voice
  ranges, not legacy state merely because it lacks the modern parameter tree.
- N4: distinguish product support (16 KB firmware, optional factory companion,
  or 48 KB combined image) from the complete test suite (48 KB combined v1.8).
  Fail the common profile fixture clearly before dependent tests run.
- N5: check the entire ROM-on and ROM-off inventories, labels, fixture edges,
  positive timeouts and absence of skip/disabled policies. No ROM is used by CI.
- N6: owner decision on 2026-09-28: keyboard PITCH adjustment retains its value.
  Mouse spring return remains unchanged. This is an accepted policy, not a bug.
- N7: show pending-project identity mismatch AND ignored optional-companion
  warnings together. Neither should silently erase the other.

## Verification ledger

Windows x64, MSVC 2022 Release, local dependencies pinned by CMake. Base is the
SHA above; follow-up branch is `fix/rc-nonhost-hardening-20260928`.

| Check | Result | Evidence/scope |
| --- | --- | --- |
| New N2/N3/N7 regressions before fixes | FAIL (expected) | All three independently failed; profile passed, `RC2_reproductions.log` |
| Fixed targeted regressions and adjacent state/reload/stability checks | PASS 7/7 | `RC2_fixed_regressions.log`, 35.53 s |
| Complete test executables, VST3 and Standalone compile | PASS | `RC2_final_build.log`; no installation, existing C4805 warnings remain |
| Actual ROM-on/ROM-off configurations and complete inventories | PASS | 36/10 registrations; `RC2_registration_final.json`, `RC2_registration_rom_off.json`; checker and seven negative controls PASS |
| 16 KB full-suite profile negative control | FAIL (expected) | `RC2_firmware_only_profile.log`; clear 48 KB requirement, not product rejection |
| Product 16 KB load with invalid companion | PASS | N7 regression; both pending identity and ignored-companion messages retained |
| Full sequential 36-case CTest run | FAIL overall: 35 PASS, 1 timeout | `RC2_full_ctest.log`, 1285.99 s; only aggregate host reset reached 180.02 s |
| Host reset rerun with Windows 360 s budget | PASS 2/2 | `RC2_host_reset_rerun.log`; reset 176.16 s, 176.25 s overall including profile |

The full run's MIDI-range test passed in 289.85 s, corrected MONO processor in
127.80 s, and MONO soak in 100.19 s. All ten ROM-free tests passed. The timeout
rerun is separate evidence, not a retroactive green label on the first full run.
Latest per-test outcomes therefore cover all 36 unique tests with PASS,
combining the 35 passing full-run cases and the reset rerun. A second single
36-case all-green invocation was not run; no assertion or scenario was removed.

Reproduction used the new tests on otherwise unchanged baseline product code.
After the fixes, the focused seven were `user_bank`, `v18_profile`, `stability`,
`processor`, `direct_rom_reload_boundary`, `state_rom_identity` and
`pending_rom_content_identity` (all prefixed `vdx7_`). Build/test equivalents:

```text
cmake --build <build> --config Release --parallel 1
  --target vdx7_all_tests VDX7_VST3 VDX7_Standalone -- /nr:false
ctest --test-dir <build> -C Release --output-on-failure --no-tests=error
ctest --test-dir <build> -C Release -N --show-only=json-v1
python scripts/check_test_registration.py <inventory.json> --self-test
```

No simultaneous heavy build or second test suite was run during the final
sequential suite. The new optional macOS ASan/UBSan workflow covers seven
non-GUI ROM-free components. Its first remote runtime result is pending;
configuration text alone is not sanitizer PASS. LeakSanitizer is not enabled.

## F11/F13 — invisible GUI resource cleanup

Removed only the five unused editor image members/decodes: chassis, LCD frame,
panel, envelope grid and divider. Kept the used value-field image and all source
artwork. The historical manifest now points to
[the current runtime resource contract](../design/GUI_RUNTIME_ASSETS.md).

Optional `vdx7_gui_header_tests --benchmark-editors N` creates 0/1/4/8 no-ROM
processors and editors in fresh processes, then holds them for one second.
Local helper sampled Windows process working set every 25 ms. Three runs each;
table is median sampled peak working set and total processor+editor creation
time, not retained private bytes, DAW RSS, audio CPU or a cross-platform promise.
0 editors is the process baseline. No compilation ran concurrently.

| Editors | Before MiB | After MiB | Before ms | After ms |
| --- | ---: | ---: | ---: | ---: |
| 0 | 13.38 | 13.38 | 0.072 | 0.091 |
| 1 | 40.16 | 31.72 | 161.099 | 119.850 |
| 4 | 65.91 | 37.02 | 316.245 | 152.251 |
| 8 | 100.64 | 44.23 | 536.506 | 201.038 |

Logs: `RC2_editors_before.json`, `RC2_editors_after.json`. GUI regression PASS
before/after for all five fixed sizes and About assets. Default 1200-wide PNG
snapshots are byte-identical: SHA-256
`8301b54e1740e0c4308f09d95c6be1fd28cde217950cea3aed7ef16b165246d3`.
This is component/snapshot evidence, not Settings interaction or desktop HiDPI.
Interactive Standalone Settings/About follow-up: NOT RUN. The Windows
computer-use launch approval timed out before a target window was available;
no UI automation bypass was attempted. No existing Standalone settings file
was found during the pre-launch read-only check. Desktop/HiDPI acceptance
remains open despite the successful automated five-size and pluginval checks.

## Optional VST3 compatibility gate

Official [Tracktion pluginval 1.0.4](https://github.com/Tracktion/pluginval/releases/tag/v1.0.4),
portable Windows download SHA-256
`c08e61ce3b96db41636f8ec7e76f4c7e2c13ebdac7fa1b5a1f52b4f32ec715ab`.
Only the workspace-built VST3 was passed to it; installed plugins were untouched.

```text
pluginval --strictness-level 5 --random-seed 20260928 --timeout-ms 60000
  --output-dir <private-logs> --output-filename RC2_pluginval_details.txt
  --validate <workspace-build>/VDX7_artefacts/Release/VST3/VDX7.vst3
```

PASS: completed detailed log ends `SUCCESS`, including cold/warm open, editor,
editor while processing, state, automation, buses and processing at
44.1/48/96 kHz with 64/128/256/512/1024 samples. No GUI skip was requested.
Windows launcher returned before its worker completed; result is based on the
completed validator log, not that launcher exit alone. No explicit private ROM
fixture was supplied or verified in this validator, so this does not establish
audible firmware synthesis. Separate Steinberg VST3 validator: NOT RUN (not
provided; pluginval reports that subtest skipped). This is not REAPER acceptance.

REAPER/other DAW, physical MIDI/audio-device behavior, final RC and release
acceptance: NOT RUN. Automated processor evidence must not close those gates.

Local logs are retained in the task's `outputs/` folder, outside the repository.
ROM fixtures remain private and outside the repository.

## Handoff / publication state

Local implementation checkpoint: `5203fd9663d03bef1f83bc5234a764b1ca9ecaaa`,
tree `ac27dc74e3da7e4f97f2ae4c25d864c9f800455c`, on
`fix/rc-nonhost-hardening-20260928`. Subsequent local documentation records
this handoff. Main was re-fetched and remained `811f3a3` before publication.

Initial GitHub upload: BLOCKED by automatic safety review. The attempted source-tree
upload was rejected because authorization/trust for the external payload was
not considered established. No alternate upload route was attempted. The owner
subsequently explicitly approved the 24-file upload to this branch and creation
of a Draft PR against main on 2026-09-28. No ROM or binary is in that diff.

At the local checkpoint, new Windows/macOS and sanitizer Actions were NOT RUN.
Publication and subsequent remote outcomes are tracked in the associated Draft
PR and its Checks; do not infer PASS from permission to upload.
Earlier main/PR #87 PASS results do not apply to this new source. The new
sanitizer workflow is prepared but unverified remotely. No merge, release,
tag, asset replacement or installed-plugin replacement was performed.

Next: re-fetch main and reconcile incoming changes;
publish the tested content, compare remote/local trees, open a Draft PR and
verify all three Actions workflows. Interactive Settings/About still needs
desktop-app access approval. Final packaging/RC and host gates remain separate.
