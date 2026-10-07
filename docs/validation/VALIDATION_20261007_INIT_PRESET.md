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

## Remaining gates

The initial PR Windows/macOS runs failed their registration smoke checks:
the strict CTest inventory did not yet include the two new opt-in Init tests.
The inventory now declares both tests without weakening the ROM-free/private
fixture policy. Actual local ROM-enabled/private-bank and ROM-free CTest JSON
inventories pass the checker, including its negative controls. These are
registration checks, not private factory-bank integration execution.
The review's mixed-capture race is corrected by the locked working-voice
snapshot; the local processor regression covers identical-byte selection
changes and returning to the original program before confirming.

Fresh branch Windows/macOS builds, hosted sanitizer results and review remain
the PR merge gates. The baseline merge-main runs are green but are not PASS for
this change. New VST3/AU/Standalone format builds are delegated to those fresh
platform workflows; the local build above is shared code and test runners.
Real Windows/macOS REAPER acceptance, extended audio/sample-rate/held-note
matrix and release packaging/publication are not covered by this round.
D2 remains in progress until the relevant acceptance gates are resolved.

Local opt-in runners (not registered in public ROM-free CI):
`vdx7_processor_tests --init-preset <private ROM>` and
`vdx7_processor_tests --init-preset-gui <private ROM>`.
They become CTests only with explicit existing local ROM configuration.
