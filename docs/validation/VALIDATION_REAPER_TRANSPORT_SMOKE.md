# REAPER transport smoke — 2026-09-24

Actual REAPER 7.80, eight-instance corrected MONO test project, SSL 2+,
44.1 kHz / 512 samples, master -18 dB. Same previously verified workspace
plugin build; no plugin replacement or source-code change in this round.

## Operations and evidence

Invoked Play/stop repeatedly through the native Actions window; observed Play
state alternate on/off. These initial operations were not audio-recorded and
are UI-operation checks only, not measured stop/start acceptance.

Set loop points to the selected full MIDI item (0–11.911 seconds), enabled
repeat, and started playback. Recorded actual master output with File / Save
live output to disk, stereo 24-bit PCM, including stopped output (no silence
gating, no stop-on-first-stop). During recording, invoked Go to start of project;
Play state remained on afterwards. Then stopped playback (Play state off),
disabled repeat and allowed the stopped output to continue recording before
ending the bounce. REAPER confirmed live output saving completed.

Local recording: `mono-transport-live.wav` (not distributed).
- Duration: 118.85133786848073 seconds; 44100 Hz stereo.
- Maximum absolute normalized sample: 0.36573266983032227.
- Full-scale clipped samples: 0.
- Last five seconds: exact digital zero.
- Every ten-second window over the first 90 seconds contains nonzero output;
  windows starting at 90 seconds and later are silent.

PASS for aggregate sounding playback and final stopped silence in this smoke
sequence. No crash observed. The active repeat setting and sustained playback
exercise the short fixture repeatedly; no sample-accurate loop-count assertion
or timestamped seek-event oracle was recorded.

## Limits — not silently closed

No proof of individual-instance pitch, brief-dropout absence, exact seek moment
relative to held Note 0, or transport-boundary sustain combinations. The MIDI
fixture has repeated zero notes, legato orders and later 72 notes, but no pedal
events. Initial stop/start operations lack a recorded audio oracle. Those
specific acceptance cases remain PARTIAL / NOT RUN, not a complete transport
PASS. No new automated suite was run and native known failure is unchanged.

Cleanup: stopped playback and live bounce, repeat off, removed time/loop
selection, saved only the separate test project. Other user project untouched.
