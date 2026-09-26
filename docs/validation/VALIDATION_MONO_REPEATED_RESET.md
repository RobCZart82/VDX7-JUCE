# Corrected MONO repeated-reset acceptance

Baseline remote PR #47: `3cf3a2d56ff3eacdf9df33e92007422508f8351a`.
This round changes regression tests only, not the plugin or installed build.

User feedback: the latest installed build showed no perceived fault during
general REAPER use. Correction setting, exact build and deliberate Note 0 input
were not established for that user session. This is positive general-use
feedback, not targeted Note 0 acceptance.

## Added processor regression

One corrected MONO instance runs 12 consecutive cycles at 48 kHz / 64 samples.
Each cycle establishes sounding Note 0, then a velocity-zero release with
sustain still engaged. A public processor reset must clear firmware and adapter
ownership and leave silence. Late Note Off and sustain-release events from the
interrupted history must remain harmless. Fresh Note 0 and Note 72 must each
sound, own exactly one MONO entry, and release to empty ownership and silence.
Note events exercise first/last sample positions. The same instance is reused
without reinitialisation across cycles. Existing allocation and finite-output
checks remain active.

This is NOT a REAPER stop/start, seek or loop test: a host is not guaranteed to
call reset on those actions. Those host cases, pitch measurement for this new
fixture, long-duration load and physical pedals remain separate acceptance work.
REAPER was inspected but no transport acceptance was completed in this round.

Commands: build `vdx7_host_reset_tests`, then CTest regex
`^vdx7_mono_corrected_processor$` (also includes the required profile fixture).
Build exit 0. Targeted run: 2/2 PASS, exit 0, 58.91 seconds; corrected group
58.11 seconds. This is not a full-suite rerun. Native acceptance was not run
and its known failure remains unchanged. No new reproduced defect was found.
Test executable SHA256:
`176d3ced256e2c407da7fc73b155ebfd28de45c305be994f84538a720e57ebf0`.
