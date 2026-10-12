# D5 firmware-reload sound-mode lifecycle

Dátum: 2026-10-12. Baseline main:
`de318935efae0da5b0fa182a2582526230cfed83` (#181 után).
Ág: `fix/sound-mode-rom-reload-lifecycle`.
PR: [#182](https://github.com/RobCZart82/VDX7-JUCE/pull/182); final-head CI és review még függőben, nincs beolvasztva.
Motor/teszt commit: `6b602612e54724eb0b32da1fdfef101f674a444a`;
Windows reference-tool commit: `8f2e6b74999205569690c5e870c49fffcb32a9e0`.
Irányadó: [egységes fejlesztési terv](../development/DEVELOPMENT_PLAN.md).

## Reprodukált belső D5 hiba

Az engine successful `loadRomImage()` után törölte a native/SRC historyt,
de megtartotta az interrupted live mode rampát. A teszt a valódi v1.8 engine
stabil módjából 32 hostmintányi ellenkező irányú live fade után tölti újra a
firmware-t. Javítás előtti guard-kontroll, production kód változtatása nélkül:

```text
FAIL: successful ROM reload cold-installs latest intent at unity without resetting EGS phase
```

Exit 1, az első privát reload-esetnél. Ez belső D5 renderer-lifecycle rés,
nem bizonyíték a szállított 1.0.1 Clean funkció hibájára: public Clean admission
és SETTINGS választás továbbra sincs engedélyezve. Nincs ROM-patch.

## Javítás

Successful boot és bank/program telepítés után a meglévő quiescent API rögzíti
a mai engine intentet. Régi native/SRC history törlődik; unity gain és az első
teljes új scan elején mód-install. A régi partial scan órái nem vesznek el,
de a kizárólag inaudible boundary előtti megszakított minta nem publikálható.
Boot nem rekonstruálja EGS-t: a valódi scan phase megmarad, nem állítjuk nullára.
Elutasított firmware ugyanúgy nem mutálja a működő motort/trajectoryt.
A mono-policy restore ugyanezt a reload-utat használja. Nincs új callback lock,
allokáció, fájl-I/O, paraméter, módmező, UI vagy firmware-támogatás.

## Tesztek és kontrollok

`VDX7NativeSoundModeTests.cpp`: 3 sample-rate × 2 kiinduló Native/Correct policy
× 2 direct reload/policy rebuild × 2 Classic/Clean kívánt mód = 24 privát pálya.
Valódi megszakított fade precondition; érvénytelen, de 16 KB méretű blank-ROM
snapshot-megőrzés; sikeres reload esetén phase/intent/unity és cold install;
32768 mintás saját carrier output véges és nem néma. Mindkét policy-rebuild
irány szerepel. Meglévő order/cancellation/overshoot/partition/peer/cold-control
tesztek megmaradtak; a friss direct EGS kontroll külön stabil adaptert kap,
hogy ne az ellenőrizendő cold-install API állítsa elő a referencia módját.

### Windows kontroll-tool külön hibája

`VDX7ClassicRenderReference.cpp` nyers floatot írt text stdout-ra. Windows
LF→CRLF konverzió miatt az előírt 2 359 296 bájtnál hosszabb, sérült stream
keletkezett; az első szigorú dekódolás `struct.error`-ral FAIL. `_O_BINARY`
kimenettel ismételt size/finite/nonzero/byte-identity kontroll PASS.
Ez a privát diagnosztikai tool javítása, nem plugin-audio módosítás.

Változatlan detached baseline engine és candidate engine, azonos MSVC Release,
core/voice static library és ugyanaz a reference stimulus program. Saját
hangszínek: 44.1/48/96 kHz × algorithm 0/15/31 × feedback 0/7; 18 friss boot
eset, összesen 589 824 float. Nyers bájtok teljesen azonosak, SHA256 mindkét
oldalon `62febcba4ea6ab5111035b47c939fa878613a02d82c87fb2f027cd3c72f79dc0`.
Ez **fresh steady Classic kontroll**, nem interrupted reload null-difference:
az utóbbi régi trajektóriáját/partial-outputját szándékosan nem tartjuk meg.
Nem teljes gyári preset/hardware/listening/quality tanúsítás.

## Eredményjegyzék

| Ellenőrzés | Státusz | Kör / korlát |
|---|---|---|
| Interrupted reload reprodukció, fix előtt | FAIL | Első privát eset, precondition PASS; exit 1 |
| Native/Clean privát v1.8 renderer regresszió | PASS | 24 új reload/rebuild pálya és meglévő 3 rate partition/ordering/cold/peer kontroll |
| Windows ci_checks, VST3, Standalone build | PASS | Új külön build; JUCE 9.0.3, MSVC 19.44; nincs telepítés |
| Teljes ROM-free CTest | PASS | 22/22 |
| Private actual-processor snapshot/restore kör | PASS | Captured parameters, invalid admission, owner, stale ROM completion; prepare/reset dispatch kontrollok |
| Fresh Classic main/candidate összevetés | PASS | 18 eset, 589824 float, 2359296 bájt, finite/nonzero és byte-identical |
| Python unittest | PASS / SKIP | 94 futott, 93 PASS / 1 Windows symlink SKIP, 0 FAIL/ERROR |
| Registration checker self-test | PASS | Pozitív és 9 negatív kontroll |
| További célzott ROM-os CTest | PASS | 9/9: profile, sound-mode admission, pre-ROM state, imported recall, state transitions, host-reset, direct reload, state ROM identity, pending content identity |
| Final-head Windows/macOS/sanitizer CI/review | NOT RUN | Push/PR után külön szükséges, nem örökli #180/#181 PASS-t |
| Helyi macOS/sanitizer / REAPER / listening | NOT RUN | Nincs indított DAW vagy helyi macOS/sanitizer kör |
| New release package acceptance | NOT RUN | Nincs új verzió, frozen package vagy publikálási döntés |

Tényleges dependency pins: JUCE `be29c81492b6151c8ea8d14c840e1311963b3a83`,
core `d5473776a0449d60a997b91bdc888598a33265ac`. A baseline külön detached,
tiszta checkout; a build/output a repositoryn kívül. Privát 16 KB original v1.8
firmware-t használtunk a natív teszthez és a Classic összevetéshez. A 9 CTest
profile-jához a korábbi helyileg validált bankos 48 KB tesztmásolat kellett,
azonos firmware-prefixszel; ez nem eredeti factory-bank tanúsítás.
Firmware, bank, audio vagy személyes út nem került Gitbe.
A CI továbbra ROM-free: a fenti valódi firmware-es bizonyíték helyi, nem CI-futás.

## Következő lépések és megállási kapu

Final-head platform/review után merge és külön main-ellenőrzés. Majd maradék
release/reset/pending/mono lifecycle és reentráns restore regressziók; teljes
Clean reader/writer/admission csak együtt működő rendererrel/ownerrel és
pozitív state/lifecycle bizonyítékkal. Ezután SETTINGS, mérések, verzió/csomag
fagyasztás, ugyanazon binárisok valós host/hallásos elfogadása, külön publikálás.
Nincs tag/release/asset-írás, új firmware-támogatás vagy telepített plugin-módosítás.
