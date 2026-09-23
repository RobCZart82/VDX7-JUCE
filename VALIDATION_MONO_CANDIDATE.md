# MONO Note 0: isolated corrective experiment and visible product gate

2026-09-23. Baseline local `e8bb6914fe2169f44ad1ecf5d81c4ad1191e1bdb`,
remote Draft #47 `aa5fc05eddd6dfc477af0ddb45880dfc166ea8af`, matching tree
`5be1b35d58154e30a3475b33437b30db9a144f32`.

**P1 remains OPEN. This is a reproducible design experiment, not a production
fix, ROM patch or corrected CPU implementation. The installed plugin is untouched.**
The supplied review usefully requires Note 0 itself to sound, not only that
ownership eventually disappears, and requires failing acceptance to be visible
in the combined test result. Both are implemented here.

```text
Normal plugin MONO acceptance: FAIL — known, not yet repaired defect
Isolated correction:         separately tested experimental execution
Integrated plugin fix:       not implemented
```

The first line is the pre-existing failure becoming visible, not a new regression
caused by the unlinked experiment. It must remain a real failing test.

## Experiment boundaries

`Tests/VDX7MonoCandidateTests.cpp` is a standalone executable linked only to the
pinned core and voice-data helper. VDX7 does not include it. It requires the same
private known-v1.8 image fingerprint as existing ownership tests and verifies
decoded instruction sites. It loads our own fast-release single-sine patch,
not factory voices. Three in-memory changed-image controls must be rejected
before boot. ROM contents are checked unchanged after every pump.

The test deliberately overrides Z immediately before selected branches. The
pinned core executes each instruction before its interrupt checks; intervening
interrupts therefore preserve the branch input without editing stacked flags.
No note transposition/suppression, direct voice-table/count clearing, automatic
mode cycling or firmware-byte edit is used as a correction. Mode selection and
patch setup are ordinary explicit fixtures. These condition interventions change
execution and are NOT firmware-faithful/native behavior or cycle-equivalence
evidence for a deployable correction.

The candidate uses the active flag in the second slot byte, not the pitch byte,
to define MONO occupancy. Release additionally requires a valid occupied matching
slot and a successful search. The shared search intervention is MONO-only.
One common invariant applies throughout: an empty slot and an occupied slot
containing MIDI key zero are different states, including allocation, empty-slot
skipping, matching-key lookup, release/counting and legato return selection.

## Incomplete proposals reproduced

1. Correct allocation at `D593` and found/release status at `D645` only:
   two zero Note Ons now occupy two slots, but after two Offs counts remain
   MIDI/held/MONO **0/1/1**. The second lookup stops at the already cleared first
   slot because an empty slot also contains a zero key.
2. Also exclude empty matches at `D6A8`: ownership cleanup improves, but
   **0 On -> 60 On -> 60 Off** does not restore zero's target pitch. The legato
   search still skips the remaining active zero entry.

Thus the original allocation/release diagnosis is valid but not an exhaustive
list of condition sites. The coherent experiment additionally covers first
active event (`D6BB`), highest (`D6CE`) and lowest (`D6E0`) key scans. Every one
of the six candidate decision sites must be exercised with a changed outcome.
This is an implementation hypothesis verified in the local test machine, not
independent confirmation of physical hardware or every emulator instruction.

## Acceptance-oriented experimental checks

- Isolated 0/1/60/72/127: actual held key, target pitch, nonzero sine output and
  interpolated frequency compared with unmodified native POLY references;
  final fast-release output and ownership are zero. Candidate-enabled POLY
  makes zero interventions and matches reference peak/frequency exactly.
- Neutral-transpose Note 0 and Note 1 both yield approximately 8.483 Hz in this
  specific firmware/patch fixture, not necessarily all patches or original
  hardware. An initial requirement that they differ was invalid for
  that reference, not proof of correction. A **+12 patch-transpose control**
  additionally distinguishes them (~16.3525 versus ~17.3356 Hz), with incoming
  and held keys still 0/1. This prevents a pitch-one substitute from being accepted
  by an insufficient low-end frequency oracle. It is not a production workaround.
- 20 histories: 1/2/16/17/32 zero notes, stacked or sequential, ordinary Off or
  velocity-zero On. Then genuine pitch 72 (~523.278 Hz) and release, without
  reset/program/mode recovery; target/frequency must match the native POLY 72
  reference. Every sequential On must own/play zero and every Off must clear it.
  Stacked Ons must fill the available min(repeats, 16) zero-key slots, not drop
  the input. Settled active-slot/count consistency is checked after every pump
  in the complete candidate.
- 36 three-key histories: all permutations of 0/60/127, each first release and
  both remaining release orders. Highest/lowest priority and target pitch,
  table holes, final silence and unmatched zero Off are checked.
- 16 mixed zero/60 scenarios: both arrival/release orders, portamento off/on,
  sustain off/on. Verify remaining target, intentional sustain versus unwanted
  held output and final pedal release. This is not all portamento modes/timings.

## Follow-up (2026-09-24): deliberate Note 1 substitution checks the audio oracle

Baseline local `4951d0d3a6d0b3bcbfd08e0c2bb6c57afa302cf6`, remote
`7f23761e9312919d9f88c3b7000b3b3f2a4a1783`. Only test code/documentation changes.

