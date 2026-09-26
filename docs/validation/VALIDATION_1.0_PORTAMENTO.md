# PERFORMANCE play mode / portamento development slice

## Controls

- POLY / MONO. Switching mode ends sounding notes in this instance, using the
  firmware's native CC126/127 mode-change reset, not a direct mode-byte overwrite.
- Portamento mode follows play mode: Retain/Follow in POLY and Fingered/Full time
  in MONO. It is the same firmware setting with different interpretations.
- Glissando OFF/ON and portamento time 0-99 (0 is immediate).
- CC65 remains the portamento pedal input. No new host automation IDs are added;
  these globals are stored in existing project RAM, not patch/bank SysEx.
- The actual controls replace the PERFORMANCE placeholder text; the four existing
  controller panels remain below them. This is not the final graphical skin.

## Implementation and limits

The [annotated firmware](https://github.com/ajxs/yamaha_dx7_rom_disassembly/blob/master/yamaha_dx7_rom_v1.8.asm)
defines play mode at 0x20A9, portamento mode at 0x20AA, glissando at 0x20AB and
time at 0x257D. Native CC5 refreshes the derived rate at 0xE0. The UI maps every
0-99 time to an exact CC5 value; it does not copy a firmware lookup table.
Project RAM does not contain 0xE0, so recall schedules a native rate refresh.
Where capacity allows this is inserted before program selection and future host
notes; full/recovering queues retain a deferred fallback. Incoming CC5 cancels
that deferred refresh; later input wins.

Mode changes are UI-only engine transactions under the processor mutex. Earlier
serial work is drained with a bounded render budget before submitting a mode
command, checking both the adapter FIFO and the firmware's internal receive ring.
If that backlog cannot be drained, the request fails without appending
a delayed mode command. Native mode reset is then completed with a second bounded
budget. Generated transition audio is discarded and the resampler reset, so this
is an intentional note-ending operation, not a seamless real-time mode morph.
Real-host contention/glitch acceptance remains necessary; no hard-realtime claim.

No CPU jumps, firmware patches, copied ROM payloads, plugin installation or public
release are part of this work. Arbitrary modified-firmware compatibility is not
claimed. SETTINGS MIDI input channel and master tuning remain a separate chapter.

## Tests

Final local macOS arm64 CTest: **10/10 passed in 57.93 seconds**, using the
user-supplied local ROM and desktop access for GUI checks.

- Processor bounds checks, immediate setting capture, project recall and no
  voice-bank dirty flag. GUI binding and bounds at three editor sizes.
- All 100 portamento time values round-trip through native processing; derived
  rate decreases from 255 to 1 and is recalculated after RAM restore.
- Sustained-note frequency measurement distinguishes slow mono portamento from
  an immediate octave transition; all-notes-off tail and return to POLY checked.
- Existing pitch-bend, full MIDI range, state, controller and overload regressions.
- A burst of 60 program reloads followed by a mode request verifies that a busy
  rejection does not become a delayed change; retry after drain succeeds.
- The earlier GUI stress test had hundreds of queued program reloads without
  host processing. It now renders between chapters before testing interactive
  controls, while the separate backlog test keeps that overload case explicit.

The 960-pixel PERFORMANCE rendering was visually inspected. Local VST3 build
and strict ad-hoc signature verification succeeded. The existing Xcode licence
warning in the build helper was not resolved by changing system settings.

Still required: physical pedal tests, comprehensive Retain/Follow and
Fingered/Full-time/glissando musical acceptance, Windows GUI and real DAW mode
switching under load. Local automated checks do not replace these release gates.
