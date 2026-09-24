# Portamento: retain accepted intent during native serial work and save

2026-09-23, Draft #47. Baseline local `9c6c3cefefaab07d053016c1ea8f5e248a55fd1d`,
remote `adedaac81d4a9e875ecb9b1f4b458231fa2d5685`, identical tree
`cf7735df83413bf4af1e4297e1f6fd87bd04d78b`. Both baseline platform Actions passed.
No MONO compatibility workaround, firmware/CPU patch or installed plugin change.

## Reproduced before the production change

Two focused tests against the real processor/private local v1.8 ROM failed:

- Queued time 37, then 0, then latest UI request 99: callback 1 displayed 37,
  matching transient native RAM, instead of retaining the accepted 99.
  `--portamento-display-only` exited 1.
- During actual serial/controller overflow recovery, accept UI time 73 and
  immediately call `getStateInformation` without a callback/drain/sleep.
  Saved RAM offset `0x157d` contained 0. `--portamento-save-only` exited 1.

These are independent of Q2's stale captured-frame race. Previously, successful
time submission directly overwrote firmware time RAM while older CC5 commands
could still overwrite it again. During recovery submission returned false;
processor dirty-bit retry retained the request but save copied only old RAM.

## Implementation and ordering contract

- Engine-owned `portamentoTimeSetting_` retains the latest accepted time. It is
  requested parameter state, **not** an acknowledgement or derived rate. Until
  the first request/restore, boot RAM remains the source.
- A valid time request is accepted even while the serial FIFO is unavailable.
  Existing bounded refresh/recovery submits its native CC5 when possible. The
  processor no longer needs a dirty-bit retry for an already accepted request.
- Display/getter and the detached saved-RAM copy use the requested setting.
  No live time/rate RAM write is performed by the time setter; native CC5 still
  computes both. No serial wait, extra render budget or timing guess is added.
- An accepted physical CC5 replaces intent in engine admission order; rejected
  input cannot replace it. UI requests still use the existing callback-boundary
  coalescing policy, not a new cross-thread wall-clock ordering guarantee.
- Overflow/host-reset flush marks intent for replay. Before admitting fresh
  serial MIDI, reserve room for retained CC5 plus that message and queue time
  first. A newer accepted CC5 replaces the retained request instead.
- Reset does not inject refresh work inside its release drain. Program admission
  retains its previous reset ordering. Without an explicit time request, the
  original boot-RAM/background refresh ordering is retained; do not turn a
  background refresh into a new priority command ahead of ordinary input.
- Restore installs the saved requested value and replays it through native CC5.
  Existing full-RAM project format/parameter IDs are unchanged. Only the detached
  time byte is projected for save; the derived rate is outside saved battery RAM.

There is intentionally no value-match/timeout claim of firmware acknowledgement.
The display describes the accepted setting while serial application is pending.
Supported time inputs are the public setting API, admitted CC5 and project RAM
restore. A future full native-front-panel or performance-SysEx input path must
join this intent model; it must not silently introduce another writer.

## Regression coverage

New registered local `vdx7_portamento` uses the existing stress executable with
`--portamento-only`, requires the v1.8 fixture, and validates the same image in
direct CLI invocations. Public ROM-free CI compiles it but does not run a ROM.

- Six configurations: 44.1/48/96 kHz x 64/256 samples.
- All 100 time settings in each configuration (600 cases), with older queued
  replacements and explicit display checks after every callback. Real firmware
  time and derived rate match a reference given direct native serial CC5, not
  another requested-value getter. Save is checked before serial settling.
- UI -> physical CC5 and CC5 -> UI precedence (0/1/64/127, channel 16), invalid
  values rejected, and later rejected CC5 during recovery does not overwrite UI.
- Five recovery/save scenarios per configuration: existing recovery, command
  accepted then flushed, FIFO full at submission, explicit host reset, and a
  new request/save while the reset drain is already active (30 cases total).
  Immediate state capture, restore/resave, actual native time/rate verification.
- Missing-ROM save and delayed ROM load retain the saved time.
- Retained CC5 bytes precede fresh Note On after recovery; actual native pitch-72
  ownership and audible output are required, followed by release to silence.
- Callback ordinary-C++ allocation/deallocation probe remains enabled throughout.
- Existing live-bank recovery regression now also verifies every old queued byte
  and the exact appended program bytes. Its original +2-only expectation remains
  unchanged for this fixture without an explicit time request.

The first implementation's full run was **21/24**, not success: premature and
unnecessarily prioritized reset refresh failed the existing live-bank ordering
and two history/onset tests (266.00 s). Merely suppressing duplicate refresh was
insufficient: overlap onset still failed. The final implementation preserves
background ordering when no time request exists and defers reset refresh until
after program admission. The release-prefix check was strengthened; the original
two-block history/onset limits were **not** increased and no assertions removed.

## Validation results

Final rebuilt full local macOS arm64 suite with desktop access:
**24/24 PASS (290.83 s)**. The dedicated portamento group passed in 25.49 s.
Two labelled MONO characterization groups assert the known defect, not repair.
Focused six-configuration regression and the unchanged history/onset tests PASS;
ROM-ON all tests/VST3 and ROM-OFF CI targets rebuilt successfully.
ROM-free tests **6/6 PASS (1.43 s)**, ROM tests OFF and ROM path empty.
Local development VST3 manually ad-hoc signed and strictly verified after the
existing Xcode-license signing-helper warning; installed bundle untouched.
Original native MONO-zero diagnostic separately still FAILS, exit 1,
MIDI/held/MONO counts 0/1/16. It is not counted as passing acceptance.

## Limits / next work

No physical DX7, Windows ROM-runtime or real DAW acceptance in this round; no
full-plugin sanitizer/WCET claim. Serial latency remains and display is not proof
of completed firmware execution. State-install/epoch races, pitch/mod delivery,
bypass and remaining P1/reset gates stay open. Targeted MONO correction is now
approved for development separately, with original/default behavior preserved;
no corrected path is implemented. Keep Draft #47; no main merge/tag/release.
