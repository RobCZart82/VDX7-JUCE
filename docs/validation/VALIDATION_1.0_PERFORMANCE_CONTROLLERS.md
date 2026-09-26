# Milestone 4A — firmware-backed controller panel

Based on merged PR #11 (`b5eb34f27a385ebe80bc6d5de5c1423a7c8c6944`). Its
macOS/Windows PR checks and subsequent main builds were successful. This is a
development slice, not completed milestone 4 or a release.

## Implemented scope

- PERFORMANCE/EDIT switching. Four controller panels expose ranges 0–99 and
  independent pitch, amplitude and EG-bias assignment switches: mod wheel,
  foot (CC4), breath (CC2), and channel aftertouch.
- Persistent LCD navigation, algorithm dropdown, header Save As, output and
  keyboard. Operator editing controls are hidden in PERFORMANCE and restored
  when returning to EDIT. The existing graphical style is retained for now.
- These are global firmware battery-RAM settings. UI access is non-realtime,
  serialized with the engine mutex; host state-change notification happens only
  after unlocking. No new automation parameters: the original 148 IDs/indices
  remain unchanged. They do not set the voice-bank export-dirty flag.
- Existing full-RAM project storage includes these settings without a schema
  change, including missing-ROM resaves and later ROM restoration. Factory bank
  changes and voice/bank SysEx imports do not carry controller/global settings.
- The GUI roadmap also records the newly approved warm charcoal, yellow-green
  LCD, restrained button palette and original VDX7 visual identity. Reference
  photos/branding are not copied into the repo or binary.

## Firmware evidence and regression findings

The pinned core names foot/breath RAM fields in the opposite order to the
firmware. Its two defaults happen to be equal, so this was not exposed by boot.
Initial audio tests caught zero response when editing the wrong controller's
range. The wrapper now uses wheel/foot/breath/aftertouch order, two-byte stride:
assignments at 0x232E, ranges at 0x2336. No dependency source was modified.

The [annotated v1.8 firmware source](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm)
documents these addresses and the analog-input refresh path. Runtime tests using
the user's local ROM verify the mapping and audio response independently.

After an edit, two bounded, allocation-free analog handshakes refresh the
firmware's derived modulation, only after pending controller messages drain.
The firmware's current wheel input is converted back to seven-bit form before
re-submission, avoiding stale input and repeated doubling. A new range therefore
takes effect on held input without another external MIDI event. The ROM is not
patched and the CPU is not redirected into a guessed routine. Compatibility
with arbitrary third-party/modified firmware is not claimed.

## Verification scope

Final local macOS arm64 Release CTest run: **9/9 passed, 49.87 seconds**.

Expanded processor tests exercise all assignment masks for each controller,
range endpoints, invalid indices/values, exact RAM isolation, instance isolation,
bank import/selection preservation, immediate project save, missing-ROM round
trip, and settings retained while rendering. GUI tests exercise all four knobs,
12 switches, reverse refresh, EDIT/PERFORMANCE visibility and persistent LCD.
Component bounds are checked at 960, 1440 and 1600 pixel widths. PERFORMANCE
snapshots at 960 and 1440 pixels were visually inspected.

Twelve audio A/B pairs cover four controllers times three assignment routes.
Both versions receive the same controller input; one raises range from 0 to 99
while a note is held, with no later external MIDI input. Both execute equal
refresh handshakes. Tests check finite audio, nonzero audio differences, scaled
firmware modulation and preservation of the raw controller value. EG bias uses
midrange input, since maximum input can correctly produce no attenuation.

Local VST3 build and direct ad-hoc signature/strict verification succeeded.
The build helper's pre-existing Xcode licence warning was not resolved by
changing system settings. No installed plugin, ROM payload, tag or release was
changed or distributed.

## Still open

- Fresh CI and user review/merge for this chapter.
- Physical controller/DAW acceptance, automation policy for new globals if later
  desired, real-host project-dirty indication and longer interactive sessions.
- Play mode, pitch-bend settings, portamento and SETTINGS channel/tuning.
- USER library and slot-based Save As; current Save As remains SysEx file export.
- Final hardware-inspired skin and original wordmark. No claim of finished GUI.
