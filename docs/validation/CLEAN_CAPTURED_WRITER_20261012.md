# Internal Clean captured writer checkpoint

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
