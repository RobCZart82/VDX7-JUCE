# Internal Clean captured writer checkpoint

## Current closure — 2026-10-13

#188 is merged, main `624f68ca01cfa8c6c63c66c650c11c2505e4c99f`.
Final-head `349111fe38acec0bf82ff400c38bf0bb6b9226c7` platform/sanitizer PASS,
merged-main Windows/macOS PASS. Draft/reader-closed/CI-pending statements below
are historical checkpoints, not the current feature state. SETTINGS and
validated Clean save/recall are implemented on main. Release acceptance is
separate; the owner reports latest-build Windows/macOS tests passed, but exact
binary/package and itemized acceptance evidence is not yet recorded.

## Current status index — 2026-10-12

Latest integrated follow-up: [full regression/review report](CLASSIC_CLEAN_INTEGRATION_REVIEW_20261012.md).
It records the public audio matrix and two review fixes; final-head CI/merge
status must not be inferred from historical checkpoints below.

This report preserves dated evidence, not one cumulative final-head PASS.
Main baseline is `2d6c837247cfb013ee8ce83b58154bf3ff52c304` (#187).
Draft #188 now includes validated Clean writer/reader, SETTINGS selection,
policy/ownership tests, revised spacing and negative Apply controls.
Code/test checkpoint: `b299f325eec4309c1cf4724a71ddec2ce52e0dd3`; owner-feedback
documentation checkpoint: `ba49ecff4ab31b675c8f448d5702beac76eaea6a` (platform
checks in progress at this inspection). Earlier `520c848` platform checks PASS.
Remaining gates: final-head review/platform checks, full integrated coverage,
measurements and exact-candidate host/package acceptance. Historical "reader
closed"/"SETTINGS missing" passages below describe earlier commits only.
The sole active execution order is in DEVELOPMENT_PLAN.md, not this report.

## Ready public recall lifecycle — 2026-10-12

Follow-up baseline: `639d0da8434bea77e90444c2cadc7bf6fdb24828`.
Original private v1.8 firmware: ready Clean→Classic→Clean public binary recall
increments the project revision once per restore and cold-installs Classic.
The returned Clean owner survives reset, repeated release, prepare and audio
at 44.1/48/96 kHz; the native renderer settles in Clean at unity.
PASS: rebuilt macOS imported processor target and private `--snapshot-only`
run, including existing stale ROM-completion and ownership controls.
This verifies ownership and native mode, not audible output or listening quality.
Windows/sanitizer CI and the complete lifecycle/audio/SETTINGS gates remain.
Firmware is outside Git and is not included in the change.

## Public pending lifecycle follow-up — 2026-10-12

Baseline Draft head: `890f58379833dab513fe8f899fa113d0c8956595`.
Public binary Clean recall (no test-only intent injection) now has explicit
ROM-free coverage for reset, repeated release, prepare and audio at
44.1/48/96 kHz. Desired mode, pending/readiness and revision are preserved;
the saved mode/routing pair survives. Public legacy Classic recall replaces
the owner, and recalling the saved Clean state replaces it again, with one
revision per accepted restore.

PASS: fresh macOS Release shared code and imported processor test build;
`--snapshot-only` and default ROM-free imported processor run; diff check.
Dependencies match the pinned JUCE and Retromulator sources. Existing Draft
head platform CI passed; this follow-up needs its own CI. NOT RUN this round:
private ROM, ready/mismatched-ROM lifecycle, Windows/sanitizers, installed DAW
and listening tests. Earlier reader-closed paragraphs below are historical.

## SETTINGS and policy integration (2026-10-12)

### Owner feedback — 2026-10-12

The owner supplied a screenshot of the revised SETTINGS layout: Sound mode and
buttons are visibly separated at the shown size. The owner reports the tried
functions work well, Classic/Clean has a clearly audible, musically favourable
cleaner/crystalline contrast, and repeated SETTINGS adjustments did not freeze
the application. These are owner-reported visual/listening/stability observations,
not a controlled full acceptance matrix. Exact binary hash, host/platform/version,
sample rates, buffers, patch inventory and save/reopen cases were not supplied
with this feedback. Do not infer Windows PASS, complete concurrency coverage,
all-scale layout acceptance or permission to publish. Keep remaining exact
candidate/host and measurement gates open.

Negative Apply follow-up on `520c848cdd068b6e0b586b680f83cd3e64504a18`
(Windows/macOS/sanitizer PASS): seven invalid/unavailable controls cover tuning
limits, MIDI-channel limits, invalid mode, missing ownership token and tuning
without ready firmware. Every rejection checks exact saved payload and owner
preservation. Fresh macOS processor build and ROM-free snapshot run PASS.
No runtime behavior changed; final-head CI and real GUI/host acceptance remain.

Owner screenshot follow-up: information row height reduced 24→20 logical pixels;
64 logical pixels of explicit footer space added after Sound mode, before the
button row. With 11 information lines this adds about 30 logical pixels to the
natural dialog height while keeping extra separation below the final combo.
All custom components are removed/deleted on modal completion. This fixes the
layout budget, not an assertion of visual acceptance on every display/scale.

Follow-up on `71b4dbc48b5e61bde9aa879092a29274437c2853` (all three platform
checks PASS): deterministic reentrant and real-thread SETTINGS requests at the
detached save boundary, Classic→Clean and Clean→Classic. Old save retains its
captured mode, live intent changes without a new project revision, next save
contains the new mode. Fresh macOS processor build and ROM-free snapshot run
PASS. New-head CI remains a separate gate; no complete concurrency claim.

Classic/Clean selector added to SETTINGS with Classic default and Apply/Cancel
semantics. No new host parameter. Apply validates captured mode/revision under
the existing engine mutex before settings mutation. MONO policy, tuning/channel
and mode selection share this transaction; notification occurs after unlocking.
Tests cover no-ROM selection, instance isolation, save/reopen, legacy Classic,
stale mode and newer same-mode project rejection, and ready Clean Native/Correct
rebuild at 44.1/48/96 kHz. Not complete concurrency/audio/visual/DAW acceptance.

PASS: fresh macOS shared-code/CI-test/VST3/Standalone builds; ROM-free CTest
22/22; private original-v1.8 snapshot-only processor run; diff check. Pending
legacy → UI Clean required writer to add the schema even when the old pending
tree had no mode properties; covered by the no-ROM SETTINGS test. A pending
Classic tree with a schema correctly encodes mode 0 (not legacy absence).
Full private stability run was attempted with the raw 16 KiB ROM and rejected
its fixture at "pending fixture: combined ROM"; that run is NOT PASS and needs
the combined fixture. Final-head Windows/macOS/sanitizer CI, visual dialog and
real host/listening acceptance NOT RUN locally. No installed binaries changed.

## Stopped Clean firmware reload follow-up (2026-10-12)

Baseline: `9b05fb1a4b9872150127455fac2314c9066a5562`; its Windows/macOS/
address-undefined CI passed. Added a stopped ready Clean original-v1.8 ROM
reload control: successful reload preserves desired mode, readiness and project
revision; reprepare and 32 audio callbacks retain ownership and settle Clean.
PASS: fresh macOS processor-test build and private snapshot-only run at
44.1/48/96 kHz. No runtime code change. New-head CI, full lifecycle/policy,
SETTINGS, listening and release acceptance remain outstanding.

## Firmware-identity follow-up (2026-10-12)

Baseline: `dcad45d8bb2615a332b3094540d941284fd4cd14`. Public Clean binary
restore with a deliberately incompatible saved ROM identity remains pending
after loading valid private original v1.8 firmware; owner/revision remain
unchanged, and normal save preserves Clean mode and the incompatible identity.
A subsequent compatible Clean restore becomes ready against the loaded ROM,
with exactly one new project revision. Independent processor instances retain
the existing missing-ROM completion coverage.

PASS: fresh macOS Release imported-processor build and private snapshot-only
run at 44.1/48/96 kHz. Only tests/documentation changed. NOT RUN this follow-up:
full plugin/CTest rebuild, Windows/sanitizer CI, installed DAW or listening.
Draft and final-head review/platform gates remain open; no release assets changed.

## Earlier Draft checkpoint: validated Clean reader és round trip

Az érvényes Clean read a közös payload admissionbe kerül; a korábbi valid
Clean rejection helyett pozitív pending/ready tesztek. Mind a 12 malformed
schema kontroll megmarad. No-ROM binary recall mód/payload preservation;
privát v1.8 ready save/reopen, matching ROM identity és feedback, prepare,
native settled Clean/unity három rate-en PASS. Classic writer legacy marad.
Az alábbi korábbi reader-closed megállapítások történeti checkpointok.

PASS: final Windows shared-code és két processor test build, no-ROM admission
és snapshot-only, private snapshot-only pozitív round trip. Első private run
exit 1 diagnosztika nélkül; a nagy új reopen test-példány heapre helyezése után
final build/run PASS. Nem production crash bizonyíték. NOT RUN: teljes új
plugin rebuild, final-head macOS/sanitizer, teljes privát lifecycle, hallásos
vagy telepített host acceptance. Draft marad; nincs release/jóváhagyás.

Baseline: `2d6c837247cfb013ee8ce83b58154bf3ff52c304`, 2026-10-12.
Ág: `feature/clean-captured-state-writer`.

A normal save writer az ugyanazon engine-lock capture-ben rögzített desired
Clean módhoz sémapárt ír a leválasztott payloadra. Classic kimenet továbbra
is legacy. Encoding közben nincs új owner-read, audio lock vagy callback.
Internal Clean setup csak test-only injection; nincs felhasználói request/UI.
Public reader a Clean state-et továbbra is mutáció nélkül elutasítja. Ez
átmeneti részlépés, nem round-trip elfogadás és nem publikálható kész funkció.

Actual processor kontroll: no-ROM normal capture után új Classic restore
reentránsan és külön szálon; saved Clean pair megmarad, live owner Classic,
legacy mentés változatlan, captured Clean restore továbbra is rejection.
A #187 pending capture kontrollja is megmarad. Nem új bizonyított termékhiba.

PASS: Windows Release shared code és processor tesztbuild; snapshot-only;
regisztrált ROM-free CTest 22/22; dokumentáció guard 3/3 és diff check.
A teljes CTest többi exe-je meglévő helyi buildből futott; nem teljes új
VST3/Standalone rebuild. NOT RUN: új final-head platform/sanitizer CI,
privát firmware, ready Clean binary recall, installed REAPER/hallásos teszt.
Nincs firmware, csomag, release/tag/asset változtatás.

Következő: teljes validated Clean admission/ready és pending recall pozitív
integráció, életciklus/mérések, majd SETTINGS. Addig a public gate zárt.

## Draft folytatás: actual internal Clean completion

Privát original-v1.8 firmware, saját valid bank test-copy: hiányzó ROM mellett
Classic payload restore, test-only Clean owner injection, majd valódi ROM/RAM
completion. Három rate (44.1/48/96 kHz), prepare és 32 audio callback után
ready/revision preservation, native settled Clean/unity és finite output;
normal writer Clean sémapár + matching ROM identity + feedback=6, release
után változatlan owner. A finite output nem bizonyít nem néma hangot vagy
hallásos minőséget. Public reader továbbra is Clean rejection.
Ez pozitív completion/writer részteszt, nem public binary Clean round trip.
PASS: final Windows processor-test build, privát snapshot-only kontroll a
fenti három rate-tel, dokumentáció guard 3/3 és diff check. NOT RUN ebben a
folytatásban: teljes új CTest/plugin rebuild, macOS/sanitizer és REAPER.
