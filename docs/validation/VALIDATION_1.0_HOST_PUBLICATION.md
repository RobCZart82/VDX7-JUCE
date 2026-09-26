# Non-realtime host publication — development checkpoint, 2026-09-20

Scope: P0-2 direct parameter notification from `processBlock`, including program,
bank and live SysEx changes. This does not certify the entire audio path as
allocation-free/lock-free, or close the remaining realtime stress gates.

## Before / after

The new local integration regression failed on the old implementation:
`audio callback must not publish host parameters`.

`updateEngineSnapshot` now updates atomic metadata and sets an atomic publication
flag only. It never calls the host or posts a message. A processor-owned 30 Hz
JUCE timer polls from the message thread, without requiring an editor. Explicit
non-realtime state/save/load/editor operations can synchronize immediately.

Synchronization drains ordered commands and collects fixed-size voice values
while holding the engine mutex, then RELEASES that mutex before notifying the
host. An atomic in-progress guard prevents recursive/overlapping publication
without making callers wait. New edits or engine changes interrupt stale
publication and remain pending for a later poll. Self-notification suppression
is scoped to the parameter being published, so a host callback editing another
parameter is not mistaken for the plugin's own echo.

Parameter views converge on the next available message-thread poll (nominally
33 ms, not a hard latency guarantee). In a headless host without a message loop,
`getStateInformation` explicitly synchronizes. Its detached parameter-state copy
is filled from the SAME engine snapshot as the packed RAM, even if the save is
reentrant during a partially completed host notification. No editor, timer tick
or complete notification burst is required for a coherent saved sound.

## Notification call-site audit

Both explicit `setValueNotifyingHost` sites in `Source/PluginProcessor.cpp` are
inside `synchroniseOperatorParametersFromEngine`: the operator loop and the
global-voice loop. They run only AFTER the engine-lock scope ends. Entry points:

- Processor timer: message thread; created with processor, stopped at destruction.
- Editor initialization: message thread.
- ROM/SysEx file loading, rename and paste: non-realtime editor/file operations.
- State save/restore: host non-realtime serialization context; reentry is guarded.

No `processBlock` path calls this method. The processor's `updateHostDisplay`
sites remain in non-realtime file/utility operations, outside engine locks.
JUCE editor control attachments can notify from user interactions on the message
thread; host-originated automation is an input, not processor snapshot output.
Callers must not invoke non-realtime file/state APIs from an audio callback.

## Test coverage

Final local macOS arm64 Release validation: **7/7 CTest PASS**, none skipped,
32.72 seconds. VST3 build, direct ad-hoc signing and strict verification PASS.
The installed plugin was not changed.

- Observe all 148 parameters during audio program, CC32 bank and live bank SysEx
  changes: zero value notifications directly from `processBlock`.
- Drive the processor's publication poll without constructing an editor.
- Host listener synchronously calls `getStateInformation`: completes without
  deadlock; saved voice parameter values agree with the saved packed RAM.
- Reentrant host edit of a different parameter survives publication and re-poll.
- Existing ordered edits, missing-ROM restore, parameter identities and legacy
  project state are covered by the full local integration suite.

## External checks and limits

Before this patch was uploaded, the pre-existing main runs for
`b3ad24669e4dff4d2e900ead350f51cef09b3f20` completed successfully on both macOS
and Windows. They were not cancelled or replaced. These are BASELINE results,
not CI evidence for this new source. New-source CI remains required before merge.

Remaining gates include deferred MIDI timeline, lifecycle cleanup, realtime
allocation/locking and CPU stress, actual REAPER acceptance, SRC measurements
and exact-commit RC packaging. No release/tag, firmware upload, installed-plugin
replacement or GUI redesign is part of this checkpoint.
