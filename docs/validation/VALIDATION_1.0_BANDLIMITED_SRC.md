# Milestone 3B — band-limited sample-rate conversion

2026-09-20. Baseline main `ff19405379edcd4287390dfada33347d30bb6499`
(merged PR #8). Both baseline macOS and Windows workflows passed before work.
No firmware/core changes, new parameters, plugin IDs, release or installed
plugin changes. This remains a development checkpoint.

## Implementation and timing

Replace linear interpolation with a causal Blackman-windowed sinc resampler.
257 fractional-phase tables (256 intervals with coefficient interpolation),
321 stored taps, radius-126 window and cutoff 0.475 times the lower of native
and output sample rates. The extra stored taps accommodate host-latency rounding;
coefficients outside the window are zero. Each phase has normalized DC gain.
The native source rate stays 49096 Hz; firmware envelopes, operators and MIDI
processing are not changed.

Coefficient construction/allocation is non-realtime, in construction/prepare.
Rendering has fixed doubled-ring history, table reads, multiply/adds and no
allocation or locks. No host-block-sized lookahead is introduced. The causal
filter adds latency; its centre is chosen to match an integer host sample count:

| Host rate | Reported latency | Milliseconds |
| --- | --- | --- |
| 44100 Hz | 115 samples | 2.608 |
| 48000 Hz | 126 samples | 2.625 |
| 96000 Hz | 251 samples | 2.615 |

JUCE receives this latency during prepare, after releasing the engine lock.
A reentrant host-listener test saves processor state during latency notification
to guard against lock inversion. Impulse-centroid error is below 0.00002 host
samples in the three local measurements (test tolerance 0.02 samples).
Firmware/serial/envelope response time is additional and is NOT advertised as
fixed plugin latency. DAW compensation behavior still needs REAPER acceptance.

## Reproducible ROM-free spectral gates

The production path receives synthetic native-rate sines. One second settling,
one second coherent measurement; gains are relative to a 1 kHz reference.

| Measurement | Linear baseline (3A) | New local result |
| --- | --- | --- |
| 20 kHz at 44.1 kHz | -5.02 dB | -0.00094 dB |
| 23 kHz folded to 21.1 kHz at 44.1 kHz | -6.79 dB | below -120 dB |
| Worst of tested 44.1 kHz folded tones | not swept in 3A | -89.82 dB |
| 39.096 kHz image from 10 kHz input at 96 kHz | -24.88 dB | below -140 dB |

Very low individual readings can lie near a filter zero/numerical floor; do not
claim a uniform -120/-140 dB stopband. Automated limits deliberately have margin:
selected 1/10/20 kHz gains within 0.15 dB; tested folded tones below -70 dB;
the selected 96 kHz image below -80 dB. Probes include 22.2/22.5/23/24/24.5 kHz.
These are discrete engineering checks, not an exhaustive frequency sweep,
perceptual guarantee or measured comparison to physical DX7 hardware.

The upper transition band is intentionally attenuated: e.g. 23 kHz at 48 kHz
is about -17.38 dB. Existing project state/parameter identities are preserved,
but old renders will not be bit-identical: high-frequency coloration and filter
delay change intentionally. Hardware-fidelity claims still need reference audio.

## CPU and regressions

The 12-case local-ROM stress grid still produces exactly equal samples for the
same per-rate event timeline at 64/128/256/512 buffers. All note-range, sustain,
restart, state, edit-order and host-publication regressions are retained.

Local arm64 Release stress callback totals for about 0.5 seconds of audio were
roughly 30–31 ms at 44.1/48 kHz and 36 ms at 96 kHz. Earlier linear measurements
were about 23–25 ms. This is an indicative cost increase, not a controlled CPU
benchmark or an assurance about other machines. Per-block maxima are logged,
but scheduler-sensitive wall times are not test pass criteria.

## Remaining gates

Local macOS arm64 Release: full CTest 9/9 passed in 44.20 seconds, including
five opt-in local-ROM tests and reentrant latency publication. Development VST3
build and strict ad-hoc signature verification passed. These are local results;
new-source CI and actual DAW acceptance remain separate gates.

- Full allocation instrumentation, long-running automation/serial overload and
  host/device contention acceptance remain in milestone 3.
- DAW latency compensation, live-play feel and representative patch listening
  must be checked on the development build before 1.0.0 acceptance.
- Fresh macOS Universal / Windows CI and PR approval precede the next chapter.
- Milestone 4 PERFORMANCE/SETTINGS is not implemented by this change.
- User ROM stays local; no firmware data is added to source, CI or artifacts.