The SAME `requireAudioPitch` function is now used for positive shifted-patch
references and the negative control. Reference is unmodified native POLY Note 0
with the +12 patch. Candidate fixture, rendering duration and 0.2% relative
frequency tolerance stay unchanged. Only the negative control's input is Note 1;
the expected key/pitch remains Note 0. No production input is changed.

| Input to candidate | Expected/reference Hz | Actual Hz | Relative error | Audio oracle |
|---|---:|---:|---:|---|
| Note 0, unchanged | ~16.3525 | ~16.3525 | 0.000356% | PASS |
| Note 1, deliberate test mutation | ~16.3525 | ~17.3358 | 6.01296% | FAIL |

Both inputs must first produce finite, non-silent, measurable sound, so absence
of audio cannot masquerade as pitch discrimination. Audio frequency is then
checked BEFORE held-key/target metadata. The sensitivity test
accepts only the dedicated `PitchMismatch` failure as evidence; invalid ROM,
setup/input errors or metadata failures cannot count as a successful negative
control. If the mutant passes the audio oracle, no metadata check masks it:
the sensitivity test fails. The normal experiment includes this positive/negative
pair automatically, while a standalone mutant invocation returns a real exit 1:

```sh
vdx7_mono_candidate_tests /absolute/path/to/private/dx7.bin --pitch-oracle-only
vdx7_mono_candidate_tests /absolute/path/to/private/dx7.bin --pitch-oracle-note-one-mutant
```

First command: positive PASS + negative explicitly REJECTED, exit 0.
Second command: `FAIL: rendered audio pitch does not match the expected note`,
exit 1. This is a deliberate input-mutation diagnostic, not a WILL_FAIL test and
not the normal plugin's separate known MONO failure. Neither native acceptance
nor production behavior is modified.

Follow-up validation after the final oracle changes:

- Rebuilt the ROM-enabled candidate and ROM-free `vdx7_ci_checks` targets.
- Fixture + complete candidate + native acceptance: candidate **PASS 30.63 s**,
  native acceptance **FAIL 0.14 s**, still MIDI/held/MONO **0/1/16**.
  The three-test invocation returns **CTest exit 8** (30.79 s).
- Standalone Note 1 mutant: the measured 6.01296% mismatch produces the dedicated
  audio-pitch failure and real **exit 1**. ROM-free runtime: **6/6 PASS, 0.93 s**.
- The full 26-test suite was not rerun for this test/documentation-only change;
  its earlier result is recorded separately below. No production source, CMake,
  ROM or installed plugin was changed. P1 remains OPEN.

## Product gate — not a green characterization substitute

The original `--mono-note-zero-only` diagnostic and its assertion are unchanged.
It is now registered as `vdx7_mono_note_zero_acceptance`, with `release-blocker`
label and required known-ROM fixture. There is **no WILL_FAIL, skip, expected
failure conversion or hidden exclusion** in the ordinary full local invocation.
The combined local result must fail while the product defect persists.

The two older known-bug characterization tests and the new candidate experiment
are explicitly labelled. Their PASS does not satisfy the product gate. Public
Actions compile the experiment but still run only ROM-free tests; the private
ROM is never uploaded. Historical 24/24 counts excluded the failing acceptance
diagnostic and must not be presented as the new full-suite result.

## Validation

- ROM-ON `vdx7_all_tests` and ROM-OFF `vdx7_ci_checks` compile/link, including
  the new standalone candidate executable. No production source changed.
- Full local macOS arm64 rebuilt suite (desktop access for existing GUI tests):
  **25/26 passed in 307.58 s; CTest exit 8**, with only
  `vdx7_mono_note_zero_acceptance` failing (MIDI/held/MONO **0/1/16**).
  Candidate experiment passed in 28.96 s. The two existing characterization
  groups passed; this does not reinterpret their documented failures as fixed.
- After further strengthening only the experiment's per-event/stacked allocation,
  native Note 72 reference and changed-image rejection assertions, rebuilt both
  configurations and reran its fixture + experiment + original acceptance:
  **experiment PASS 29.33 s; original acceptance FAIL 0.14 s**, combined exit 8
  (29.49 s). Other production/test source from the full run is unchanged.
- Public-CI-style ROM-free runtime: six tests pass. The private-ROM experiment
  and failing product gate are NOT executed there. No full-plugin sanitizer,
  callback-cost, physical-hardware or real-host acceptance claim is made.

No option/state format, installed binary, ROM bytes or default behavior is
changed in this chapter. Prior-head macOS/Windows Actions passed; an uploaded
new head needs its own checks. Keep #47 Draft and P1 OPEN.

## Required next work

Choose and integrate a clearly disclosed optional correction, with native
default/legacy-project fallback and strict known-image guards. Resolve held-note
option transitions, every production CPU stepping path, save/restore/reset/mode
changes and unknown/missing-image behavior. Then run the corrected **actual
processor** through playback, ordering, capacity and lifecycle acceptance, plus
callback-cost/real-host checks. Raw-core candidate success alone cannot close P1
or authorize main merge/release. See `DESIGN_MONO_NOTE_ZERO_POLICY.md`.
