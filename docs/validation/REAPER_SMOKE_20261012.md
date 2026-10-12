# REAPER fejlesztési VST3 részteszt / Development VST3 smoke test

2026-10-12. Nem release-jóváhagyás / Not release approval.
Az egyetlen aktív munkaterv / Sole active work plan:
[DEVELOPMENT_PLAN.md](../development/DEVELOPMENT_PLAN.md).

## Azonosítás / Identity

- REAPER 7.82 macOS; real-time 44.1 kHz / 512 samples.
- User-installed VDX7 VST3; UI version `1.0.1-dev`.
- Executable SHA-256: `c40cb0d9731b36cb691811977bdb73acd5b108fb18da188a257d90d66cce69d3`.
- Original v1.8 private firmware; ROM/banks and project payload not published.
- Source commit / final release package provenance NOT ESTABLISHED.
- Separate test project; existing user project and installed plugin not modified.

## Eredmények / Results

| Ellenőrzés / Check | Eredmény / Result |
|---|---|
| VST3, original v1.8 ROM, ROM1A BRASS 1 betöltés / load | PASS |
| SETTINGS 100%, gomb/mező átfedés / button-field overlap | PASS: nincs / none |
| Classic→Clean→Classic→Clean Apply | PASS: nincs észlelt fagyás / no observed hang |
| Clean és tuning=12 save/reopen | PASS |
| Tuning=25 és Classic elvetése / Cancel | PASS: Clean és 12 megmaradt / retained |
| MIDI 36/60/84, akkord / chord, CC64 sustain/release | PASS: látható audio output / visible audio output |
| Két duplikált Clean példány rövid párhuzamos lejátszása / two duplicated Clean instances | PASS: mindkét mérő aktív / both meters active |
| Classic és Clean offline export, 44.1/48/96 kHz | PASS: hat nem néma fájl / six non-silent files |
| Clipping és beragadt sustain e fixture-ben / in this fixture | PASS: clip=0, final 0.5 s exact silence |

## Mérések / Measurements

24-bit PCM stereo, full-speed offline, approximately 8.412 s per file.
48/96 kHz: 'Mix/process FX at project or hardware sample rate' unchecked.

| Mode | Hz | Linear peak | Linear RMS | Final 0.5 s RMS |
|---|---:|---:|---:|---:|
| Classic | 44100 | 0.785753 | 0.090812 | 0 |
| Clean | 44100 | 0.833290 | 0.091479 | 0 |
| Classic | 48000 | 0.701490 | 0.090568 | 0 |
| Clean | 48000 | 0.715037 | 0.090552 | 0 |
| Classic | 96000 | 0.766212 | 0.090721 | 0 |
| Clean | 96000 | 0.718280 | 0.090966 | 0 |

HU: tuning=12 firmware-egység; örökölt projekt-tempo 136 BPM. Nem spektrális,
hanghűségi, latencia- vagy kiegyenlített hallásos minősítés. A beragadásmentesség
csak a konkrét rövid fixture-re vonatkozik, nem minden MIDI/transport esetre.

EN: tuning=12 firmware units; inherited project tempo 136 BPM. Not spectral,
fidelity, latency or level-matched listening acceptance. Absence of stuck notes
applies only to this short fixture, not all MIDI/transport cases.

## Nem futott / NOT RUN

HU: hosszú stressz; minden GUI-skála; eltérő, független példánymódok reload után;
automation, MIDI-channel határok, MONO változatok, hiányzó/eltérő ROM recovery,
freeze/unfreeze; valósidejű 48/96 kHz és más bufferek; Windows, standalone,
AU/egyéb host; kontrollált Classic null/spektrum/hallásos és teljes csomagteszt.

EN: prolonged stress; all GUI scales; independently different instance modes
after reload; automation, MIDI-channel boundaries, MONO variants, missing/mismatched
ROM recovery, freeze/unfreeze; real-time 48/96 kHz and other buffers; Windows,
standalone, AU/other hosts; controlled Classic null/spectrum/listening and full
package acceptance. Final candidate must have its own results or explicit scoped deferrals.

## Helyi bizonyíték / Local evidence

Project, MIDI, six WAV files and PCM checker remain outside Git in workspace
`work/VDX7-REAPER-validation-20261012/`; original local report `RESULTS.md`.
Only this text report is published to source control: no ROM, DAW payload or
binary assets. This report records completed tests, not a second roadmap.
