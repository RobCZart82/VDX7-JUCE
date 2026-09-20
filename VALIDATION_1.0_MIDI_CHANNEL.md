# SETTINGS host MIDI channel selection

SETTINGS now offers OMNI (all channels) and 1-16 alongside master tuning.
Selection filters external channel messages before both direct delivery and
deferred storage. Rejected channels cannot change notes, controllers, programs
or bank selection. The existing validated global bank-SysEx import remains
unfiltered, as does the UI keyboard. The dialog states these exceptions.

This is a host-input routing filter, not a modification of firmware MIDI RAM
or firmware code. Accepted events still use the established single-part adapter
route to the firmware's receive channel. It does not add multitimbrality.

The project has one optional `midiInputChannel` integer (0=OMNI, 1-16=channel).
Absent/out-of-range values fall back to OMNI; old project behavior is preserved.
Missing-ROM state captures and re-saves retain channel changes. It is not a host
automation parameter or part of voice/bank SysEx. Voice bytes and dirty flags
are not changed by the setter. UI Apply currently requires loaded firmware
because the same dialog also applies firmware master tuning.

The audio thread samples selection once per block. On change it discards the
old deferred timeline and requests release of existing notes/sustain before
delivering fresh events. Release survives engine-lock contention; the UI setter
does not block on the engine. A stopped host applies the release on the next
processed block. This is intentionally not a seamless held-note channel transfer.
UI-held key graphics may remain down until physical release; no UI MIDI is
retriggered by that visual state. Already submitted firmware events cannot be
withdrawn, but the release messages follow them in order.

## Coverage

Local macOS arm64 full CTest: 10/10 passed in 58.01 seconds. The additional
GUI screenshot run passed and SETTINGS was visually inspected. VST3 build and
strict ad-hoc signature verification passed; the existing helper Xcode licence
warning was not addressed by changing system settings. Fresh PR CI is required.

- ROM-free filter matrix covers OMNI/1-16, all source channels and all channel
  message classes in existing CI tests.
- Local processor tests: selected 1/16/7 notes only, wrong-channel program/CC
  rejection, held-note/sustain release, stale deferred-event discard under
  contention, fresh-channel note delivery and UI keyboard bypass.
- Project recall, legacy-field omission, missing-ROM recall and edited resave.
- GUI checks 17 selector entries/current selection; final Apply/conflict paths
  still require real-host interaction acceptance.

Remaining: physical-controller/Windows GUI acceptance, dense live switching and
mixed automation listening tests, final visual skin. No release, tag, firmware
payload or installed-plugin replacement is part of this chapter.
