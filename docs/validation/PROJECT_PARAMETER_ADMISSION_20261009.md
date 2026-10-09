# Projektparaméterek ellenőrzése visszatöltés előtt

Dátum: 2026-10-09. Baseline: `6d66610109a867e9c1494f95081c594a2a8506ea`.
Javítási ág: `fix/validate-project-parameters`. A hibás projektparaméterek
ellenőrzése a Classic/Clean integráció előtti állapotvédelmi javítás;
nem új hangmód, firmware-támogatás vagy release-elfogadás.

## Reprodukált hiba

A valódi `setStateInformation()` ellenőrzés nélkül adta át a PARAMETERS
gyermekfát az APVTS-nek. A JUCE elfogadja a `nan` szöveget nem véges számként,
és a prefix-konverzió hibás szöveget is számmá alakíthat. A projekt ekkor már
módosította a routingot, az imported katalógust, a pending payloadot és a MIDI
epochot. A hibás hangerőérték nulla kimeneti erősítést okozhatott; összeomlást
vagy nem véges audio mintát a reprodukció nem igazolt.

A regresszióban a PARAMETERS értékét `nan`-ra változtatjuk, miközben a routing
és katalógus másik projektre váltana. A valódi bináris restore a javítási guard
ideiglenes kikapcsolásakor exit 1-gyel megbukott:

```text
FAIL: invalid parameters preserve routing and exact imported catalog ownership
```

## Javítás és kompatibilitás

A paraméterellenőrzés minden projektállapot-módosítás előtt fut. Az ismert
hostparaméterek csak teljes decimális vagy tudományos számformátumot kaphatnak,
amely double és float formában is véges. NaN, infinity, túlcsordulás, üres érték,
hibás vagy csonka szám és ismert ID duplikálása elutasítja az egész projektet.
Több PARAMETERS fa és ismert ID nem PARAM típusú gyermekben szintén elutasított.

A hiányzó paraméterfa, részleges régi paraméterlista és hiányzó value továbbra
is a korábbi restore/default viselkedést használja. Az APVTS által figyelmen
kívül hagyott ismeretlen ID-k nem kerülnek a hostparaméterekbe. Véges, tartományon
kívüli értékeknél megmarad a korábbi tartománykorlátozás: a packed RAM továbbra
is irányadó, ezért a támogatott legacy EG/fine nyers adatokat nem írjuk át és
nem utasítjuk el pusztán a GUI tartománya miatt.

A `VDX7ImportedProcessorTests.cpp` mind a 148 ismert paraméteren NaN-rejectiont
vizsgál, továbbá a globális hangerő/kerék vezérlőkön hibás számformátumokat.
Live és már visszatöltött/pending helyzetben összeveti a teljes bináris
projektmentést, a routingot, a katalógus tulajdonlását, a MIDI epochot és a
katalógus revisiont. Pozitív kontrollok fedik a tudományos jelölést, a defaultot,
az ismeretlen ID figyelmen kívül hagyását és a régi részleges/RAM-only state-et.
A regresszió a meglévő ROM-free CI-teszt része; nincs új workflow vagy tesztszűrő.

## Helyi ellenőrzések

macOS ARM64, JUCE 9.0.3, változatlan 148 hostparaméter és pluginazonosság.

- Negatív kontroll a guard nélkül: FAIL, exit 1; guard visszaállítva.
- Teljes ROM-free CTest: 21/21 PASS.
- Python regressziók: 94/94 PASS, nincs SKIP.
- `vdx7_ci_checks` és macOS VST3/AU/Standalone development build: PASS.
- Tényleges ROM-free tesztleltár és checker self-test: PASS.
- Célzott snapshot/admission kontroll firmware nélkül és privát original
  v1.8 firmware-rel: Release és ASan/UBSan PASS.
- Legacy 127-es EG és 100-as fine bankadatok importja, passive syncje,
  single/bank exportja, pending és loaded projekt-visszatöltése: PASS.
- Diff whitespace-ellenőrzés: PASS.

ASan: `detect_leaks=0:halt_on_error=1`; UBSan:
`halt_on_error=1:print_stacktrace=1`. Ez célzott helyi sanitizer eredmény,
nem teljes sanitizer mátrix, LeakSanitizer vagy ThreadSanitizer elfogadás.
A privát 16 384 bájtos ROM SHA256-ja:
`6e7aa7b3605131c124914abbc74078acf7bd78354379d6b3ad78373ab7bfd383`.
Firmware vagy Yamaha factory bank nem kerül Gitbe vagy kiadási csomagba.

## Nyitott kapuk

A végső PR-fej Windows/macOS/ASan–UBSan CI-je és review-ja, majd merge után
a main Windows/macOS futásai külön kapuk. A helyi build nincs telepítve.
Valódi REAPER/hallásos próba, Classic null-difference mérés és új telepítő/source
csomagelfogadás ebben a körben nem történt. A publikált 1.0.1 változatlan.
Clean admission továbbra is fail-closed; a kívánt mód/payload/pending/ROM-epoch
teljes tranzakciós integrációja az [egységes fejlesztési terv](../development/DEVELOPMENT_PLAN.md)
nyitott D5 lépése marad.
