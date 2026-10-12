# Classic/Clean integration and regression review — 2026-10-12

## Scope / Hatókör

**HU:** #188, main baseline `2d6c837247cfb013ee8ce83b58154bf3ff52c304`.
Friss macOS Release tesztbuild, a rögzített JUCE/Retromulator forrásokkal.
Csak privát original Yamaha DX7 MkI v1.8 firmware; a 48 KB-os kombinált
fixture firmware-részének SHA-256 értéke megegyezik a telepített 16 KB-os
original v1.8-éval. Firmware, gyári bank, build és DAW-projekt nincs a commitban.
Ez integrációs/review bizonyíték, nem végleges release- vagy csomagelfogadás.

**EN:** Fresh macOS Release test build of #188 against the stated main baseline,
using the pinned JUCE and Retromulator sources. Private original v1.8 only;
the combined fixture's firmware hash matches the installed raw v1.8 image.
No firmware, factory bank, build or DAW project is committed. This evidence
does not authorize publication or certify final release packages.

## Review

**HU:** A review két P2 hibát talált, mindkettő javítva és célzottan tesztelve:
a pending projekt ROM-completionje érvényteleníti a régi SETTINGS választást,
így az nem írhatja felül a visszahívott hangolást; a ROM nélkül kiválasztott
Clean mód a firmware betöltésekor cold-installt kap. A teljes tulajdonosi
pillanatképet ellenőrizzük a módosítások előtt. A kompatibilis/mismatched ROM
közti különbség, a régi Classic állapotok és a 148 hostparaméter megmaradnak.

Reviewed production changes: coherent captured mode/payload encoding; valid
Clean admission through existing project/ROM validation; legacy Classic and
12 malformed-schema rejection controls; SETTINGS token validation before
mutation, independent MONO policy, notifications outside the engine lock;
per-instance ownership and no new host parameters; dialog component lifetime.
GitHub review of `4df15a4` identified two P2 boundaries, addressed in this
follow-up: compare the full SETTINGS owner snapshot, including readiness, so
pending completion cannot overwrite recalled tuning with an old dialog value;
cold-install ready owner intent under the quiescent ROM-load lock so a fresh
pre-ROM Clean selection does not start with a Classic-to-Clean fade. Regression
controls cover both cases without test-only intent injection. Obsolete gating
comments updated. Final-head review/CI remain independent merge gates.

## Added integrated audio regression

`testLiveSettingsAudio` uses three real processors at 44.1/48/96 kHz and
32/64/257/512-sample callbacks (12 combinations). It exercises public SETTINGS
Classic→Clean→Classic and rapid latest-intent requests while a real MIDI note
is held/sustained, then releases sustain and renders the tail. Assertions:
finite and nonzero output and genuinely different Clean samples; exact default Classic reference; bit-exact unchanged
Classic peer versus independent control; stable reported latency and project
revision; final settled Clean at unity; normal writer saves latest Clean.
The scheduling interval is at least 3 ms per step, allowing real MIDI admission
and audible sustained output even at 96 kHz with 32-sample callbacks. An earlier
fixed-block window had no differing audible samples in a short trajectory;
that strengthened test failed and was corrected rather than dropping the
nonzero/different-output precondition. This is a bounded deterministic regression, not a long concurrent DAW soak,
spectral/listening evaluation or proof of a silent tail for every factory patch.

## Results / Eredmények

- Fresh `vdx7_ci_checks` build: PASS.
- Fresh macOS VST3/AU/Standalone builds: PASS; copying/installing disabled.
- Python packaging/provenance/workflow regression: 94/94 PASS; test-registration
  self-tests PASS. These do not certify new release artifacts.
- Public integrated audio matrix: PASS (12 combinations).
- Private raw-v1.8 native engine/SRC suite: PASS at all three rates. Covers
  held/sustain/rapid requests, partition invariance (1/64/257/511), unchanged
  Classic peer, cold-install reference, latency/RAM preservation, rejected ROM
  and 24 interrupted ROM/MONO rebuild trajectories. The 768 interrupted reset
  phase/direction/fade/cancellation controls also PASS.
- Initial full combined-ROM CTest: 58/59 PASS; `vdx7_processor` failed its
  explicit desktop-access precondition in the sandbox. Re-run with desktop
  access: profile and processor 2/2 PASS. Do not label the initial run 59/59.
- Pre-review-fix rebuilt full regression: 59/59 PASS (245.57 s). This preceded
  the two production fixes and is not their final-head PASS.
- Final post-review-fix full rebuild/regression: 59/59 PASS (238.14 s), including
  the public audio matrix and both review regressions. Readiness-token owner
  control also PASS without ROM, for platform/sanitizer CI coverage.

**HU összegzés:** a javítás utáni teljes helyi regresszió 59/59 PASS; a nyilvános
hangút 12 kombinációja, a két review-hiba kontrollja és a 94 Python-teszt PASS.
Mindhárom macOS formátum buildje PASS, telepítés nélkül. Az első, korlátozott
GUI-futtatás hibáját nem tüntettük el; a megfelelő hozzáféréssel végzett teljes
újrafuttatás zöld. Ez nem új Windows-ROM vagy végleges csomag/DAW-elfogadás.

## Remaining publication gates / Publikálás előtt

Final-head Windows/macOS/sanitizer and completed review, current-main ancestry,
then SHA-bound serial merge and separate post-merge main checks. Final candidate
binary/package provenance, Windows and standalone host acceptance, broader
REAPER rate/buffer/GUI/freeze matrix, long stress and controlled spectral/RMS/CPU
measurements remain separate release gates. Earlier installed REAPER smoke is
documented in [its report](REAPER_SMOKE_20261012.md), not attributed to this
fresh test build. Sole active execution order: [development plan](../development/DEVELOPMENT_PLAN.md).
