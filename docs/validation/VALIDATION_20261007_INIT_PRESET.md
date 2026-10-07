# Init Preset development validation

Date: 2026-10-07. Baseline main: `ba55afb76cc3184d91357bdaec989365a20c6caf`
after documentation PR #151. This is development work after the published 1.0.1,
not a release acceptance or replacement of its assets.

## Behaviour

UTILITY → Init Preset always asks for confirmation. Only the current editable
RAM voice is replaced. The immutable factory catalog and saved USER files are
not overwritten; other working slots and PERFORMANCE/SETTINGS remain unchanged.
Existing unsaved edits to the current working copy must be saved separately
before confirmation. Cancel does not initiate a replacement. A changed voice,
selection or publication revision makes the confirmation stale and rejects it.
The async callback uses an editor SafePointer, including late answers after close.
Voice bytes, selected program and revision are captured together under the
processor engine lock, so a concurrent program change cannot mix confirmation
data from different voices. Identical voice bytes and a selection round trip
do not make an old confirmation valid.

The VDX7-owned seed from #150 has a 10-character stored name `Init Prese`.
Display-only per-slot provenance expands this to `Init Preset`; existing dirty
tracking adds the star. Export remains standard DX7 SysEx without a star.
Provenance travels with project RAM, including missing-ROM pending project
resaves, and defaults to zero for older states. Malformed/out-of-range metadata
is rejected before replacing project state. Ordinary imports and factory/USER
loads clear the appropriate provenance and are not identified by name alone.

No parameter IDs/order, plugin identity, firmware admission, core DSP or main
panel layout changed. This is not a full Settings reset or Yamaha INIT dump.

## Local results

- PASS: fresh Apple clang C++20 shared-code/processor/editor and core build,
  followed by fresh ASan/UBSan rebuild of `vdx7_ci_checks`. Pinned cached JUCE
  and Retromulator sources were compiled, not a previous core binary.
- PASS: all 15 ROM-free CTests with ASan/UBSan, including voice-data, direct ROM
  diagnostics, GUI header and pre-ROM processor state. Leak detection disabled;
  no leak-test claim.
- PASS: all 78 Python packaging/release fixture tests. These do not accept any
  new public release payload.
- PASS: local opt-in Init processor test with ASan/UBSan and private original
  v1.8 firmware, SHA-1 `715dbb8e96a4df2a7f096b368334a7654860bb26`.
  It covers confirmed/stale actions, exact seed data, other slots, synthetic
  eight-bank catalog, isolated USER-file preservation, globals, instance
  isolation, project/pending recall, legacy state, export and separate USER save.
  At 48 kHz / 512 samples it also checks finite output on both channels,
  measurable note-on and note-off retirement using the VDX7-generated seed.
- PASS: local opt-in real plugin dialog test with ASan/UBSan: Cancel preserves
  serialized project state; confirm produces the friendly dirty LCD title;
  a late confirmation after editor destruction safely does nothing.
- PASS: seed regression checks every operator/global voice parameter, instance
  independence and a single-voice SysEx round trip.

The firmware is read from an existing local file only. Fixtures use synthetic
patch/catalog data and isolated temporary USER files. No firmware, factory
patches, recorded audio, installed plugin or real DAW project are committed or
modified. No REAPER instance was launched.

## Initial implementation merge checks

The initial PR Windows/macOS runs failed their registration smoke checks:
the strict CTest inventory did not yet include the two new opt-in Init tests.
The inventory now declares both tests without weakening the ROM-free/private
fixture policy. Actual local ROM-enabled/private-bank and ROM-free CTest JSON
inventories pass the checker, including its negative controls. These are
registration checks, not private factory-bank integration execution.
The review's mixed-capture race is corrected by the locked working-voice
snapshot; the local processor regression covers identical-byte selection
changes and returning to the original program before confirming.

The corrected #152 branch passed Windows, macOS and hosted sanitizer checks
(37638893074, 37638893142 and 37638893083); its review thread was resolved.
It merged as `269e2305423de4410bbf30022aeb22373e531e59`. Platform workflows cover
the VST3/AU/Standalone format builds; the local build above is shared code and
test runners. The initial local round did not cover the extended matrix below,
real Windows/macOS REAPER acceptance or release packaging/publication.

## Extended audio validation

The next test-only round starts from merge-main `269e2305423de4410bbf30022aeb22373e531e59`
after #152. The #152 branch Windows/macOS/sanitizer checks all passed and the mixed
confirmation capture review was resolved. Merge-main Windows and macOS checks
also PASS (37641092380 and 37641092371). No production DSP, plugin/parameter identity, GUI, admission
policy, release asset or installed plugin changes in this round.

