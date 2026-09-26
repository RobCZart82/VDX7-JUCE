# USER-bank storage foundation

Development slice after merged PR #12. PR and main macOS/Windows checks passed
before this chapter. No release, firmware payload or installed-plugin change.

## Approved workflow

ONE SAVE AS... header button will offer:

1. **Patch -> USER bank** (default): patch name, USER bank and slot. Existing
   target name shown before explicit overwrite confirmation. Other slots remain.
2. **Export Patch...**: one voice SysEx file.
3. **Export Bank...**: the complete 32-voice bank SysEx file.

UTILITY remains editing/organisation. Its duplicate exports will be removed
when the full shared save dialog replaces them, not prematurely. The USER bank
is a persistent, gradually populated collection, independent of a DAW session.
Factory voices are only sources of user copies; factory ROM is never written.
Global PERFORMANCE settings remain project state, not voice/bank SysEx.

## Implemented in this chapter

`VDX7UserBank` is a non-realtime storage component with independent ROM-free tests.
It is **not yet linked into the plugin or exposed by the GUI**. Existing plugin
save/export behaviour is unchanged; do not advertise a usable USER library yet.

Its `Snapshot` holds 32 packed 128-byte voices and a 32-bit occupancy mask. Empty
slots contain synthesized silent placeholders, not factory data. `savePatch`
accepts one source voice by value, a 1–10-character printable ASCII name and a
zero-based target slot. It preserves the other 31 slots and renames only the copy.

On save, a process mutex and a named interprocess lock protect cooperating
writers. A busy save returns an error instead of blocking the audio thread or
silently retrying. The file is re-read under these locks: stale views can merge
different-slot edits, but a changed destination requires a new user decision.
An occupied destination additionally requires explicit overwrite confirmation.
If a previously displayed bank was deleted, the stale save does not recreate it.

The complete replacement is written to a sibling temporary file, flushed,
reloaded/verified and committed using JUCE's target-file replacement. No early
truncation of the destination. Unsupported, malformed or checksum-damaged banks
are rejected and left intact, not treated as a fresh empty library. Unavailable
folders are errors; the storage helper does not create directories implicitly.
Direct symbolic-link targets are rejected. This is cooperative local-file
concurrency protection, not a guarantee against arbitrary external writers,
network filesystem semantics, disk failure or sudden power loss.

## File format v1 (development)

4108 bytes, deliberately distinct from standard SysEx:

- bytes 0–3: ASCII `VUB1` (format/version marker)
- bytes 4–7: little-endian 32-bit occupancy mask
- bytes 8–4103: 32 packed 128-byte voices, all bytes seven-bit
- bytes 4104–4107: little-endian CRC-32/IEEE of bytes 0–4103

CRC detects
accidental corruption, not malicious modification. No paths, globals, ROM or
firmware are stored. Standard single/bank SysEx remains the separate exchange
format and does not preserve library occupancy metadata.

## Tests

The new `vdx7_user_bank` CTest is built and RUN on both CI platforms, with no ROM.
It covers first save/reopen, silent empty slots, slot 32's high occupancy bit,
different-slot stale merges, explicit overwrite, stale target conflicts, invalid
names/slots/voice bytes, unavailable destination, truncated/new-version/checksum
damage, failed-load output preservation, and refusal to reset damaged/deleted banks.

Two threads exercise concurrent same-process saves with busy retry; a child
process holds the interprocess lock to verify busy rejection and subsequent
retry. Bank and individual saved voices round-trip through the existing SysEx
encoder/decoder. Only generated temporary test data is written and cleaned up.

## Next integration slice

Local validation: all 10 CTest tests passed on macOS arm64 in 49.08 seconds,
including the ROM-free USER-bank test. Existing firmware-dependent regression
tests used the locally supplied ROM only; no firmware is included in this change.
The storage backend is not linked into the plugin yet, so this is not validation
of the future SAVE AS dialog or LCD integration.

- Explicit library folder policy and a default USER bank.
- Capture the source voice consistently before opening the save dialog; do not
  silently substitute a later MIDI-selected patch while the dialog is open.
- Name/slot UI, destination preview, cancel, overwrite confirmation, busy/error
  reporting and reload/reconfirmation on conflict. Cancellation must write nothing.
- LCD USER selection and loading, while preserving unsaved working-bank edits.
- Only acknowledge the source patch's dirty flag if its content still matches
  the saved snapshot. Saving a copy must not erase edits made during the dialog.
- Show global PERFORMANCE exclusion clearly; no duplicate header save buttons.
- Processor/GUI/host tests, multi-instance library discovery and session restart.
