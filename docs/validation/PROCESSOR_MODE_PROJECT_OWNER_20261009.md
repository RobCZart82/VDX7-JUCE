# Hangmód és projekt közös állapotkezelőjének processor bekötése

Dátum: 2026-10-09. Baseline: `541c39cd8ee6bba0643df52dbfd90716c3a6b268`
(#174 után). Ág: `feature/processor-mode-project-state`.
A D5 részlépés a valódi processor pending állapotát, projektgenerációját és
Classic kívánt módját egy ownerhez rendeli a meglévő engine lock alatt.
Clean renderer, hangmódváltás és SETTINGS még nincs bekapcsolva.

## Megvalósított állapotkezelés

A korábbi külön `projectRevision_` helyét a `VDX7SoundModeOwner` revíziója veszi
át. Nem fut két külön projektgeneráció: a saved-ROM restore, a kézi ROM-load,
az autodetect és a pending completion ugyanazt a számlálót használja. A #174
stale-ROM védelme, példányizolációja és a revíziókimerülés elutasítása megmarad.

A már megszerzett motorzárból az owner új `*Locked` adapterei hívhatók.
A `std::unique_lock` tényleges tulajdonlása és mutex-azonossága ellenőrzött;
másik mutex vagy fel nem vett/felengedett lock nem ad hozzáférést. Az adapter
nem vesz fel újabb lockot, így nincs saját mutexből történő önblokkolás.
A meglévő önálló, self-locking owner API ugyanazokat a műveleteket használja.

A teljes projektvalidáció és a mono-policy elfogadása után a payload és a
kívánt Classic mód egy engine-tranzakcióban települ. Az új projekt ekkor
pending, még akkor is, ha a példányban egy korábbi motor már fut: az új RAM
és ROM-identitás még nem került együtt elfogadásra. A compatible completion
a jelenlegi projektgenerációt és a meglévő `savedStateMatchesRom()` ellenőrzést
ugyanazon lock alatt vizsgálja; a RAM-install és az owner pending/ready
átállítása nem válik két külön tranzakcióvá. Nem használ előre kiszámolt
mutable kompatibilitási eredményt.

ROM-only boot/reload és a nem-valósidejű állapotlekérdezés a tényleges
`engine_.isLoaded()` értékből frissíti a nem-pending readiness-t. Egy pending
projektet ez a frissítés nem oldhat fel: eltérő firmware mellett a betöltött
motor és a projekt készültsége különböző lehet. Mentéskor az owner skalár
pillanatképe ugyanott rögzül, mint a payload. Egy már explicit Classic
mezőpárral őrzött pending projekt e captured kívánt értékből kódolódik.

## Kompatibilitási kapuk

Normál és legacy pending save továbbra sem hoz létre új `soundMode` /
`soundModeVersion` párt. A már explicit Classic pár megőrzése nem működő
Clean mód hirdetése. Érvényes Clean és hibás sémapár az egész restore
módosítása előtt elutasított. Nincs publikus módváltó, GUI-kapcsoló, új host-
paraméter, automatikus firmware-policy váltás vagy audio-dispatch.

Revision, pending, ready, gain és egyéb runtime ownership nem kerül a mentett
projektbe. A 148 hostparaméter ID/sorrend, pluginazonosság és Classic DSP
változatlan; az audio callbackhez nem adtunk lockot, allokációt vagy I/O-t.
A fejlécben elérhető snapshot csak nem-valósidejű, read-only diagnosztikai
állapotnézet; nem írható Clean kívánt módot kínáló felhasználói funkció.

## Ellenőrzések

macOS ARM64, JUCE 9.0.3; Release és célzott RelWithDebInfo ASan/UBSan.

- Actual processor ROM-free owner/pending mentés: PASS. Friss Classic,
  valódi pending payload/revízió, Clean-rejection teljes állapotmegőrzés,
  régi explicit Classic capture közbeni új legacy recall és runtime mezők
  kihagyása. A leválasztott régi snapshot nem módosítja az új projektet.
- Privát original v1.8 actual processor: PASS. Első boot revízióemelés nélkül;
  hiányzó ROM; eltérő ROM-identitás mellett betöltött, de pending projekt;
  megfelelő jelenlegi ROM melletti RAM/mód-completion; feedback 6 megőrzése;
  prepare/release és ROM-only reload kívánt mód/revízió megtartása.
- #174 stale completion reentráns és valódi kétszálas kontrollok, kézi load
  és teljes restore: PASS a shared owner generációjával is.
- Owner adapter: PASS a hibás mutex, fel nem vett token, pending bypass tiltás,
  stale completion predicate/commit tiltás és friss readiness kontrollok.
  A noexcept predicate követelmény fordításkor is ellenőrzött.
- Meglévő önálló valódi szálas owner regressziók: PASS. A szándékosan cached
  ROM-install és ROM-completion negatív adapterek exit 1 FAIL-t adnak, ahogy
  elvárt. Ez teszt-only negatív kontroll, nem a production új hibája.
- Teljes ROM-free CTest: 21/21 PASS; Python: 94/94 PASS, nincs SKIP.
- CTest tényleges regisztráció és checker self-test: PASS.
- Diff whitespace és a három frissített dokumentum 31 helyi hivatkozása: PASS.
- `vdx7_ci_checks` és macOS VST3/AU/Standalone dev-build: PASS.
- Célzott actual-processor snapshot/admission/generation/owner kontrollok:
  ASan/UBSan PASS firmware nélkül és privát v1.8-cal; standalone owner is PASS.
- Privát state-transition runner: PASS, voice/operator és routing-publication
  kontrollokkal. Privát pre-ROM recall, legacy/Classic/Clean admission és
  legacy EG 127 / fine 100 bank import/export/pending/loaded recall: PASS.

ASan: `detect_leaks=0:halt_on_error=1`; UBSan:
`halt_on_error=1:print_stacktrace=1`. Nem teljes helyi sanitizer mátrix,
LeakSanitizer vagy ThreadSanitizer bizonyíték. A privát firmware 16 384 bájt,
SHA256: `6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Yamaha firmware/factory adat nem kerül Gitbe, CI-be vagy release csomagba.

## Nyitott integráció és kiadási kapuk

Ez a Classic-only project/pending owner részintegrációja, nem a teljes D5.
APVTS publikáció, önálló automation írások és az összes admission utáni
host/metadata frissítés még nem egyetlen atomikus restore tranzakció.
Az ugyanazon generáción belüli két kézi ROM-load latest-wins sorrendje sem
része ennek a körnek. A commitok nem-valósidejűek, és a korábbi JUCE/RAM
másolási műveleteket használják; nincs általános allokáció- vagy OOM-garancia.

Production audio-visit, Clean writer/admission, UI-request és renderer/lifecycle
még nincs bekötve. A renderer-adapterben minden readiness-változtató lifecycle
út és az audio-lock utáni kívánt mód olvasása külön bizonyítandó; a mostani
non-realtime readiness view nem e teljes audio-lifecycle elfogadása.
Clean payloadot nem őrzünk kész funkcióként DSP nélkül.

Final-head Windows/macOS/sanitizer CI, érdemi review, majd merge utáni main
Windows/macOS külön kapu. Teljes combined-ROM/factory katalógus tesztmátrix,
Classic null-difference, Clean spektrum/CPU/átmenet, REAPER/hallásos próba és
új telepítő/source csomag e körben NOT RUN. A dev-build nincs telepítve;
publikált 1.0.1 és release assetek változatlanok. A következő lépések az
[egységes fejlesztési tervben](../development/DEVELOPMENT_PLAN.md) maradnak.
