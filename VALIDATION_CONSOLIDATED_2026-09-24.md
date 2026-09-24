# Consolidated validation — 2026-09-24

Source: local e43c9b8eb5d9dcddf560c0683efef8a4defec66a; equivalent GitHub
commit c8d413c666ba7fcabd4a9e46435c06cb80e45ccd. These commits have identical
source trees. No production code was changed during this validation.

## Automated suite

All test targets were up to date (`vdx7_all_tests`: no work to do).
Full CTest: 27/29 passed in 403.45 s, exit 8. Two failures:

- `vdx7_processor`: desktop access unavailable in the sandbox. A separate
  desktop-enabled rerun passed 1/1 in 9.08 s. Do not rewrite the original run.
- `vdx7_mono_note_zero_acceptance`: native MONO Note 0 release still fails,
  MIDI/held/MONO = 0/1/16. This is an actual retained failure, not an inverted
  expected-success test.

Across the full run and explicit environment retry, 28 distinct tests passed;
one native acceptance test remains failing. Corrected processor acceptance
passed in 54.76 s, including both two-instance isolation cases. This is not
an all-green release gate.

GitHub check-runs for the above remote commit: build-macos and build-windows
completed successfully. Windows runtime/DAW acceptance is not implied.

## REAPER host smoke checks

REAPER 7.80, local arm64 VST3, SSL 2+ device. Loaded module path was checked
against the workspace build. Executable SHA256:
437da776c521f6f68b2d67de93933938b3b856d912046bf13cd35933eb4e6154.
Later commits change tests/documentation only; this production build is unchanged.

Saved a separate multi-instance test project and duplicated the corrected MONO
track via REAPER's UI. Saved RPP verified four, then eight VST entries.
All tracks play the same repeated Note 0, legato-order and subsequent Note 72
sequence. Each render includes a three-second tail. Four/eight-instance online
renders completed at 44.1 kHz / 512 samples.

The first four-instance render clipped the summed master (+1.9 dB). Retained
as a rejected gain-staging trial. Lowered only the test project's master to
-18 dB and rerendered: four-instance peak -16.4 dB, zero clipped samples.
Eight-instance trial also completed with zero clipped samples. Both had
nonzero audio in later Note 72 windows and exact silence at 13–14.9 s.

These short renders do not prove absence of dropouts, individual-instance
pitch accuracy, full polyphonic worst-case performance, or physical MIDI input.
The RPP, ROM and factory voice state remain local and are not distributed.

The eight-instance Online Render matrix covers device requests of
44.1/48/96 kHz crossed with 64/128/256/512 samples. REAPER's device status
was checked after each change, and render sample rate matched the device.
The output files use `mono-8instances-RATE-BLOCK-online.wav` names. A local
PCM analysis checks later-note windows 4.45–5.2 and 11.05–11.85 seconds,
tail 13–14.9 seconds, and full-scale clipped sample counts. These are
smoke criteria, not an xrun/latency or dropout oracle. No CPU benchmark
claim is made while other validation work is running.

All 12 matrix renders completed (approximately 14.912 seconds each):

| Device rate | 64 samples | 128 samples | 256 samples | 512 samples |
| --- | --- | --- | --- | --- |
| 44.1 kHz | PASS | PASS | PASS | PASS |
| 48 kHz | PASS | PASS | PASS | PASS |
| 96 kHz | PASS | PASS | PASS | PASS |

PASS here means completion, nonzero audio in both subsequent-note windows,
zero full-scale clipped samples, and exact zero in the measured tail.
It does not certify pitch, legato priority, absence of intermittent dropouts,
or that every individual instance produced correct audio in the summed output.
The final 44.1 kHz / 512-sample render had window peaks 0.2274706364 and
0.2238430977, tail peak 0, and zero clipped samples.

Restored the original REAPER device preferences after the matrix: sample-rate
request disabled (field 48000), block-size request disabled (field 512).
The device status again showed 44.1 kHz / 512 samples. Saved the separate
eight-instance test project; the user's other project was not edited.

## Remaining acceptance boundaries

Physical sustain/portamento pedal operation, Windows and Intel Mac host runs,
long-duration performance, external MIDI jitter, sanitizer runs and release
packaging/signing are not established by these checks. Native versus corrected
mode publication policy still requires an explicit decision. Do not mark the
complete release checklist passed from these limited results.
This pass also does not establish full GUI scaling acceptance, DAW automation,
play/stop/seek/loop edge cases, or eight-instance saved-project reopening.
These remain separate work items rather than implied passes.
