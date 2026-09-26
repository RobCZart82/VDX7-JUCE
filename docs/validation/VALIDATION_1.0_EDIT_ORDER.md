# Ordered voice edits — development checkpoint, 2026-09-20

Scope: P0-1 program/bank selection versus pending voice/operator edits.
No GUI redesign, release/tag, firmware upload or installed-plugin replacement.

## Reproduction

Before changing processor behavior, the integration test failed with
`program switch/edit ordering preserves correct voice`. The test chooses an
edit value different from the current value, avoiding a no-op false positive.

## Ownership and ordering

Loaded-engine program, bank, operator and global voice edits share one bounded
4096-entry MPSC sequence queue. Host/UI producers reserve and publish commands;
the sole consumer runs under the existing engine mutex. Both audio processing
and non-audio save/editor synchronization consume that same order. An unfinished
producer reservation prevents later commands from overtaking it. Parameter IDs,
indices and the state format remain unchanged.

`setCurrentProgram` and parameter listeners do not acquire the engine mutex or
allocate queue storage. Consumption is limited to 4096 commands per call, not
an unlimited loop while other threads continue producing. This is not a proof
of callback deadline compliance under pathological automation; stress/CPU and
host-notification work remain separate gates.

Factory-bank loading still replaces the single internal RAM bank. An edit made
before that replacement belongs to the old bank and may be discarded by the
explicit bank load, as before; it must not leak into the new factory bank. This
does not add an eight-bank unsaved-edit cache. Normal program changes within
the loaded bank retain edits in their original voice slots.

ROM-unavailable edits retain the previous separate deferred-restore mechanism.
Explicit successful ROM/state/SysEx replacements discard pending commands.

## Saturation policy

Do not silently overwrite/coalesce a command. A full queue atomically freezes
its producer reservation counter. Previously accepted commands can drain, but
new commands are rejected until an explicit state/ROM/import reset. A visible
status warning tells the user to save/export accepted changes and reload the
project. In particular, a rejected selection cannot be followed by an accepted
edit to the old selection. Overload is reported, not claimed lossless. GUI/host
parameter controls may temporarily show rejected requested values; packed RAM
remains authoritative on restore.

## Validation coverage

Local macOS arm64 Release: complete CTest suite PASS, 7/7, no skips (32.93 s).
The ROM-free queue test also passed 100 consecutive runs. VST3 build and direct
ad-hoc codesign/strict verification passed; no installed plugin was replaced.
This is not a Windows or REAPER acceptance result.

- 16 local ROM cases: program/bank × edit-before/after × save/audio flush ×
  global voice/operator edit. Compare all 4096 bank bytes, including untouched voices.
- 32 successive program/edit pairs before a single save; no mailbox coalescing.
- ROM-free queue tests: FIFO across wraparound, capacity/fail-closed/recovery,
  four concurrent producers and one consumer (2048 commands, no duplicates).
- Existing missing-ROM, SysEx, 148-parameter, MIDI range and timing regressions.

## Still open

P0-2 host notifications can still originate from the audio callback after a
selection change. This patch deliberately does not claim to resolve that audit.
Pure parameter edits do not add new snapshot/host-publication bursts. Deferred
MIDI timing, lifecycle cleanup, realtime stress, SRC and actual host acceptance
also remain open. GitHub CI for this source must pass before merge; this is not
an RC acceptance declaration.
