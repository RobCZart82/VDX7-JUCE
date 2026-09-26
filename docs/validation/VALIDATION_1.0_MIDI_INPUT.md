# Complete MIDI input validation

## Scope and policy

Host events must be complete MIDI channel messages. Running-status fragments,
wrong lengths, statuses below 0x80 and data bytes with their high bit set are
rejected before engine queue reservation or processor keyboard/state mutation.
Program Change and channel pressure require two bytes; other channel messages
require three. Legal Program Change 32-127 still uses the existing clamp-to-31
policy; legal CC32 bank values retain the preceding chapter's mapping.

System common and realtime messages have no supported host action and are now
consistently ignored, including during engine contention. This explicitly drops
F1/F2/F3/F6 rather than forwarding them to firmware. It does not claim MIDI clock
sync, transport following, tune-request or active-sensing support.

Live SysEx is restricted before deferred storage to a complete, checksum-valid
32-voice bank: 4,104 bytes, `F0 43 0n 09 20 00`, 7-bit payload/checksum and
terminal `F7`. The device ID may be 0–15. The engine uses this exact same
bounded, allocation-free validator when importing the live bank. Arbitrary,
truncated, corrupt and single-voice live SysEx is rejected; file import retains
its separately documented single-voice support.

The shared validator is also used before deferred storage, so ignored clock
traffic cannot consume its limited event slots during an engine transaction.
There are no added allocations, locks, host parameters or saved-state fields.

## Tests

Final local macOS arm64 CTest: 10/10 passed in 56.68 seconds, including GUI and
user-ROM integration tests. VST3 build and strict ad-hoc signature verification
passed. The existing Xcode licence warning in the helper remains; no system
licence/settings were changed. Fresh PR macOS/Windows CI is still required.

- ROM-free existing deferred-MIDI CI target exhausts all 256 statuses, lengths
  0-4 and all 256 values at every data position, plus null input.
- A ROM-free deferred-MIDI regression sends 300 empty SysEx messages and 16
  checksum-corrupt 4,104-byte banks before a legal Note On. Both malformed
  bursts are rejected before queue admission; the following note is delivered
  at its original sample position without panic. A structurally valid bank is
  retained by the same test.
- Local engine/processor integration rejects truncated/overlong messages, bad
  data bytes, running-status fragments and system messages without program/bank,
  expression, serial write position, overload or keyboard-state side effects.
- Contention test sends 300 ignored clocks then a legal note. The note survives
  instead of overflowing the deferred event queue; subsequent note-off works.
  Its first test draft incorrectly expected immediate release after a delayed
  256-sample block; it now drains the preserved timeline in 64-sample blocks.
  No production timeline behavior was changed to satisfy that assertion.
- Keyboard overload test starts a UI note, fills the bounded queue, releases
  the note while the queue rejects events, then runs recovery and mirroring.
  Keys are released, engine ownership is clear and no MIDI is requeued by mirroring.
  This disproves that specific reported sequence; it does not prove every GUI
  concurrency scenario safe. No blanket UI-held-state reset was introduced.

Remaining acceptance: live-host bulk-SysEx stress, broader concurrent UI
interaction, physical controllers and actual DAW MIDI/automation ordering. The
processor integration test now requires and uses `VDX7_TEST_ROM_PATH` for every
rendering processor, rather than silently falling back to auto-detected local
firmware. No firmware or factory data is committed. No installed plugin
replacement or release publication.
