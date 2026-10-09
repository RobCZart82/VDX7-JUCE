# D5 Classic/Clean: valódi processor-admission védelem

Dátum: 2026-10-09. Baseline main:
`2e42412c7da50eea10a17f222983e959a2e76310` (#170 már beolvadt).
Ág: `feature/classic-clean-processor-admission`.
Irányadó feladatlista: [DEVELOPMENT_PLAN](../development/DEVELOPMENT_PLAN.md).
Szerződés: [Classic/Clean state](../development/CLASSIC_CLEAN_STATE_CONTRACT.md).

## Hatókör és reprodukció

A valódi `VDX7AudioProcessor::setStateInformation()` korábban figyelmen kívül
hagyta a még előkészített `soundModeVersion`/`soundMode` párt. Egy különben
érvényes, de hiányos D5-metaadatú projekt telepíthette saját payloadját,
átállíthatta a mono-policyt, catalog/pending generációt, MIDI epochot és
hostparamétereket. Ez a tervezett új séma admission-hiánya; **nem állítás arról,
hogy az 1.0.1 már támogatott Clean-projekteket vagy hogy hanghibát reprodukáltunk**.

Először az új processzorteszt került be, production változtatás nélkül:

```text
vdx7_processor_tests --sound-mode-admission
PASS: legacy and explicit Classic restore positive controls
FAIL: rejected sound-mode restore preserves MIDI/catalog/dirty/pending generations
```

Ez az első hiányos pár (`soundModeVersion=1`, mód nélkül) előtt már lefuttatta
a két pozitív kontrollt. A baseline-futás itt megállt; a további negatív
esetek baseline-hibáját nem állítjuk külön lefuttatottnak.

## Javítás és regressziós bizonyíték

A közös D5 reader az XML/binary root dekódolása után, minden restore-mutáció
és ROM-fájl keresése/betöltése előtt validál. Hiányzó pár legacy Classic;
pontos `1`/`0` explicit Classic. Hiányos, ismeretlen, nem pontos pár reject.
**Érvényes `1`/`1` Clean is reject**, mert még nincs production Clean renderer:
nem állítjuk helyre Classic hanggal, nem csupán eldobjuk a módjelzést.
A writer, desired-mode owner, DSP/SRC és UI nem aktiválódott. Normál save nem
kezd új mezőket írni; korábban elfogadott pending explicit Classic párját
változatlanul megőrzi. Nincs új hostparaméter, DSP vagy callbackbeli művelet.

`Tests/VDX7ProcessorTests.cpp`: 13 elutasított pár három kontextusban:
első ROM előtti dirty edit, firmware nélküli pending szintetikus RAM és teljes
factory catalog, illetve privát v1.8-mal betöltött motor. Minden reject után
ellenőrzi a MIDI/catalog/dirty/pending generációt, mono-policyt, firmware-
readiness/routingot, mind a 148 hostparaméter aktuális értékét és a teljes
mentett binary payloadot. Ezt source-free újranyitási kontroll követi.
Két pozitív kontroll bizonyítja, hogy nem minden restore lett letiltva.
Az XML-ben látható szöveges értékeket teszteljük; az XML előtti var-típus
visszaállítását nem állítjuk. A standalone codec szigorú var-tesztei megmaradtak.

Public CTest: `vdx7_sound_mode_admission`, ROM-free. Privát külön regisztráció:
`vdx7_sound_mode_admission_integration`, v1.8 fixture-függőséggel. A sanitizer
workflow és a regisztrációs leltár is tartalmazza az új tesztet; nyilvános
Actions nem kap ROM-ot. Clean renderer integrációjakor a valid Clean rejection
helyett valódi pozitív recall/lifecycle teszt kell; a hibás schema reject marad.

## Eredmények

Windows x64, MSVC 19.44, Release, külön meglévő JUCE 9.0.3 buildkönyvtár.
Tényleges tiszta JUCE checkout: `be29c81492b6151c8ea8d14c840e1311963b3a83`.
Retromulator pin változatlan: `d5473776a0449d60a997b91bdc888598a33265ac`.

| Ellenőrzés | Státusz | Bizonyíték / korlát |
|---|---|---|
| Új reprodukáló teszt, javítás előtt | FAIL | Első partial pár megváltoztatja a restore-generációt; pozitív kontrollok PASS |
| `vdx7_ci_checks`, VST3, Standalone build | PASS | Csak dev-build; nincs telepítés vagy audioeszköz/DAW indítás |
| Teljes regisztrált ROM-free CTest | PASS | 21/21, benne actual processor-admission |
| Célzott privát original v1.8 CTest | PASS | 8/8: profile, admission, pre-ROM recall, imported recall, state transitions, controller reset, state identity, pending content identity |
| Python unittest | PASS / SKIP | 94 futott: 93 PASS, 1 Windows symlink-jogosultsági SKIP, 0 FAIL/ERROR |
| CTest-leltár | PASS | Tényleges local JSON-leltár; checker self-test pozitív és 9 negatív kontroll; configure-integration a Python-körben |
| Classic kontroll-összevetés | PASS | 30 rate/buffer/mono eset, 5 760 512 float érték, 0 eltérés; state/peak/RMS/silence metrikák azonosak |
| Új final-head Windows/macOS/sanitizer Actions és review | NOT RUN | PR létrehozása után külön ellenőrizendő; korábbi #170 zöld státusza nem e kód eredménye |
| Helyi macOS / ASan / UBSan | NOT RUN | Nem áll rendelkezésre e helyi Windows ellenőrzésben |
| Clean DSP / SETTINGS / teljes mode ownership | NOT RUN | Még nincs integrálva, nem a jelen védelem bizonyítéka |
| Valódi REAPER / hallásos / új release csomagelfogadás | NOT RUN | Nincs erre használt DAW vagy fagyasztott új release-csomag |

A Classic kontroll a #169 során készített privát JUCE 9.0.3 fingerprinthez
hasonlított friss kimenet; 44.1/48/96 kHz × 64/128/256/512/1024 × Native/Correct.
Saját szintetikus Init-voice, MIDI/sustain/reset/reload/pending és source-free
projektállapot. Ez nem hardware-hűség vagy hallásos „jobb hang” bizonyíték.
A privát audio-fingerprinthez 16 KB eredeti v1.8 firmware, a CTest-profilhoz
korábbi, helyileg validált bankos 48 KB tesztmásolat kellett. A kettő firmware-
prefixe azonos; a tesztmásolat nem eredeti factory-bank tanúsítás. Sem firmware,
sem bank, sem audio, sem személyes elérési út nem kerül e változtatással Gitbe.

## Következő lépés / megállási pont

Ez csak a D5 processor-admission első, fail-closed részlépése. Hátravan a
coherent desired-mode save/restore/pending/ROM-epoch ownership és a működő
native/SRC átmenet együttműködése, privát Clean/Classic mérések, majd SETTINGS.
Ezeket követi az új verzió és exact csomagfagyasztás, ugyanazon binárisok
valódi host/hallásos tesztje és külön publikálási döntés. Nincs új release,
tag, asset-csere, firmware-bővítés vagy rejtett Clean-elfogadás.
