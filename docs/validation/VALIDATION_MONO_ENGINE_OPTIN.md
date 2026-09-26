# MONO correction: explicit engine opt-in, processor validation

2026-09-24. Baseline local `6e3f76ba07021100f60143568c4d4e2f683cb3df`,
remote Draft #47 `4e9f96db49629a0624e75f2d545ff3472129e8d2`, matching tree
`ebedcd059b0d98d0c4f646a37aa99ac8a78a93b4`.

This is an integration stage, NOT a complete persisted/user-facing product mode
or closure of P1. Native default acceptance stays a real failing test.

## Engine boundary

`configureMonoCorrectionBeforeLoad` is engine-owner-only and rejects changes
after a ROM is loaded. There is no live switching, GUI option or automation.
It defaults false. A request alone cannot activate the correction: each accepted
ROM load checks the whole known-v1.8 compatibility fingerprint and six decoded
branch instructions/operands before activation. Unknown firmware falls back to
native execution. The fingerprint is not a cryptographic security boundary.

Both engine stepping sites (boot and audio generation, including existing
drain paths) now call one `stepFirmware`. Only a verified, opted-in MONO machine
at one of the six sites reaches the pure decision helper. Table reads are
bounds/alignment checked; the helper changes only Z before the ordinary core
step, matching the isolated experiment. No input transposition, occupancy/count
patches, ROM edits, additional CPU steps/render budgets, allocation or locks are
introduced. Changed branch paths are not a claim of native cycle equivalence.
This is disclosed compatibility behavior, not a CPU-emulation correctness fix.

Native construction remains unchanged apart from the disabled-hook branch.
The installed VST3 is untouched. No persisted project option is claimed: a new
ordinary processor still defaults native. Save/restore of correction intent,
user-facing selection, safe held-note transitions and unavailable-ROM intent
are required next, before this can be delivered as an end-user feature.

## Actual processor checks

`--mono-corrected-processor-only` initializes a real VDX7AudioProcessor with
engine opt-in before loading, then uses its ordinary processBlock MIDI/audio
path. The existing callback allocation probe and finite-output checks remain.

- Verifies initial inactivity, known-image activation and rejected live changes.
- Six branch-displacement rejection controls restore each test mutation BEFORE
  any CPU execution; the final known-image check also verifies unchanged ROM.
- 32 sequential zero On/Off pairs, per-On allocation and per-Off complete
  ownership cleanup, without reset/mode recovery between them.
- Zero On -> 60 On -> 60 Off returns to zero's firmware target; final zero Off
  clears ownership and audio. This target check is not full audible portamento
  transition characterization.
- +12 single-sine patch: native POLY zero reference, corrected MONO zero,
  intentionally substituted one with unchanged zero expectation, and opted-in
  POLY zero. Audio pitch comparison has the same 0.2% tolerance for positive and
  negative controls. Every input must sound and subsequently release to silence.
- Native POLY 72 compared with corrected MONO 72 after zero history/legato,
  including genuine pitch measurement and final release.

These are six processor fixtures at 48 kHz / 64 samples. Frequency uses
interpolated positive zero crossings in the last second of a two-second render.
They do not replace the isolated broader ordering/capacity matrix, complete
sample-rate/buffer coverage, physical hardware or actual DAW acceptance.

## Validation results

- Rebuilt local ROM-enabled `vdx7_all_tests` and ROM-free `vdx7_ci_checks`.
- Full local macOS arm64 Release run, including desktop-dependent GUI tests:
  **27/28 PASS, 314.03 s, CTest exit 8**. Only unchanged native
  `vdx7_mono_note_zero_acceptance` fails (MIDI/held/MONO 0/1/16).
- New corrected processor group **PASS, 3.14 s**; isolated candidate PASS29.82 s.
- ROM-free runtime **7/7 PASS, 1.31 s**. This does not run private-ROM acceptance.

Measured processor output (patch transpose +12, 0.2% relative pitch tolerance):

| Path/input | Measured Hz | Check |
|---|---:|---|
| Native POLY Note 0 reference | ~16.3524 | Sound and release |
| Corrected MONO Note 0 | ~16.3524 | Positive pitch PASS |
| Corrected MONO Note 1, expecting Note 0 | ~17.3357 | Audio mismatch rejected: negative-control PASS |
| Opted-in POLY Note 0 | ~16.3524 | Reference match |
| Native POLY Note 72 reference | ~1046.56 | Sound and release |
| Corrected MONO Note 72 after zero history | ~1046.56 | Reference match and release |

The rejection is from audio frequency, not an expected-key metadata mismatch.
All corrected MONO fixtures also exercise the 32 repeated pairs and legato.
The ordinary C++ callback-allocation probe passes; this is not a full-plugin
sanitizer, worst-case callback timing or real DAW acceptance result.

## Remaining gates (P1 stays open)

### Follow-up: host rate/block matrix (2026-09-24)

Expanded the same six processor fixtures to 44.1/48/96 kHz and 64/256 samples:
36 fixtures total. Settling periods now preserve elapsed time (rounded up to
whole host blocks); pitch measurement uses the actual sample rate and the final
approximately one second of a two-second render. The 0.2% comparator, ownership,
32-repeat and legato checks, release silence and negative control are unchanged.

Rebuilt `vdx7_host_reset_tests`. Focused CTest: profile PASS, corrected matrix
PASS (19.85 s), unchanged native acceptance FAIL (0/1/16, 0.17 s).
Combined result 2/3, exit 8, 20.68 s. No full-suite rerun in this test-only round.
This expands processor coverage, not GUI/persistence/lifecycle or DAW acceptance.
Prior PR head macOS/Windows checks were green; these local changes need new CI.

Persisted mode/UI and transition policy, reset/restore/ROM-reload cases with
correction enabled, broader corrected processor ordering/capacity/pedal tests,
nonzero-only differential controls and measured callback overhead remain open.
An initial processor PASS cannot close these or make native acceptance PASS.
Keep #47 Draft; no main merge, installed binary replacement, tag or release.

### Follow-up: held-zero lifecycle (2026-09-24)

Three independent 48 kHz/64-sample corrected MONO fixtures now hold Note 0
across (1) host reset, (2) release/prepare, and (3) same-instance state restore
using the saved ROM path (therefore also reloading that known image).
After each transition, actual MIDI/held/sustained/MONO ownership and sound clear,
the correction remains active, saved voice/performance/parameter/export state
is unchanged, and fresh Note 0 then 72 each sound, retain the original key and
release to silence. Final firmware profile is unchanged. Existing callback
allocation and finite-output checks apply. Fixtures do not repair each other.

Rebuilt host-reset executable. Focused profile + native acceptance + corrected
suite: 2/3 PASS, exit 8, 22.36 s. Corrected lifecycle plus 36 rate/block/pitch
fixtures PASS 21.52 s. Unchanged native acceptance FAIL 0.15 s (0/1/16).
No full-suite rerun or DAW test in this test-only round.

This is NOT correction-option persistence: the same instance already opted in
before load. New-instance restore, missing/unknown ROM policy, serialized intent
and UI selection are still unimplemented. Live switching remains rejected.
