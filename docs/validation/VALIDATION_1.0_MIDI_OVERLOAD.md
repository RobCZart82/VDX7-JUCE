# Milestone 3C — MIDI overload and callback allocation probe

2026-09-20. Baseline main `f265c7c5d39cf48aee6c4f5b6109aff652d032d1`
(merged PR #9); baseline macOS and Windows checks passed before development.

## Reproduction and cause

The pinned core's 8192-byte MIDI RX ring has no full check. Its read/write
indices becoming equal means empty, so unchecked writes can overwrite unread
bytes or make a full buffer appear empty. Separately, the 1024-entry controller
FIFO rejects full writes but the core's public `push` discards that return value.
Consequently a note-off/pedal release could disappear while wrapper ownership
said it had been sent. A new regression failed before the fix:
`serial overflow must reconcile note ownership`.

## Bounded recovery policy

The wrapper checks space for the complete message before any serial bytes are
written, leaves one slot unused, and also checks controller saturation. All
access is engine-owned/serialized; no core/dependency source is modified.
On exhaustion:

- Discard queued serial/controller traffic and increment a diagnostic counter.
- Preserve any in-flight sub-CPU handshake so its second byte still arrives.
- Release sustain and portamento pedals and enqueue complete note-offs for all
  128 pitches, including voices no longer represented in wrapper ownership.
- Ignore subsequent same-timestamp messages until rendering advances. Restore
  the latest requested program after the recovery release batch, then accept
  new input. A sustained overload can cause another recovery later.
- Reject live bank SysEx during this short recovery state rather than flushing
  the recovery note-offs. The sender must retry after reducing traffic.
- When live bank loading resumes, preserve the serial queue (including any
  recovery releases still draining); append its program change in order.
- Show a warning in the existing status text. It remains until a successful ROM
  reload resets the runtime counter; no project/voice state is overwritten by
  that diagnostic. The existing edit-queue warning has higher display priority.

This deliberately drops overloaded input. It is not lossless, does not remove
the firmware's serial throughput limit, and note releases still pass through
firmware/envelopes rather than instantly hard-muting output. Recovery loops are
bounded (at most 1024 discarded controller entries and 128 release messages).
Lifecycle restart also sends all 128 releases, including after overload, and
lets an already-started controller handshake finish before final reset.

## Tests and audit scope

- Inspect the exact 128 complete recovery messages, same-timestamp suppression,
  controller saturation, pedal state, restart during recovery and fresh notes.
- Reach a real half-delivered sub-CPU message and verify recovery preserves it.
- Render 60 seconds of simulated 48 kHz audio with repeated chords, modulation
  and automation: no overload during this ordinary sustained scenario.
- Inject 5000 same-position note events and then 5000 controller events. Both
  overload paths reconcile ownership and produce quiet firmware output after
  the drain; a new note is audible afterwards and the status warning is present.
- The existing 12-rate/buffer cases and independent second instance remain.
- A self-tested, thread-local test-executable probe intercepts ordinary C++
  `new`/`new[]` and `delete` variants around callbacks. Zero such allocations and
  deallocations are required for the long-run, overload and grid scenarios.

The probe deliberately does NOT claim interception of direct C `malloc/free`,
aligned allocations, OS/framework internals, or activity on other threads.
Source inspection of the callback path found fixed buffers/table reads and
bounded queue operations in rendering, voice edits and validated bank SysEx;
engine locking and JUCE load metering use try-locks. File I/O, state serialization,
table construction and UI publication are outside that callback path. This is
not a race-detector result or an absolute hard-realtime proof for arbitrary ROMs.

CI now compiles the stress executable on macOS and Windows. Execution of ROM
scenarios remains opt-in and local; CI runs ROM-free tests only. No firmware or
ROM path is committed. No release, tag or installed-plugin replacement occurs.

## Next gates

Local macOS arm64 Release: full CTest 9/9 passed in 49.09 seconds, including
five opt-in local-ROM tests. The rebuilt development VST3 passed strict ad-hoc
signature verification. These results do not substitute for fresh remote CI.

After fresh PR/CI approval, proceed to firmware-backed milestone 4 features.
Keep host/device contention, physical MIDI, DAW latency compensation, broader
platform heap profiling and longer realtime soak testing on the 1.0 acceptance
checklist. A one-minute offline simulation is not a live DAW soak test.
