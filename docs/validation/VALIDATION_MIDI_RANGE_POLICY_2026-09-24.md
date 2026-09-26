# MIDI input range policy — implementation status (2026-09-24)

## Accepted product behavior

The plugin accepts MIDI Note On/Off events for notes 12–120 inclusive in both
the Native firmware and Correct MONO Note 0 Settings modes. Note 0–11 and
121–127 are filtered without transposition. This reflects the user's REAPER
observation that Note 12 / C0 is the lowest tested note that did not trigger
the native MONO lockup, and the product decision that the excluded extremes
are not needed for this instrument.

The Settings correction remains available as a distinct compatibility option.
The range guard does not patch firmware or RAM and does not prove the raw
firmware Note 0 behavior has been fixed. Raw firmware characterization remains
separate from plugin-facing acceptance.

## Code and automated coverage added

- A shared 12–120 range predicate is applied when host events are admitted,
  before deferred-MIDI storage.
- The processor's central MIDI handler also enforces the range as a backstop.
- ROM-free validator tests exhaustively cover all 128 pitches, Note On/Off,
  velocity-zero Note On, and retained low-numbered CC handling.
- A rejected lower-octave event flood must not consume deferred queue capacity;
  Note 12 must still be admitted.
- Processor integration coverage exercises both Settings modes, rejected
  low/high notes and releases, deferred contention, and accepted notes 12, 60,
  and 120 through release.
- The separate on-screen keyboard event path is also range-filtered, including
  its delayed/deferred route under engine-lock contention.
- Existing plugin processor regressions that previously used Note 0 as a test
  note now use the supported lower boundary (Note 12). Direct raw-firmware and
  engine-range characterization remains unchanged.
- The former `vdx7_mono_note_zero_acceptance` product test entry has been renamed
  `vdx7_supported_note_range_acceptance`; the raw Note 0 failure remains
  documented by firmware-level characterization, not hidden or inverted.

## Verification status

- `git diff --check`: PASS.
- Compilation and automated CTest: NOT RUN in this environment. The local Apple
  compiler is blocked by the outstanding Xcode license prompt, and CMake/Ninja
  are not available in PATH. No build or test PASS is claimed for this change.
- REAPER boundary acceptance of the new filtered build at 11/12 and 120/121 in
  both modes: NOT RUN.
- Windows and Intel Mac host acceptance: NOT RUN.

The next gate is a fresh macOS/Windows CI run on the exact change, followed by
manual REAPER checks at both boundaries, in both Settings modes, including
velocity-zero releases, held notes and post-boundary normal-note recovery.

## User-supplied REAPER render observation

The user supplied `Midi_Note11_AND_Midi_Note12.wav` and its `.RPP` project on
2026-09-24. The project records three Note 11 On/Off pairs followed by three
Note 12 pairs at 136 BPM. Its saved processor state has the optional correction
disabled (`monoNoteZeroCorrection=0`). The WAV is stereo 24-bit PCM at 44.1 kHz,
about 8.82 seconds long.

Static waveform measurements show clearly nonzero audio during all six note
groups. Note 12 follows the Note 11 group and sounds in this particular sequence;
there is no demonstrated Note 11-induced lockup here. The Note 11 samples also
have a markedly different spectrum from the Note 12 samples, including strong
high-frequency energy. The supplied render predates/inferredly bypasses the new
12–120 adapter filter, because Note 11 is audible; it therefore serves as a
reference observation, not as acceptance evidence for the new filter.

Scope: this is one user-generated sequence in one saved state and one render. It
does not establish that Note 11 is safe in every firmware mode or that Notes
0–11 are all buggy. The 0–11 cutoff remains the user's product-range decision,
with Note 0's native MONO failure as the specific reproduced safety rationale.