The local instrumented processor matrix covers all 12 combinations of
44.1, 48, 96 and 192 kHz with 32, 512 and 2048 sample blocks. Each case checks
finite output and measurable note-on on both channels, a positive sustain
control after note-off, confirmed Init with simultaneous held/sustained notes
and intervening audio callbacks, unchanged PERFORMANCE/SETTINGS, quiet output
after releases, a subsequent fresh note, and edited Init serialization/recall
with dirty provenance and subsequent note-on/release.

The direct matrix uses the private 16 KiB v1.8 ROM with SHA-1
`715dbb8e96a4df2a7f096b368334a7654860bb26`; all 12 cases PASS with ASan/UBSan.
The newly supplied original maskrom has a different image/version (1.6,
SHA-1 `ce4df31878dda9ec27b31c7bc172f16419264b90`) and is not counted as
v1.8 evidence here. The original ROM files remain unchanged.

PASS: 15/15 ROM-free CTests and 78 Python fixture tests after the matrix addition.
PASS: actual ROM-free and local-ROM CTest JSON inventories, both with and without
the separately declared private-bank test, and registration negative controls.
The public platform CI compiles this runner and verifies registration but does
not execute its private-ROM matrix. It is an explicit local test with a 240 s
timeout and the existing required `vdx7_v18_rom` fixture, not a silent skip.

The first registered run with only the 16 KiB file stopped at the existing
suite-wide profile precondition requiring a 48 KiB combined image; the three Init
tests were NOT RUN in that attempt. For registered execution, a separate private
temporary fixture combines the identical verified v1.8 firmware prefix with 256
copies of the VDX7-owned seed. No Yamaha factory patches are added and the strict
profile prerequisite is unchanged. This fixture is not a distributed ROM package.

PASS: the registered CTest run completed 4/4 tests: the v1.8 profile prerequisite,
Init processor regression, real dialog regression and all 12 audio-matrix cases,
with ASan/UBSan. The matrix also passes independently with the original 16 KiB ROM.

This is automated local processor validation, not real Windows/macOS REAPER
acceptance, listening quality, latency/performance benchmarking or a full ROM suite.

## Non Init source regression

The #153 review identified that the initial matrix started with Init synthesis
bytes and changed only the voice name. That version exercised note lifecycle,
but could not detect an Init operation that replaced only name/provenance.

The corrected matrix starts each case with the audible OP1 carrier set to coarse
2, fine 37 and output 80 instead of Init's 1, 0 and 99. Before confirming Init,
the sounding voice must contain these values and differ from the seed in the
first 118 bytes, excluding its name. After confirmation, all 128 bytes must
equal the generated Init seed, including the restored synthesis parameters.

PASS: all 12 corrected cases with the original private 16 KiB v1.8 firmware,
then 4/4 registered profile/Init tests with the synthetic combined fixture,
15/15 ROM-free CTests and 78 Python fixtures. ASan/UBSan remained enabled for
the C++ tests, with leak detection disabled. Actual local-ROM/private-bank
registration inventories and checker negative controls also pass.

An isolated local mutation retained the source's 118 synthesis bytes while
replacing its name and setting Init provenance. The corrected matrix rejected
this mutant in its first case at the exact Init bytes/title assertion (exit 1).
The mutation was removed, the production source is unchanged, and the rebuilt
unmodified implementation passed the registered tests above. This negative
control demonstrates detection of the review's specific missed-reset failure.

## Remaining acceptance gates

The #152 and #153 merge gates are complete. The #153 head `4bc6b7a` passed
Windows/macOS/ASan-UBSan runs 37646131366, 37646131345 and 37646131405, with
the corrected review thread resolved and fresh code/security reviews reporting
no further findings. It merged as `1675c83b12a4728947168c0898b03a6a71dbf145`.
Its merge-main Windows/macOS runs 37651481436 and 37651481370 also PASS.
Real Windows/macOS REAPER acceptance and release packaging/publication remain
separate. D2 is not declared fully accepted or released by these component results.

Local opt-in runners (not registered in public ROM-free CI):
`vdx7_processor_tests --init-preset <private ROM>` and
`vdx7_processor_tests --init-preset-gui <private ROM>`.
The additional runner is `vdx7_processor_tests --init-preset-audio-matrix <private ROM>`.
They become CTests only with explicit existing local ROM configuration.
