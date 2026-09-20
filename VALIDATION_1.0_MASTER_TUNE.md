# SETTINGS master tuning

The previously disabled SETTINGS button opens a master-tuning Apply/Cancel
dialog. Input is an integer -256..255, labelled firmware units, not cents or Hz.
Zero restores the core's default tuning. Invalid input or missing ROM does not
apply a value. If tuning changed while the dialog was open, reopen it instead
of silently overwriting that newer value. A SafePointer protects editor closure.

The engine uses the pinned core's existing `DX7::tune(int)` API. Its encoding is
`value + 256` at battery RAM 0x2311/0x2312. The getter decodes those bytes.
Processor access uses the existing engine mutex, with host notification after
unlock. No extra DSP pitch offset, firmware patch, host automation parameter or
new project-state field is introduced. Voice-bank bytes/dirty markers are unchanged.
Existing RAM project persistence carries tuning; voice/bank SysEx does not.

MIDI input remains the existing all-channel/OMNI behavior. Channel selection is
explicitly marked unavailable in the dialog and remains the next chapter. This
is a functional development dialog, not the final hardware-inspired skin.

## Validation

- Local macOS arm64 full CTest: 10/10 passed in 58.99 seconds.
- All 512 tuning values accepted/read back; out-of-range and missing-ROM rejected.
- Project recall restores +255; voice bytes and dirty markers remain unchanged.
- Isolated MIDI note 60 measured at low/zero/high tuning: 250.386 / 261.403 /
  272.921 Hz. Test requires increasing pitch and correct zero-tuning pitch.
  This is not a cents calibration or physical-hardware equivalence claim.
- GUI test added after the full run opens SETTINGS, checks current tuning text
  and cancels. Its separate rerun passed and screenshot was visually inspected;
  this supplements the full-suite result.
- Local VST3 built and strict ad-hoc signature verification passed. Existing
  Xcode licence warning from the helper remains; system settings were not changed.

Still required: interactive Apply/error/conflict paths in real hosts, Windows
GUI, missing-ROM deferred-recall tuning-specific test, calibrated tuning units,
MIDI input-channel filtering and final visual styling. No release/tag or installed
plugin replacement. No ROM or proprietary factory payload is committed.
