# D5 stopped release owner coverage

2026-10-12; baseline `b9b2da8aab716748322d5203ef6de87ade5cf7a7`.
Ág: `test/stopped-release-mode-owner`. Csak teszt és dokumentáció módosítás.

Actual processor: reset request, zárolással contended audio callback,
kétszeri releaseResources, majd prepare/audio. #184 review után explicit
precondition: hostResetRequested false és hostResetPending true a release előtt.
Így a pending flag törlésének kihagyását a release utáni assertion elkapja;
a korábbi teszt ezt a jelzőt csak kezdeti false állapotban vizsgálta.
Pending Clean owner nem válik admitted rendererré. Ready owner kívánt módja
és projekt-revíziója megmarad; prepare cold-installja változatlanul ellenőrzött.
A korábbi reset request/pending flag megszűnik. Nem teszteli a cleanup alatt
érkező párhuzamos resetet. A Clean owner injection test-only, nem public recall.

Az első privát kontroll FAIL volt: hibásan teljes native snapshot azonosságot
kért. A resetMidiLifecycle normál firmware clockokat is futtat; a scan phase
nem köteles változatlan maradni. A tesztet intent/owner invariánsra javítottuk,
production módosítás nélkül. Ez nem reprodukált termékhiba.

PASS: Windows Release processor teszt build; ROM-free és privát original-v1.8
snapshot-only kontroll. A privát 48 KB fixture saját valid bank test-copy,
nem gyári bank-elfogadás. Firmware/bank tartalom nincs Git-ben.
PASS: ROM-free CTest 22/22 a tesztkörben írható TEMP/TMP mellett.
NOT RUN: új final-head macOS/sanitizer CI, installed REAPER/hallásos acceptance.
#183 final-head Windows/macOS/sanitizer és merged main Windows/macOS PASS.

Irányadó: [egységes fejlesztési terv](../development/DEVELOPMENT_PLAN.md).
