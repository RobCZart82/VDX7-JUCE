# Milestone 2 — MIDI timeline and lifecycle, 2026-09-20

Development checkpoint, not release/host acceptance. Baseline PR #6 is merged;
baseline macOS/Windows checks on `df07026c7f647c8abd85695bff0d031aca1d22c4`
passed before this chapter began. No running Actions job was cancelled.

## Reproduced failures

- `deferred note duration must not collapse`
- `device restart clears old note ownership`
- `program metadata and firmware input agree`

## MIDI policy

Contention pauses playback of an explicit MIDI sample timeline, not arrival of
new input. Each deferred event retains its absolute input position. Subsequent
host MIDI joins that timeline until recovery finishes. Playback consumes only
events due within the current block, rendering between their positions.
Normal, uncontended input bypasses deferred storage entirely (no new 256-event
limit on ordinary host MIDI blocks).

This preserves spacing/order between deferred and subsequent MIDI events,
including future note-offs that have not arrived when a delayed on is replayed.
It cannot undo already-silent blocks: audible latency increases, and notes that
were already sounding before contention can be lengthened. Delay is reset only
when the queue is empty AND no MIDI note/sustain is held. Never clear the time
offset immediately after delivering a lone note-on.

Storage remains bounded to 256 events/65536 bytes. A backlog window over two
seconds (or one unusually large host block, whichever is larger), event/byte
overflow or non-monotonic input discards the batch and requests all-notes-off /
sustain reconciliation. This exceptional recovery intentionally drops stale
events; it is not claimed lossless. Compaction is bounded and allocation-free.
The original firmware's own serial latency and envelope behavior are unchanged;
sample-position preservation is not a guarantee of audibility for every patch.

## Device lifecycle

`releaseResources` and `prepareToPlay` clear deferred/keyboard events under the
host's stopped-callback lifecycle contract. The engine flushes old serial and
controller queues, releases MIDI notes/sustain/portamento pedal and advances the
unmodified firmware for 250 ms of emulated time to consume releases. It then
reconstructs the EGS in place to clear envelopes/phases/filter tails, resets the
resampler and reselects the current program. No firmware/core source is modified.
This work is non-realtime only, never performed in `processBlock`.

Packed voices, bank/program, firmware and controller configuration RAM remain.
CC7/CC11 levels remain; host master/pitch/mod parameters remain, with pitch/mod
reapplied after prepare. Held notes/pedals, pending serial events and old audio
tails deliberately do not survive restart. Actual REAPER transport/seek/loop
acceptance is still a separate gate; a host stop is not always a device restart.

## Program Change

VDX7 exposes 32 programs. The existing clamp-to-31 policy now applies to BOTH
wrapper metadata and the byte sent to firmware for incoming values 32–127.
Tests inspect the actual serial bytes for PC 0, 31, 32 and 127. This avoids
depending on undocumented firmware behavior for out-of-range program numbers.

## Coverage

- ROM-free: exact sample positions across multiple blocks; program/sustain/note
  ordering; 32-sample duration; future note-off; lag cap, existing overflow tests.
- Local ROM: original 192-sample on/off pair remains active during the first
  64-sample recovery block; subsequent off delivered; note-off/overflow regressions.
- Restart while one note sounds and another is deferred: no stale ownership,
  no deferred replay, silent old voices, unchanged packed bank/program.
- Existing 148-parameter, state, all-note-range, timing and processor tests.

New-source GitHub CI must pass before the next development chapter. Remaining
milestones 3–4 are NOT claimed complete by this checkpoint.

## Local result

2026-09-20, macOS arm64 Release: affected test targets and VST3 built; full
CTest passed 7/7 in 32.19 seconds, including four opt-in local-ROM tests.
The development VST3 was ad-hoc signed and `codesign --verify --deep --strict`
passed. No installed plugin was replaced. Firmware is not included in source,
CI or the development bundle. Universal/Windows CI remains the PR gate;
local arm64 tests are not a substitute for that gate or host acceptance.
