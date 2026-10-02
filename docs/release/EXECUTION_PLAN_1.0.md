# Consolidated execution plan — 1.0.1 corrective development

## Release publikálási terv — 1.0.1

Állapotellenőrzés: 2026-10-02. Ez a **következő munkák egyetlen irányadó
terve**; az alatta szereplő auditok és korábbi kiadási feladatsorok történeti
bizonyítékok, nem újra végrehajtandó párhuzamos tervek.

### Hol tartunk most?

- Újra ellenőrzött main: `5298373020374293302c3db806fbf44958046a02`, #116
  már beolvadt. Windows `37002235337` és macOS `37002235328` PASS ezen a mainen;
  a #116 végső ágának Windows/macOS/ASan-UBSan ellenőrzései is PASS.
  Az ellenőrzés időpontjában nincs nyitott PR vagy hibajegy.
- Következő jelölt: `b7fce0503059c8c2e3c61d6ccf5af33612739e32`, #118 után;
  post-merge Windows/macOS PASS. Az ebből készített pontos, 5 120 fájlos
  forráscsomagból friss offline függőségű build, 13/13 CTest, 77/77 Python és
  VST3 fordítás PASS; célzott privát-firmware bankteszt PASS. A nagyobb
  ROM-optimalizálás nem indul, a részletes hibajelzés tulajdonosi döntéssel
  a következő javítókiadásra halasztva. [Forrásbuild](../validation/VALIDATION_20261002_SOURCE_BUILD_101.md).
  Új, nem elfogadott natív csomagolás: `37051589142`, még folyamatban.
- A [36998278475 csomagolási próba](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36998278475)
  PASS: Windows EXE és kézi ZIP, macOS Universal PKG és kézi ZIP, platformonként
  13/13 ROM nélküli teszt, Windows telepítés/frissítés/eltávolítás, forrásellenőrzés
  és közös checksum/staging. Ez `prep`, nem elfogadott vagy publikált release.
- A rögzített, reprodukált javítások beolvadtak; a lent felsorolt auditokból
  nincs itt megjelölt, még implementálandó bizonyított runtime hiba. Ez **nem
  új teljes audit, és nem általános hibamentességi állítás**. Az eredeti érvénytelen
  kombinált ROM elutasítása tudatos kompatibilitási határ, nem letesztelt migráció.
- A független letöltött-csomag vizsgálat most részben lezárt: a GitHub API-val
  letöltött három artifact külső hash-e, hét belső hash, pontos négy/kétfájlos
  készlet, 5 117 fájlos forráschecker, 276 termékforrás Git-összevetése és Mac
  PKG/Manual payloadazonosság PASS. A korábbi letöltési akadály megszűnt.
  A b7fce05 jelölt forráscsomag offline buildje már PASS; a helyi Windows
  EXE-kibontás/telepítés és végső hostelfogadás továbbra sem történt meg.
  [Friss felülvizsgálat és határok](../validation/VALIDATION_20261002_RELEASE_REVIEW.md).
  Végső 1.0.1 hostelfogadás és publikálási jóváhagyás nincs rögzítve.
  `RELEASE_APPROVAL.json` üres; a publikált `v1.0.0` változatlan.

### Végrehajtási sorrend és lezárási feltételek

| Kapu | Hátralévő munka | Mikor kész? | REAPER kell? |
| --- | --- | --- | --- |
| R1 — dokumentáció és befagyasztás | #116 és main zöld, lezárt. A friss felülvizsgálati terv mentése után kiválasztani a végleges termékforrást A és csomagolót B, teljes SHA-val; nem szükséges új funkciókör. | A/B és a hozzájuk tartozó bizonyítékok rögzítve; nincs véletlenül kimaradt main-változás. | Nem |
| R2 — független csomagvizsgálat | A 6cc8cda próba letöltése, hash-, készlet-, eredet- és Mac payloadvizsgálata PASS; b7fce05 pontos forráscsomagjának offline buildje PASS. A végleges A/B bináris csomagján ismételni az ellenőrzést, a nyitott helyi/platform határokat lezárni vagy indokoltan halasztani; új A esetén érintett forrásellenőrzés is kell. | Az alábbi R2-lista megfelelő részei PASS; a négy végső binárishash ismert, a fennmaradó határok kifejezettek. | Nem |
| R3 — végleges forrás és regressziók | Ha A/B eltér a tesztelt jelölttől, a változás arányában új build/teszt/csomagolás. A végleges A privát-ROM tesztjeinek és forráscsomagjának megfeleltetése. | Pontos A/B-n Windows/macOS/érintett sanitizer és Python tesztek sikeresek; ROM-os eredmények és negatív kontrollok tényleges állapota dokumentált. | Nem |
| R4 — host- és kompatibilitási döntés | A végső binárisokra célzott hostteszt, vagy a tulajdonos külön, tételes döntése a nem futtatott vizsgálatok halasztásáról és a ROM-kockázatról. | Pontos binárishash, OS/host verzió és eredmények, illetve explicit DEFERRED kockázatlista; régi 1.0.0 eredmény nem 1.0.1 PASS. | A REAPER-specifikus részekhez igen; most nem indítjuk |
| R5 — elfogadott csomag előállítása | A/B jóváhagyása után külön C commitban jóváhagyási rekord; canonical main workflow elfogadott módban, majd új csomag- és hashvizsgálat. | Érvényes A/B/C kötés, minden workflow-kapu PASS, a ténylegesen publikálandó fájlok újra ellenőrizve. | Csak ha a bináris változott vagy R4 ezt előírja |
| R6 — végső kiadási dokumentáció és engedély | HU/EN release notes/README/kézikönyv, négy hash, pontos verzió és forráslink, halasztások/aláírási/ROM-figyelmeztetések; külön publikálási engedély kérése. | Tulajdonos a pontos verziót, SHA-kat, fájlokat és kockázatokat jóváhagyta; nem pusztán a fejlesztési tervet. | Nem |
| R7 — publikálás és utóellenőrzés | Csak engedély után külön tartós `v1.0.1-source` és `v1.0.1` termékkiadás, megfelelő tagek/forrásazonosság; publikus linkek és letöltések újraellenőrzése. | Forrás legkésőbb a binárisokkal elérhető; négy termékletöltés, helyes latest állapot, publikus fájlhash-ek PASS. | Nem |

### R2: mit kell még REAPER nélkül ellenőrizni?

Az alábbi lista a végső A/B csomagra vonatkozik. A `6cc8cda…` próba hash-,
inventory-, forrás- és Mac payloadellenőrzése már dokumentált PASS; nem kell
úgy kezelni, mintha a letöltés továbbra is akadály lenne. A main azóta csak
dokumentációban változott: `Source/`, `Resources/`, `Tests/`, `scripts/`, CMake,
installer és workflow fájlok byte-ra változatlanok a próba forrásához képest.
Ez a regressziós bizonyíték újrafelhasználását indokolja, nem jelenti azt, hogy
a régi forrás ZIP a friss main dokumentációját is tartalmazza.

1. Az outer Actions ZIP-digestet a GitHub artifact digesttel összevetni. Ez nem
   helyettesíti az inner EXE/PKG/Manual ZIP hash-eket; mind a hét belső ellenőrzési
   fájlt a `SHA256SUMS.txt` alapján vizsgálni, és a négy publikálandó hash-t kiírni.
2. A négy felhasználói letöltésben csak a kiválasztott VST3 legyen; teljes bundle,
   Windows x64, macOS arm64+x86_64, helyes `1.0.1` címke/azonosság, megfelelő
   telepítési helyek, megőrzött plug-in/paraméterazonosságok és licenc-/notice-fájlok.
3. A két `BUILD-INFO` A/B/workflow és verzió/fordító/SDK/dependency adatait
   ellenőrizni; `prep` ne legyen átnevezéssel elfogadottá alakítva. Yamaha ROM,
   titok, cache vagy helyi személyes útvonal nem kerülhet a csomagokba.
4. A megfelelő corresponding-source ZIP-et a mellékelt önálló checkerrel
   ellenőrizni; forrás/dependency pinek, minimumtartalom és licencek megfeleljenek
   A/B-nek. Rögzíteni egy hálózat nélküli build/tesztet a végleges forráscsomagból,
   vagy ennek külön, indokolt tulajdonosi halasztását; régi ZIP tesztje nem elég.
5. A `Four-Downloads` pontosan négy fájlos, a `Source-Downloads` forrás ZIP +
   checksum kétfájlos készlet legyen. Letöltési akadály esetén kérni a konkrét
   artifact ZIP átadását; nem kell hozzá REAPER vagy a telepített plug-in cseréje.

### R3/R4: milyen tesztek és esetleges javítások maradtak?

- A befagyasztott forrás privát, jogszerű ROM-mal végzett célzott/full tesztjében:
  no-ROM mentés/első ROM-betöltés, függő projekt és későbbi szerkesztések,
  host reset, CC120/CC121 és telített/sűrű MIDI, sérült bank elutasításakor
  állapotmegőrzés. A már rögzített 43/43 valid-kontroll eredményt csak azonos
  releváns kód/konfiguráció bizonyításával lehet felhasználni, nem automatikusan
  új bináris elfogadásaként. Az eredeti sérült fixture-t változatlanul megtartani;
  szerkesztett tesztmásolat nem minősített ROM vagy migrációs megoldás.
- Javasolt végső REAPER smoke: telepítés utáni felismerés és verzió, jogszerű ROM
  betöltése, megszólalás; tartott hang+sustain mellett Stop/reset majd új hang;
  Pitch/Mod és hostautomatizáció; USER import/export; projekt mentés–bezárás–újra-
  megnyitás, matching ROM, hiányzó/eltérő ROM-ból helyreállítás és függő szerkesztés.
  A rossz kombinált ROM elutasítását és a korábbi állapot megőrzését is ellenőrizni.
- További, eddig nem elfogadott végső hostmátrix: offline render vs realtime,
  44.1/48/96 kHz és pl. 64/256/1024 buffer, több példány, sűrű MIDI/kontenció,
  GUI/Settings/About/HiDPI, fizikai Intel Mac és nem tesztelt OS/host változatok.
  Ezek nem bizonyított hibák. Mindegyikről PASS/FAIL/NOT RUN és, ha vállaltan
  kihagyjuk, tulajdonosi DEFERRED döntés kell; nem végtelen új audit a cél.
- REAPER az általános csomagoláshoz, checksumhoz, forrás- és CTest-vizsgálathoz
  nem szükséges. A hosttesztet a tulajdonos is elvégezheti; más VST3 host eredménye
  annak a hostnak bizonyíték, nem REAPER PASS. Helyi telepítés/REAPER-indítás előtt
  külön engedély és mentés szükséges, a mostani terv ezt nem végzi el.
- Ha új FAIL valódi hiba: pontos A/OS/host/hash és reprodukáló kontroll,
  külön javítóág + regressziós teszt, célzott javítás, minden szükséges zöld PR-kapu,
  felülvizsgálat, beolvasztás, új jelölt és érintett kapuk újrafuttatása. Ne
  fagyasszunk be ismert adatvesztési/állapot- vagy csomagintegritási hibát pusztán
  arra hivatkozva, hogy majd egy következő kiadás javítja.

### ROM ellenőrzés finomítása és optimalizálása

Tulajdonosi tervjóváhagyás: 2026-10-02. Ez az R3 része, nem külön roadmap,
és nem jelenti az alábbi kódmódosítások elkészültét vagy a kiadás elfogadását.
A cél jobb diagnosztika és mérhető egyszerűsítés, a szigorúság csökkentése nélkül.

Végleges diszpozíció az 1.0.1-hez, tulajdonosi válasz 2026-10-02:
**DEFERRED a következő javítókiadásra** a részletesebb ROM-hibajelzés;
nagyobb optimalizálás továbbra is mérhető előnyhöz kötött. Az alábbi sorrend
a későbbi munkát vezeti, nem új 1.0.1 blokkoló. A jelenlegi szigorú ellenőrzés
és állapotvédelem változatlan marad. Ez nem a végső binárisok/hosttesztek
elfogadása, és nem külön publikálási engedély.

1. **Első lépés a következő javítókiadásban:** közös ellenőrzőből részletes
   eredmény: hibakategória, bank/hangszín, mező vagy bájtpozíció, tényleges érték
   és megengedett tartomány. A felhasználói üzenet egyértelműen különböztesse
   meg a méret-/olvasási hibát, hibás kombinált képet és kihagyott opcionális
   bankot. Érvénytelen méretnél/pointernél ne olvasson mezőértéket; teljes ROM
   adatot vagy személyes útvonalat ne írjon naplóba.
2. **Mérés után választható optimalizálás:** vizsgálni a külön hétbites és
   szemantikai bejárás, valamint a processor/engine ismételt ellenőrzésének
   költségét. Mérni érvényes bankot, korai/késői hibát, külön companiont és
   kombinált képet, továbbá a zárolás idejét. Mérés nélkül nincs bizonyított
   lassulás vagy ígért gyorsulás; ez önmagában nem kiadásblokkoló runtime hiba.
3. **Csak igazolt előnynél átalakítás:** az ellenőrzést lehetőleg a motor
   zárolása és állapotmódosítása előtt végezni, közös szabályrendszerrel.
   Ismétlés csak akkor hagyható el, ha a pontos, változatlanul átadott adatok
   ellenőrzöttsége garantált; a motor közvetlen betöltési útja nem válhat
   védtelenné. A bonyolultabb struktúra nem indokolt pusztán egy bejárás miatt.
4. **Elfogadási feltételek:** a korábbi és új ellenőrzés ugyanazokat az
   adatokat fogadja el/utasítsa el. Regressziók: minden bankhatár, hétbites
   és szemantikai hibák, pontos diagnosztika, teljes állapotmegőrzés sikertelen
   betöltéskor, függő projekt/szerkesztés megőrzése, közvetlen motorhívás és
   érvényes kontroll. Új PR és szükséges zöld platform/sanitizer kapuk után
   változó termékforrás esetén új végső jelölt és érintett csomagellenőrzés kell.

Változatlan szabály: hibás kombinált ROM teljes elutasítása, a korábbi hangszer
megőrzésével; hibás opcionális külön bank kihagyása látható figyelmeztetéssel.
Nincs automatikus 99-re korlátozás, néma javítás vagy kombinált-ROM fallback.
Az elutasított régi ROM eredetét/kompatibilitását ez az optimalizálás nem dönti
el, nem teszi minősítetté a módosított tesztmásolatot, és nem garantál régi
projekt-visszaállítást. Nagyobb optimalizálás mérhető előny hiányában halasztható;
a diagnosztikai finomítás 1.0.1 előtti diszpozíciója fent rögzítve: DEFERRED.

### R5–R7: publikálás előtti és publikálási lépések

Az [A/B/C jóváhagyási eljárás](RELEASE_APPROVAL.md) kötelező. A mostani
`6cc8cda…` csak jelölt, nem előre jóváhagyott végső A/B. Ha az új dokumentáció
is a végleges forráscsomag része lesz, új A-t kell kiválasztani és annak pontos
csomagját ellenőrizni. C külön jóváhagyási commit, B nem lehet C.
Elfogadott módban új build készültével a régi prep bináris hash-jeit nem szabad
átvinni; ha byteszinten változik egy fájl, az érintett bináris/hostelfogadást
újra el kell végezni vagy külön vállalt halasztásként rögzíteni.

A [négy-letöltés politika](PUBLIC_DOWNLOADS.md) szerint a termékkiadás:
Windows `Setup.exe` + `Manual.zip`, macOS Universal `.pkg` + `Manual.zip`.
Nem kerül fel ötödik termékassetként a teljes Actions ZIP, BUILD-INFO, forrás
vagy checksumfájl. A négy SHA-256 a leírásban szerepel. A kiválasztott külön
`v1.0.1-source` release a megfelelő teljes forrást és checksumját tartalmazza,
nem latest; a termék `v1.0.1` erre közvetlenül hivatkozik és csak engedély után
lehet latest. Tartós forráslink nélkül nincs kész publikálás.

Az [új HU/EN release-note tervezet](RELEASE_NOTES_1.0.1_DRAFT_HU_EN.md) és a
README/kézikönyv végső szövege őrizze meg: unsigned Windows, macOS technikai
ad-hoc aláírás, nincs Developer ID/notarizáció; rendszerbiztonság kikapcsolása
nem ajánlható. Aláírási fiók/tanúsítvány beszerzése a tulajdonos döntése szerint
nem ennek a kiadásnak a feladata. A ROM és régi projektek kockázata, minden
halasztás, a forrásazonosság és a valódi támogatási korlátok legyenek láthatók.

Publikálás után frissen letölteni és hash-elni a nyilvános négy termékfájlt és
a külön forrást; linkek, nevek, latest és forráschecker PASS. Jegyzőkönyvben
rögzíteni a release URL-t, időt, A/B/C-t, tesztelt binárishash-eket és kockázatokat.
Meglévő `v1.0.0` tag/asset módosítása nincs engedélyezve.

### Nem kiadásblokkoló karbantartás

Node 20 action-runtime figyelmeztetés és ubuntu-latest közelgő image-váltás,
tesztcél-elnevezés és általános warning/refaktor igények: külön, tesztelt
karbantartási munka. Nem új bizonyított szoftverhiba és nem indok az A/B
ellenőrzés nélküli megváltoztatására. Az érintett környezetet/provenance-t
rögzíteni; ha tényleges FAIL vagy támogatási akadály lesz, visszaemelni R3-ba.

## Earlier corrective checkpoint — 2026-10-02

The current corrective round starts from main
`928c8b7dbea9816df8e5cc3700354859aaa4d8d1`, after PR #113.
The owner approved fixing the three reproduced findings below on a separate
branch, `codex/factory-bank-source-integrity`. PR #114 is MERGED at
`9d53e9ee578ae614f76025d36ef7a258659de3f6`. Final head
`c370aec016dafcc3f33f3e71077c2d0c35c1ecab` passed Windows `36990444825`,
macOS `36990444757` and ASan/UBSan `36990444781`; no unresolved review threads
or merge conflicts remained. Owner decision (2026-10-02): reject
the entire invalid combined ROM, preserving the previous state. No automatic
factory-data clamping or firmware-only fallback for a bad combined image.
Post-merge Windows `36992554669` and macOS `36992554670` both completed PASS
on that exact main SHA. The owner now requests completion of
the publishable 1.0.1 product and release preparation. This is not approval of
an exact product/tooling pair or authorization to publish a tag/release.
Do not treat this as acceptance of a release candidate.

| ID | Reproduced failure | Current correction / acceptance |
| --- | --- | --- |
| BH-20261002-01 | P2: invalid optional/combined factory voice data accepted into engine state | MERGED #114 with owner-approved strict rejection. Focused engine/processor state-preservation/export/reopen PASS; full valid positive-control suite 43/43 PASS (13 ROM-free + 30 local-ROM). Original-fixture 28 loading failures remain recorded as rejection of four out-of-range later voices, not compatibility passes. Original ROM unchanged; no runtime normalization. Final-head platform/sanitizer gates PASS; exact release acceptance separate. |
| BH-20261002-02 | P2: verifier/staging accepts a regenerated inventory missing mandatory LICENSE | MERGED #114: shared mandatory source/notice minimum; bundled checker/readme required for 1.0.1, historical 1.0.0 compatibility retained. Missing-file negatives and full diagnostic archive checks PASS. This is a minimum-content check, not publisher authentication or a complete independent Git comparison. |
| BH-20261002-03 | P3: checksum output aliases an input and destroys the asset | MERGED #114: rejects identical/resolved paths, hardlinks and symbolic-link outputs before writing. Local path/hardlink regressions PASS; local Windows symbolic-link capability test NOT RUN. Exact-head macOS and sanitizer gates PASS. |

[Round validation](../validation/VALIDATION_20261002_BANK_SOURCE_INTEGRITY.md)
records actual executed checks and remaining gates. Prior ledger entries below
retain their historical baselines and do not imply final 1.0.1 acceptance.

1.0.0 is published at product `d79ed5214d82caf70e3941e5a620bab137d3f9ca`;
release tools/main reviewed at `3fa8c2e00ad4dcd1860551cf3596ee4ad29de789`.
Publication time: 2026-09-29 23:49:49 UTC. Preserve that tag and its assets.
This development round starts from main
`d4f589764284e72a2d532a6055ac8d918cb53d9c`, after PR #112. Its final-head
Windows/macOS/ASan-UBSan PR gates passed; post-merge Windows `36908721471`
and macOS `36908721498` also completed successfully at that exact main commit.
Exact-candidate packaging remains separate; this does not accept a 1.0.1 binary.
The owner authorized corrective development and merging green, verified PRs
into main for 1.0.1, and now identifies 1.0.1 publication as the goal.
No exact product/tooling pair or final binary assets have yet been approved.
Earlier checkpoints below describe their own historical states, not current
release availability or a continuing absence of known defects.

This is the single active work ledger; do not create competing roadmaps.

| ID | Priority / evidence | Implementation and next acceptance |
| --- | --- | --- |
| AUDIT-20260930-A1/A2 | P1, reproduced pending-project edit loss and wrong-engine capture | MERGED [#104](https://github.com/RobCZart82/VDX7-JUCE/pull/104); required platform/sanitizer checks PASS and local 36/36 CTests PASS. Rejects unsupported operations, preserves edits and prioritizes recovery. [Boundary validation](../validation/VALIDATION_20260930_PENDING_BOUNDARY.md). Exact-1.0.1 acceptance remains separate. |
| AUDIT-20260930-A3 | P1 packaging, published ZIP fails its bundled verifier | MERGED [#105](https://github.com/RobCZart82/VDX7-JUCE/pull/105); all PR checks PASS. Ten packaging tests, real extracted checker and offline Windows build/11 CTests PASS. [Verification record](../validation/VALIDATION_20260930_SOURCE_VERIFIER.md). Final-release acceptance remains open; existing 1.0.0 assets unchanged. |
| AUDIT-20260930-A4 | P2 provenance guard gap, not unauthorized publication | MERGED [#109](https://github.com/RobCZart82/VDX7-JUCE/pull/109): exact reviewed source/packager pair, committed approval snapshot and canonical main workflow context required before accepted packaging. Final-head Windows/macOS/ASan-UBSan and post-merge Windows/macOS checks PASS. Null approval fails closed; preparation and legacy integrity verification remain available. [Approval procedure](RELEASE_APPROVAL.md), [regression evidence](../validation/VALIDATION_20261001_RELEASE_APPROVAL.md). No release is approved by this change. |
| AUDIT-20260930-A5 | P2 installer layout; no reproduced host scan failure | ASSESSED: retain AppId and current uninstall directory for 1.0.1. Guarded hosted-Windows checksum-pinned 1.0.0 -> candidate -> uninstall, installed-payload hashes, one AppId registration and synthetic user-file preservation PASS in run `36998278475`. [Native evidence](../validation/VALIDATION_20261002_PACKAGE_PREPARATION.md), [mechanism](../validation/VALIDATION_20261001_INSTALLER_UPGRADE_SOURCE.md). No host scan defect was reproduced or claimed fixed; exact local-host acceptance remains separate. |
| AUDIT-20260930-A6 | P2 toolchain provenance | MERGED [#110](https://github.com/RobCZart82/VDX7-JUCE/pull/110); Windows/macOS/ASan-UBSan PR checks PASS. Compiler-engine verification correction MERGED [#115](https://github.com/RobCZart82/VDX7-JUCE/pull/115), all three final-head checks and post-merge Windows/macOS PASS. Pins Inno 6.7.1 and records configured compiler, SDK, CMake, runner image and verified dependency commits in hashed BUILD-INFO assets. [Mechanism](BUILD_TOOLCHAIN_PROVENANCE.md), [correction](../validation/VALIDATION_20261002_INNO_VERSION.md). Run `36993835698` failed at numeric PE metadata; fresh unaccepted run `36998278475` tests the corrected mechanism. Final candidate provenance review remains required; observed versions do not imply bit-identical installers or an immutable runner. |
| AUDIT-20260930-A7 | P2 stale release evidence/docs | PARTIAL: plan, README and HU/EN guides distinguish published 1.0.0 from corrective development; final #104/#105 validation recorded. Owner's future [four-download policy](PUBLIC_DOWNLOADS.md) is MERGED [#111](https://github.com/RobCZart82/VDX7-JUCE/pull/111); final-head Windows/macOS/ASan-UBSan and post-merge Windows/macOS checks PASS. Actual candidate payload staging, full accepted binary asset reconciliation and durable matching-source access remain OPEN. |
| AUDIT-20260930-A8 | P3 test-only C4805 | IMPLEMENTED: explicitly convert the boolean carry flag before integer bitwise packing; fresh Windows rebuild without C4805 and focused trace regression PASS. |
| AUDIT-20260930-A9 | P2 missing sanitizer coverage, not a DSP defect | IMPLEMENTED: resampler added to both instrumented build targets and CTest filter. Actual ASan/UBSan run is a required PR gate; Python packaging remains separate. |
| AUDIT-20260930-A10 | P3 descriptive prototype metadata | IMPLEMENTED: description says instrument; bundle ID, plugin codes and parameter identity unchanged. Platform builds are required PR gates. |

Work order: A1/A2 -> A3 -> reproduced deep-audit runtime fixes below
-> A4/A6/A7 -> proportionate A5/A8/A9/A10 work
-> frozen 1.0.1 build/acceptance. Each PR carries its validation and updates
this ledger. MERGED is not VERIFIED on the final candidate. Never convert
NOT RUN / DEFERRED to PASS without execution.

Preserve the earlier fixes, all 148 parameter IDs/order, state compatibility,
Notes 12–120, Native default, keyboard PITCH value retention and approved GUI.
No ROM, secrets, caches or installed binaries in GitHub. No REAPER launch or
installed-plugin replacement during non-host development. Broader host/rate/
buffer/offline/instance/GUI/Intel acceptance remains explicitly deferred;
tail=0, try-lock silence and bounded queues are characterization items, not
new proven defects. A targeted final-binary host check needs separate consent.

Final gate: exact source and packager SHAs, Windows/macOS build and tests,
private local-ROM regressions, self-verifying source archive, asset hashes,
payload/licenses, installer upgrade smoke and explicit deferred-risk record.
Present these before separately requesting 1.0.1 publication authorization.

### Next release steps — 1.0.1

Historical order after #114 (now superseded by R1–R7 above): close post-merge
main gates and run unaccepted preparation. The run below completed. Never treat a
running Action, a constructed private fixture, or an earlier source-only
archive as acceptance of the final installable binaries. Keep the approval
JSON null until reviewing actual candidate evidence.

Preparation run [36993835698](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36993835698)
has been dispatched from main for product source
`9d53e9ee578ae614f76025d36ef7a258659de3f6`, with
`accepted_for_publication=false`. The local authorization preflight passed
with source/tooling/approval-workflow identities at that SHA and
`release_accepted=false`; typed local context is not publisher authentication.
That run completed with Windows packaging FAIL: the ISCC executable's numeric
ProductVersion fields yielded `0.0.0`, despite Chocolatey reporting 6.7.1.
macOS packaging PASS; combined assembly and Windows installer/upgrade checks
NOT RUN. This is a packaging gate defect, not a reproduced DSP defect.
The correction uses a no-output stdin probe compile to observe the actual
compiler engine version, still requiring exactly 6.7.1 and successful exit.
Local mocked probe and workflow regression tests do not certify native ISCC.
A fresh non-publishing preparation run using the merged correction is required;
rerunning the old immutable workflow cannot test the fix. Approval remains null.

PR #115 is now MERGED as `6cc8cda30e9e66d3ab97699bcb9d2014a78b8b14`.
Final-head Windows/macOS/ASan-UBSan checks PASS; post-merge main Windows
`36996855918` and macOS `36996855952` PASS. Fresh non-publishing preparation
[36998278475](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36998278475)
uses that exact product/tooling/workflow SHA, acceptance disabled; authorization,
Windows/macOS packaging and assembly all PASS (9m 37s). Windows native Inno
6.7.1 verification, clean install/uninstall and checksum-pinned 1.0.0 -> 1.0.1
upgrade/uninstall/user-file preservation PASS; both platforms' ROM-free CTests
13/13 PASS. Seven-asset checksums, four-download staging and source self-check
(5117 files) PASS on the hosted runners. Full local packaging regressions:
77 discovered, 76 PASS, one symlink-capability SKIP. Native result must be
reviewed independently before publication: the local artifact download timed
out, so independent downloaded-payload/hash reconciliation is NOT RUN. No host
acceptance or publication approval is inferred from the green preparation.
See [exact-candidate evidence](../validation/VALIDATION_20261002_PACKAGE_PREPARATION.md).
The [1.0.1 HU/EN release-note draft](RELEASE_NOTES_1.0.1_DRAFT_HU_EN.md)
records compatibility/signing warnings and the still-open publication gates.

The strict combined-bank rejection is a deliberate compatibility boundary:
previously accepted out-of-range factory images cannot be loaded by 1.0.1.
An old project bound to such an image may remain pending rather than silently
installing different ROM content. The valid test copy is not a user migration
or matching-ROM solution. Document this risk prominently and do not claim old
1.0.0 REAPER reports certify the new packages or all legacy-ROM projects.

Historical checkpoint order; for remaining work use R1–R7 above:

1. DONE: coordinated project/installer/workflow version update merged in #112.
   Development remains visibly `1.0.1-dev`; stable preparation is `1.0.1`,
   not accepted. New packaging rejects mismatched source/installer versions
   before platform work; historical 1.0.0 verification remains compatible.
2. After final-head PR gates and post-merge main Actions pass, freeze the exact
   candidate and run the non-publishing stable installer workflow. Inspect
   actual compiler/SDK provenance, both native installers, source archive,
   combined checksums and the separate four-file staging artifact.
3. Close A5 upgrade/uninstall evidence, review durable matching-source access
   under the four-download policy and record exact-candidate acceptance or
   explicit deferred risks. Do not reuse 1.0.0 host passes as 1.0.1 evidence.
4. Freeze the reviewed source/tooling pair, commit approval separately, and
   regenerate accepted metadata. Reconcile final hashes and publication notes
   before publishing a new v1.0.1 tag/release. Preserve v1.0.0 unchanged.

Version-update validation: [1.0.1 candidate preparation](../validation/VALIDATION_20261001_VERSION_101.md).

PR #112 merged as `d4f589764284e72a2d532a6055ac8d918cb53d9c`.
Final head `15f11830e432018809630d2d9cce6085c7103ef6` passed Windows
`36907192768`, macOS `36907192756` and ASan/UBSan `36907192754`.
Its fresh local development harness passed 43/43 executable CTests with the
private ROM (44 registered; interactive save-dialog case NOT RUN), plus
58/58 Python tests. This is source/harness evidence, not accepted stable binaries.
New source staging and installer-upgrade tooling have their own review/gates.

Owner publication-layout decision (2026-10-01): exactly four manually uploaded
user downloads — Windows x64 EXE installer + Manual Install ZIP, macOS Universal
PKG installer + Manual Install ZIP. Keep complete validation/source evidence
separate; include hashes and reviewed durable source links in release notes.
Use the checked four-file staging allowlist, not a wildcard upload of the
validation artifact. [Policy](PUBLIC_DOWNLOADS.md). This neither publishes
1.0.1 nor changes existing v1.0.0 assets or the separate acceptance gates.

Owner source-delivery decision (2026-10-01): separate `v1.0.1-source` GitHub
release in this repository, not marked latest, containing matching complete
source ZIP plus checksum manifest. Link it from the main `v1.0.1` release,
which remains latest with exactly four user downloads. Do not put source in
the Manual ZIPs. Mechanism selected; final tuple/hash/source review stays OPEN.

## Deep audit corrective round 2026-09-30

The fresh audit baseline is main `3b943a893ca95c9b31ca43ee5a10cc05cd9ed7d0`
after PR #107. Four reproduced P2 findings are recorded in the
[Hungarian deep audit](../validation/VALIDATION_20260930_DEEP_AUDIT_HU.md).
They are new boundary cases, not a reassignment of the earlier A1/A2 IDs.
The owner authorizes implementation and merge after successful required PR
checks; publishing another release remains a separate decision.

| ID | Reproduced failure | Required acceptance |
| --- | --- | --- |
| DEEP-20260930-S1 | First ROM installation overlapping a no-ROM save loses accepted voice/operator edits | Detached deferred-edit snapshot; deterministic overlapping save/load and reopen; preserve both operator mask halves and the saved generation |
| DEEP-20260930-S2 | Host edits during pending project ROM initialization are discarded | Retain later edits through firmware/RAM installation and final publication; with/without RAM, before/during/after installation and delayed listener routing |
| DEEP-20260930-M1 | Direct CC120 does not advance/anchor the deferred timeline | Preserve next-block Note On/Off/CC, exact reset sample offset, empty tail, repeated reset and partition equivalence; ROM-free boundary cases under ASan/UBSan |
| DEEP-20260930-M2 | CC121 loses non-wheel controller reset messages in a full application FIFO | Durable reset zeros, observed sustain OFF edge before later pedal ON, all six sources, full/near-full controls, held-note and persistent-setting preservation; post-reset host notes and controllers retained in order on the bounded timeline; gate remains closed through wheel firmware completion and fresh fallback pacing after late input |

All four fixes are MERGED in [#108](https://github.com/RobCZart82/VDX7-JUCE/pull/108).
The round was locally validated with a fresh Release build,
43/43 executable CTests (44 registered; desktop `vdx7_processor` NOT RUN),
12/12 Python tests and the inventory contract/seven negative controls PASS.
The instrumented CC121 runner passes 24 cases; ASan/UBSan deferred-MIDI and
resampling component tests pass 2/2. Independent cross-review is complete.
Details and scope limits: [fix validation](../validation/VALIDATION_20260930_DEEP_AUDIT_FIXES.md).
Windows/macOS/ASan-UBSan PR checks passed on final head
`2d30e59d56ab2a7ccf8d4b0166551a79db1a3038` (runs `36780563928`,
`36780563512`, `36780563385`); review conversations were resolved. Post-merge
Windows `36781795966` and macOS `36781795912` passed on
`ff1df3619824cc8d1642869216b19280ad88e8d0`. These are development checks,
not acceptance of a new release candidate. The approved GUI, 148 parameter
identities, project format, existing stable tag and release assets remain
unchanged. Exact-1.0.1 binary/host acceptance remains separate.

## Historical main checkpoint — stable installer acceptance (2026-09-29)

Current main is `d79ed5214d82caf70e3941e5a620bab137d3f9ca`, merge of PR #101. PR checks and post-merge Windows/macOS CI passed. The non-publishing stable VST3 installer workflow [36621909919](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36621909919) passed on this exact SHA. Both platform builds and ROM-free CTests passed; Windows installer install/uninstall smoke test, macOS package payload checks, and combined SHA-256 manifest verification also passed. It uploaded `VDX7-1.0.0-Release-Assets-d79ed5214d82caf70e3941e5a620bab137d3f9ca` (ID `11059321610`, outer artifact SHA-256 `65d3ac7195a00f7d3040816cbbd257ea247f0cb65d487c6823cc8c02675712a3`, expires 2026-12-28).

The owner reports successful Windows installer and REAPER-plugin use. On macOS, the owner reports the Universal `.pkg` installed on a Mac mini M1 / macOS Tahoe 26.7 after the per-app Gatekeeper “Open Anyway” flow, and that the installed VST3 opened and worked/sounded good in REAPER. These are owner-reported real-machine acceptance; exact installed package hashes, REAPER versions for these trials, and full test-matrix details were not supplied. Full record: [stable installer acceptance](../validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md).

Independent review of run 36621909919 is complete: the downloaded outer artifact digest matched GitHub, all seven embedded asset hashes passed, the source archive's 5,082 files verified against its manifest and exact source/dependency pins, and both VST3 payloads were inspected. No firmware, secrets or local paths were found. The evidence and all inner asset hashes are in [stable installer acceptance](../validation/VALIDATION_20260929_STABLE_INSTALLER_ACCEPTANCE.md). The artifact is still validation-only: BUILD-INFO says not accepted/approved for publication and the source manifest records `release_accepted: false`. Final stable metadata must be regenerated and the combined manifest rechecked; preserve the tested binaries if doing metadata-only repackaging. The bilingual notes/deferred-test disclosures and final release review remain. Preserve the unsigned Windows and macOS Gatekeeper/notarization warnings; do not recommend disabling system-wide protections. AU is outside 1.0.0 distribution.

## Test-phase decision — 2026-09-28

The owner elects to wind down open-ended exploratory testing: no reproducible
defect has been found, and any subsequently confirmed issue is expected to be
fixed in a post-1.0.0 release. This closes the exploratory bug-search phase,
not every possible test. Remaining unrun host/audio/GUI matrix items must stay
identified as **not run/deferred**, never be relabeled as passes. Before
publication, review the remaining unchecked release gates and explicitly
accept or defer them; this decision alone does not publish or authorize a
release. A serious data-loss, security, or release-blocking defect would still
justify stopping publication and issuing a corrective release as appropriate.

## Historical checkpoint — after PR #91

Current reviewed main: `37c83f378a81e4623993941fb7e10c4ab58c208c` (merge of
PR #91). PR #91 adds opt-in `rcN` display/artifact identity and focused
ROM-free validation; its Windows, macOS and ASan/UBSan checks passed. The
post-merge main runs also passed: Windows `36441316285` and macOS
`36441316112`, including builds, ROM-free tests, source-packaging checks,
local-ROM test-registration smoke and artifact upload. These are successful
development/main checks, **not** an exact-RC run or REAPER acceptance. The
exact-candidate workflow has not yet been dispatched, no RC SHA has been
accepted, and no stable tag/release has been published. At this checkpoint
there is no open PR. Keep owner reports separate from executed evidence.

Reviewed baseline: `817987b9337aca9d0e21d14427ac14fc74833505` (main after #90).
PR #90 merged the bilingual release-preparation documentation and recorded the
owner-reported Windows #247 VST3 installation hash. Its PR Windows/macOS and
ASan/UBSan checks passed. Post-merge main Windows run 36433962468 and macOS run
36433962462 both passed on `817987b`; the sanitizer check passed on the PR head.
No open PR was present at the next-work checkpoint. Owner report is installation
evidence, not new functional acceptance.
PR #88 checks passed; PR #89 source `d834f4f` passed Windows run 36410161563,
macOS run 36410161589 and ASan/UBSan run 36410161474. Post-merge main Windows
run 36411273600 and macOS run 36411273595 also passed. These are development
checks, not exact-RC acceptance. Main is re-fetched before each publication;
incoming changes are preserved. This is a dated review checkpoint, not a claim
that moving main will always remain at this SHA.
This plan combines the original 1.0 host/audio/release gates with the
useful findings F1–F19 and the test-system review. Planning is not release
authorization. Older roadmap narratives and validation notes remain historical
records unless explicitly updated here; they are not instructions to reopen
completed GUI work.

## Active work ledger — audit 2026-09-28

Use full IDs `AUDIT-20260928-N1` through `AUDIT-20260928-N8`: older roadmap
audits reused N1/N2/etc. for DIFFERENT findings. Source review is not runtime
reproduction, and an implemented fix is not exact-RC acceptance.

| ID suffix | Finding / evidence class | Next action and completion evidence | Environment | Status |
| --- | --- | --- | --- | --- |
| N1 | Reproduced edit loss while a saved project waits for a matching ROM and another ROM is loaded | Stopped processing, running callbacks and re-save/reopen regressions; preserve feedback/operator edits, leave incompatible engine unchanged, resume normal routing | Local v1.8 ROM, Windows processor harness; no REAPER | MERGED #87; failing baseline and 10/10 related checks PASS; see [validation](../validation/VALIDATION_20260928_PENDING_PROJECT_EDITS.md); exact-RC acceptance remains open |
| N2 | Storage accepts semantically invalid unoccupied USER slots that processor import rejects | CRC-valid invalid fields in occupied and empty slots; reject all malformed packed voices transactionally | ROM-free storage test | REPRODUCED then FIXED locally; included in follow-up 7/7 PASS |
| N3 | Project RAM restore lacked packed voice semantic validation | Reject malformed VMEM before mutation; loaded/deferred ROM and modern/legacy state matrix | Processor harness and local ROM | REPRODUCED then FIXED locally; included in follow-up 7/7 PASS; no crash claim |
| N4 | Shared 16 KB/48 KB fixture claim did not match full direct-engine suite | Full-suite requirement narrowed to combined 48 KB v1.8; all local tests use profile fixture; product 16 KB support unchanged | Local ROM tests | IMPLEMENTED; combined profile PASS, 16 KB negative control fails clearly as expected |
| N5 | CI registration smoke checked only one selected ROM test | Full names, labels, fixture edges, timeouts and failure policy; seven checker negative controls | Configuration-only CI | Full inventory checker and `vdx7_version_identity` merged in #91; expected inventory is 37 ROM-on / 11 ROM-off; PR and post-merge platform checks PASS |
| N6 | Keyboard pitch-wheel return policy differs from mouse release | Owner explicitly chose existing keyboard value retention on 2026-09-28; HU/EN guides clarify distinction | Component/UI policy review | ACCEPTED POLICY, not a defect; no input behavior change |
| N7 | Invalid companion warning hid pending-ROM identity mismatch | Reproduce both conditions; preserve both warnings and pending project recovery | Local ROM processor harness | REPRODUCED then FIXED locally; included in follow-up 7/7 PASS |
| N8 | Moving main described using stale SHA | Dated reviewed baseline and aligned current documents; historical evidence keeps original SHAs | Documentation review | Initial fix MERGED #87; this checkpoint tracks post-#89 main `fbea5ea` |

N2–N7 follow-up evidence, full-suite status and publication state:
[non-host hardening validation](../validation/VALIDATION_20260928_NONHOST_HARDENING.md).
Local FIXED does not imply merged, remote CI PASS or accepted final RC.
Publication checkpoint: the 24-file follow-up was published as #88 with explicit
owner approval and is now merged. All three PR checks PASS. Its N2/N3/N7 fixes
and N4/N5 test-contract changes are therefore merged implementation, not merely
local fixes; the earlier evidence rows retain the actual local reproduction scope.
New non-host round: [reproducible source packaging](SOURCE_PACKAGING_1.0.md)
and HU/EN candidate instructions implemented. Two actual source ZIPs matched;
extracted-source Windows offline build, 10/10 CTest and six packaging tests PASS.
Independent Steinberg validation: 47 PASS, 0 FAIL. About opening/rendering PASS
at the observed size; further UI testing stopped when Windows locked.
See [exact provenance and limitations](../validation/VALIDATION_20260928_SOURCE_PACKAGING.md).
Post-merge main verification on `fbea5ea`: ROM-free CTest 10/10 PASS, packaging
unit tests 6/6 PASS, 36-test local-ROM-on registration inventory and seven
negative controls PASS. A 5,073-file corresponding-source ZIP was generated
and manifest-verified (SHA-256 `19c5ab95e1af36c15b19194a37603557b1975bcd5247d72f94b284ff8db113cd`).
This exact merge-commit ZIP was not extracted/rebuilt in this check. The
At that historical checkpoint the exact-candidate workflow remained NOT RUN;
PR #91 has since added the workflow and tested build identity. It remains NOT
RUN on an exact candidate; no stable publication.

Owner-reported acceptance from the supplied handoff dated 2026-09-28: the owner
reports REAPER testing with no known issue and considers the product releasable.
The earlier dated records in section 3 retain the known scope: macOS REAPER
testing (macOS 26.7) and Windows 10 x64 using Actions build #219 from
`29ab5e350019e32f64650b068afcc7709b409607`. The macOS tested SHA, binary hashes,
REAPER application versions and complete rate/block/instance matrix were not
recorded. Keep this OWNER-REPORTED, not an assistant-run PASS or proof of every
platform/matrix cell. The owner accepts Windows distribution without
publisher signing and macOS distribution with ad-hoc signing only (no Developer
ID/notarization). Show the resulting OS security-warning risk clearly; checksums
prove integrity, not publisher identity. This does not itself authorize release
publication.

Owner scope (2026-09-28): do NOT launch the installed REAPER, replace an installed
plugin or modify host projects during this non-host preparation. Preserve the
owner-reported acceptance separately from exact-SHA/platform evidence. No stable
tag, release or asset publication is authorized by this development work; before
publication show the final version, source SHA and asset list and confirm the
separate publication authorization.

## Release-preparation work order

Do not reopen completed N1–N8 absent a reproducible new failure. The RC1
exploratory testing phase is closed by owner decision; deferred checks remain
visible in the checklist. The remaining work is packaging and release governance:

1. Finalize bilingual release notes and ensure the HU/EN installation guides,
   source package, license notices and signing warnings match the exact assets.
   Keep the current `1.0.0-dev` main downloads accurately labeled until stable
   assets exist.
2. Review the deferred acceptance items and their risk disclosures. Do not turn
   not-run checks into passes. Confirm the planned distribution scope (Windows
   x64 VST3 and macOS Universal VST3; AU/Standalone remain build targets only).
3. Once the release contents are ready, recheck protected `main`, freeze the
   final source SHA, and run the required Windows/macOS CI on that exact commit.
   PR #98 merged this workflow; it passed on main SHA
   `f7a1248b2cfff0b6fb159c189ca6a5b2cff766ec` and generated non-publishing
   Windows x64/macOS Universal artifacts. Independently inspect their embedded
   checksums, build identity, VST3s and matching source archives before deciding
   whether to freeze a release candidate. The existing candidate workflow remains `rcN`.
4. Produce stable `VDX7_RELEASE_BUILD=ON` artifacts and a matching corresponding-
   source archive; inspect both platform bundles and the archive, verify exact
   versions/dependency revisions/manifest/checksums, and record all hashes.
5. Present the final version, SHA, asset list, checksums and deferred-test
   disclosures for review. Only after separate explicit publication
   authorization create the stable tag/GitHub Release and then verify its links.
   Never rewrite an existing tag/release or bypass branch protection.

N6 remains an accepted owner decision, not a speculative fix. The supplied
handoff is useful acceptance/policy context, not a new test result or publishing
authorization. Record exact source, command, platform, fixture scope and status
in linked validation reports.

Additional findings from this round's runtime checks (not audit N numbers):

- `TEST-20260928-BANK-ORACLE`: processor integration compared an old exported
  bank with live RAM after intentional snapshot-test edits. Move the expected
  snapshot to export time; keep the original byte-exact round-trip assertion
  and add a negative control proving that the later edits changed the bank.
  IMPLEMENTED; final complete processor executable rerun PASS after the
  independent pitch-fader assertion correction below.
- `TEST-20260928-TIMEOUTS`: `vdx7_midi_range` exceeded its existing 60-second
  CTest limit and `vdx7_mono_corrected_processor` exceeded 120 seconds on
  Windows. Sequential baseline reruns reproduced both timeouts; unchanged
  executables completed in 292.44 s (MIDI, prior round) and 130.88 s (MONO,
  isolated rerun). Windows-only budgets are now 600/300 s without reducing
  scenarios/assertions. See the follow-up validation for the complete rerun;
  these are exhaustive emulation tests, not realtime wall-clock guarantees.
  Follow-up full run: 35/36 PASS; aggregate host reset additionally hit its
  180 s limit. Its Windows-only budget is 360 s; retain the initial FAIL and
  the separate unchanged-scenario rerun PASS (176.16 s) in the follow-up report.
  All 36 unique cases now have a latest PASS across the full run and rerun,
  not one newly executed all-green full invocation.
- `TEST-20260928-PITCH-LAYOUT`: after fixing the bank oracle, the processor test
  fails at `pitch faders leave room for values`. The test assumes bottom <=490
  reference units; approved layout uses y=408, height=88 (bottom=496). FIXED
  test oracle: use JUCE's thumb-inset drawing area and check it ends before the
  value label at y=488. The renderer clamps the cap within that drawing area.
  Complete processor executable rerun PASS; production GUI remains unchanged.

These results and subsequent reruns are recorded in the linked N1 validation
report. These checks do not start REAPER.

## Closed implementation and owner-approved scope

- GUI appearance is final: PRs #68/#69 and the final logo/separator polish in
  #79 are merged. The owner approved and tested the #79 GUI, and its macOS and
  Windows Actions passed. This does not close remaining release host/platform
  acceptance.
- Repository presentation and the three owner-supplied screenshots are merged
  in #70. English/Hungarian overviews and detailed guides exist; final package
  instructions and release notes still need candidate-specific review.
- Immutable export snapshots and critical-status priority are FIXED (#67).
  Keep byte-identical export acknowledgement and their regressions.
- Prior detune, bounded reads, factory-CC32 admission, supported keyboard notes,
  direct ROM-reload MIDI boundary and state-save ROM-generation pairing fixes
  stay closed unless a new regression is reproduced. Their final-RC runtime
  acceptance is not implied by merged source.
- Owner-reported pitch/mod automation recording and playback, GUI wheel
  following and pitch drag return-to-centre succeeded. This does not establish
  scroll semantics or every Write/Touch/Latch gesture boundary.

## Completed infrastructure change — local-ROM CTest failure semantics

- PR #75 merged as `af763f1`; it removes `SKIP_RETURN_CODE 77` from the five
  local-ROM CTest tests.
  Those tests are registered only after an existing ROM path is supplied, so
  an explicit but unloadable fixture must fail instead of becoming SKIPPED.
- A configuration-only check using an existing non-ROM file confirmed that
  the local-ROM tests register without a `SKIP_RETURN_CODE` property. No ROM
  test was executed, and no new Actions result was verified in this update.
  Merge does not replace the product-validation work below.

## 1. Input validation and wheel semantics

- [x] F3/F4 — FIXED and merged as PR #76 (`3c91f67`): live SysEx admission and
  internal packed import use the shared semantic validator. Reserved-bit
  preservation and transactional failures are tested. The current main also
  includes the later pitch-wheel input fix, PR #77.
- [x] F1 — FIXED and merged as PR #77 (`29ab5e3`): unintended pitch-wheel
  scroll input is constrained without changing host automation or incoming
  MIDI pitch bend; the GUI regression is included. Current main includes it.
- [x] F2 — general wheel-automation host test PASS (owner-tested in REAPER):
  both Pitch and Mod wheels record mouse movement and MIDI-keyboard control;
  manually drawn wheel curves play back, including the Mod Wheel curve.
  No wheel-automation defect was observed in this test.
- [ ] F2a — optional mode-boundary characterization: record exact
  begin/value/end ordering and centre point in REAPER Write, Touch and Latch.
  This was not part of the owner-reported test above; keep it separate from the
  passing general wheel-automation result and make no speculative behavior
  change.

## 2. ROM and project-state integrity

- [x] F5 — REPRODUCED on baseline `29ab5e3`; fix and focused local-ROM tests
  are merged in PR #78 (`23e1503`). New state
  records SHA-256 of firmware plus the effective factory voice image (or an
  explicit no-factory marker), independent of path. Mismatches keep project RAM
  pending; matching content at a new path resumes restore. Legacy states with
  no identity remain path-based for backward compatibility; a follow-up source
  review fixed the missing loaded-path comparison, covered by a passing local
  ROM-backed regression. See
  `docs/validation/VALIDATION_1.0_ROM_CONTENT_IDENTITY.md`. Keep this distinct
  from the already-fixed save-generation/path pairing race.
- [x] F6 — REPRODUCED on baseline `29ab5e3`; targeted fix and ROM-backed
  regression are merged in PR #78 (`23e1503`). A voice edit
  in a fresh no-ROM instance was discarded on first ROM load. The chosen
  behavior preserves explicit edits over the newly loaded initial voice; a
  pending saved project's packed RAM remains authoritative. See
  `docs/validation/VALIDATION_1.0_NO_ROM_FIRST_EDIT.md`.
  Local verification for the current branch: Release Standalone/VST3/AU and
  `vdx7_ci_checks` compiled; all 10 ROM-free tests passed. On 2026-09-26 the
  owner-supplied v1.8 package was used locally: F5 profile, state interleaving,
  and pending-identity regressions passed; the full ROM-backed stability suite
  passed, including F6 first-load edit retention. The fixture remains outside
  the repository and CI artifacts.
  Public Windows and macOS CI both PASS on `36df778` (2026-09-26), including
  plugin builds, all 10 ROM-free tests, and local-ROM test registration smoke;
  Actions does not run the firmware-dependent regressions without the private
  fixture. After the prepare-time epoch fix, Windows run `36269491224` and
  macOS run `36269491357` also PASS on `fed4721` (2026-09-26), with builds,
  ROM-free regressions, registration smoke, and packaging. The full local ROM
  suite was rerun on merged main `d696e56`; see
  `docs/validation/VALIDATION_1.0_MAIN_D696E56_ROM_SUITE.md`.
  See both F5/F6 validation notes for scope and fixture boundaries.
- [~] F8/F9 — SOURCE-DERIVED CANDIDATES: scoped by API/source review on
  2026-09-27; no actionable deadlock or host-reproduced whole-restore defect
  found. JUCE documents APVTS `copyState()` and `replaceState()` as individually
  thread-safe but not real-time-safe; VST3 permits state calls while processing
  (UI thread in real-time use, processing thread in offline use), but neither
  source specifies that overlapping whole-state get/set calls form one atomic
  plugin-wide transaction. In this plugin, engine snapshots are detached under
  `engineMutex_` before APVTS copies or host notifications; `parameterChanged`
  only publishes bounded/atomic edits; and `setStateInformation` releases the
  engine lock before replacing APVTS state and before host-facing parameter
  synchronization. Existing reentrant-save and state/process interleaving
  regressions pass. Do not add a broad mutex based on an unsupported
  simultaneous-restore assumption. Reopen this candidate only if a supported
  host demonstrates overlapping get/set calls producing a user-visible mixed
  state or a reachable deadlock; any fix must preserve the real-time boundary.
  Sources: [JUCE APVTS](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html)
  and [Steinberg VST3 processing FAQ](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Processing.html).
  The clarified disposition is merged in PR #83 (`94081f9`); Windows and macOS
  Actions both passed on that source.
- [x] F10 — REPRODUCED and fixed: Settings Apply partially committed tuning
  before a pending-restore MONO failure. `applySettingsFromUi` now
  validates first and performs the fallible MONO operation before committing
  tuning/channel; a v1.8 ROM-backed regression reproduces the former behavior
  and verifies the corrected all-or-none failure result. `vdx7_ci_checks` and
  35/35 ROM-backed/ROM-free CTest cases pass locally (the desktop-dependent
  SAVE AS integration test is excluded). The fix is merged in PR #78 and was
  included in the full local suite on main `d696e56`. Details:
  `docs/validation/VALIDATION_1.0_SETTINGS_APPLY.md`.

## 3. Original audio and DAW acceptance — still required

- [ ] F7 — HOST-DEPENDENT offline contention: reuse existing deterministic
  silent-block characterization, then compare repeated online, offline 1x and
  full-speed renders. Capture hashes, null difference, first differing sample,
  peak/RMS and silent blocks. Equal initial state is essential. If reproduced,
  design a separate offline synchronization contract, not a blind blocking lock.
- [ ] F15 — CHARACTERIZATION: short/slow release, nonzero L4, sustain, pitch
  envelope, stop and region end. Change zero tail metadata only with evidence;
  do not invent an arbitrary fixed tail duration.
- [ ] F16 — CHARACTERIZATION: dense CC/pitch/automation, sustain and Note Off,
  large blocks and contention, then fresh Note On/Off recovery. Keep bounded
  256-event/65536-byte policy; larger capacity alone is not an architectural fix.
- [x] Full merged-main local ROM suite: lifecycle/reset/reactivation, MIDI range,
  timing/stability/stress, ownership/history/overlap retirement, reset overflow,
  deferred partition, portamento, wheel delivery, direct reload, state-ROM
  identity and corrected MONO. On main `d696e56`, 35/35 CTest cases passed on
  macOS 26.7 with Apple clang 21.0.0 and CMake 4.4.3 using the owner-supplied
  v1.8 fixture locally. The desktop-dependent `vdx7_processor` SAVE AS test
  was attempted separately but stopped at its primary-display precondition in
  the command runner; the dialog checks remain NOT RUN. Full command and result:
  `docs/validation/VALIDATION_1.0_MAIN_D696E56_ROM_SUITE.md`. ROM stays local;
  public ROM-free CI is not firmware-runtime PASS. Repeat exact candidate
  testing if source changes after this main SHA.
- [x] Owner-reported REAPER PASS (2026-09-26): Native and Correct MONO note
  boundaries (11/12 and 120/121), Note Off, sustain and repeated-note behavior,
  automation and project reopen, transport, bypass, device restarts, physical
  MIDI, multiple instances and sample-rate/buffer checks all worked in the real
  REAPER test on macOS. Tested build SHA and full platform/rate matrix were not
  recorded; replay on the exact release-candidate SHA remains open.
- [x] Owner-reported Windows 10 x64 REAPER PASS (2026-09-26): tested with the
  Windows VST3 artifact from Actions run `Build Windows VST3 #219`, source
  `29ab5e350019e32f64650b068afcc7709b409607` (the main baseline). The user
  reports the tested functions behave as on macOS. REAPER version and detailed
  rate/block/instance coverage were not recorded. This predates the current
  branch's Windows Standalone CI addition and does not verify it.
- [ ] Operator/algorithm/feedback/master/pitch/mod automation; project reopen,
  missing/later ROM, USER/CUSTOM banks, SysEx import/export and dirty/clean state.
- [ ] Windows x64 and macOS REAPER matrix: 44.1/48/96 kHz, 64/128/256/512/1024
  samples where host-configurable; 1/4/8 instances, GUI open/closed, dense edits,
  physical MIDI and device restarts on the exact candidate SHA. Intel hardware
  acceptance or explicit limit.
- [ ] Retain original SRC frequency/aliasing/latency/listening checks, physical
  MIDI timing, audio allocation/locking audit and long-run overload/CPU tests.

## 4. Invisible GUI hardening and build coverage

- [ ] F12 — coverage gap: actual menu sizes 600×463, 900×694, 1200×925,
  1500×1156, 1800×1388. Add/adjust tests to exercise the actual five selectable
  sizes, including visible control bounds/overlap, editable fields, LCD,
  PERFORMANCE, Settings/About, tooltips, keyboard/footer and host window
  tracking. Include Windows/HiDPI. Preserve the approved appearance.
  The editor now disables arbitrary host/window resizing, and the GUI regression
  verifies no corner dragger plus in-bounds EDIT and PERFORMANCE views at all
  five fixed dimensions. It also checks all six named PERFORMANCE selectors,
  four controller ranges, twelve assignment switches, and panel-child bounds.
  A shared preset table now drives the Settings menu and has ROM-free tests for
  all five sizes and nearest-width selection. The About vector assets, child
  bounds and snapshot are also covered; the regression verifies the VDX7, GYR
  and signature vectors. Actual Settings dialog interaction, broader EDIT
  element/overlap checks, and Windows/HiDPI execution remain open. A local
  attempt to open the modal Settings window from the headless component test
  was unstable, so it is not counted as coverage or product evidence. The
  existing non-modal component/snapshot tests remain green.
  The 2026-09-28 Windows interactive Standalone follow-up was NOT RUN because
  the computer-use application launch approval timed out; no bypass attempted.
  After PR #83, all ten ROM-free tests passed locally on `94081f9`.
- [x] F11 — locally measured and implemented: remove five unused editor image
  loads, retaining all source artwork and used images. 0/1/4/8 fresh-process
  measurements (three repeats) and byte-identical before/after GUI snapshots
  are recorded in the follow-up validation. Eight-editor median sampled
  working set fell from 100.64 to 44.23 MiB; this is not DAW/audio CPU evidence.
- [x] F13 — historical manifest/spec now point to the current
  [runtime asset contract](../design/GUI_RUNTIME_ASSETS.md), with 1440×1110
  geometry, runtime/reference distinction and no promise of a full 2× pack.
- [x] F14 — the About regression requires all three vectors (VDX7, GYR and
  developer signature) and captures a rendered panel snapshot. The test passed
  in the GUI regression and merged cross-platform CI.
- [x] F18 — compile/link CI covers Standalone on Windows/macOS and AU on macOS.
  PR #79's macOS and Windows workflows passed; do not imply host acceptance
  from compilation.
- [x] F19 — optional ROM-free ASan/UBSan job for voice/SysEx/USER, deferred MIDI,
  latest display, bounded files, algorithms and status helper. Workflow added
  in this follow-up for seven non-GUI component tests on macOS; no ROM, no
  failure suppression. Run 36405070221 PASS on PR #88. LeakSanitizer is
  explicitly excluded on this platform; do not claim leak-test coverage.

## 5. Test-system audit follow-up

The supplied test-system review found no CMake syntax defect. It reports that
Windows/macOS configured and built the current graph and each ran ten ROM-free
CTest cases successfully. The items below are coverage/robustness work, not
evidence that the shipped instrument currently malfunctions.

### P1 — close misleading-green and important untested paths

- [x] CTest labels: apply `rom-free` consistently to every ROM-free test so
  `ctest -L rom-free` selects the complete ROM-free suite. Keep `gui` and other
  useful orthogonal labels where they already apply; verified locally with all
  ten registered ROM-free tests selected and passing.
- [x] Public-CI processor coverage: the ROM-free `vdx7_gui_header` target
  checks editor creation, no-ROM Save As/Algorithm disabled states, wheel input
  behavior, header rendering and white-key hover pixels at 1x/2x. Firmware
  integration remains separate and opt-in; no ROM data enters public CI.
- [x] USER-bank semantic corruption: checksum-valid, 7-bit-clean packed voice
  with an invalid semantic field is rejected transactionally (destination
  unchanged); covered by F3/F4 and verified in the local 10-test run.
- [x] Packed-VMEM invalid-field matrix: tests only fields whose accepted ranges
  are confirmed by the format/product contract; checksum-valid invalid imports
  are rejected and destination output remains unchanged.
- [x] GUI input coverage: ROM-free component-level tests verify pitch spring
  return on release, MOD retention, keyboard adjustment and pitch-scroll
  filtering in `vdx7_gui_header`. Scroll/trackpad and host automation gesture
  boundaries remain separate checks; preserve owner-reported REAPER evidence
  and do not call a missing test an observed product defect.

### P2 — make local, release and alternate build paths explicit

- [x] Give every CTest test an intentional timeout. ROM-free checks use
  20–60s; measured GUI/processor paths have wider limits; lifecycle/stress/soak
  keep their longer existing overrides. Inventory uses recent recorded runtime
  evidence (including ~9s GUI, ~26s portamento and ~45s corrected processor) to
  avoid tight wall-time limits. The ROM-on registration smoke confirmed all
  36 registered tests expose a timeout.
- [x] Add a no-execution CMake registration smoke to Windows/macOS CI:
  configure `VDX7_ENABLE_ROM_TESTS=ON` with a placeholder path, then inspect
  CTest's JSON listing. The checker covers all 36 ROM-on and 10 ROM-off names
  at baseline, fixture edges, labels, timeouts and absence of disabled/skip
  policy, replacing the earlier single-test assertion. This branch's new
  version-identity target changes the expected inventories to 37/11; the checker
  is updated accordingly. This is registration evidence, not ROM acceptance,
  and executes no firmware tests.
- [x] Compile smoke with `VDX7_RELEASE_BUILD=ON`, with no artifact publication:
  local macOS Release Standalone, VST3 and `vdx7_ci_checks` built; all 10
  ROM-free tests passed. This is a compile smoke only, not release or host
  acceptance.
- [x] Exercise supported compile targets: macOS CI includes Release
  Standalone, AU and VST3; Windows CI includes VST3 and Standalone. Both Actions
  passed on PR #79's exact source tree (runs `36303586325` and `36303586277`).
  Compilation is not host acceptance.
- [x] Exercise the corresponding-source/offline dependency path: a clean source
  snapshot with `third_party/JUCE` and `third_party/dx7Lib` populated from the
  documented pinned revisions configured with
  `FETCHCONTENT_FULLY_DISCONNECTED=ON`; VST3, AU, Standalone and CI-test targets
  built, and all 10 ROM-free tests passed. This validates the extracted source
  tree layout and offline build path, not archive publication or host acceptance.
- [x] Optional `pluginval` gate exercised locally: Windows 1.0.4, strictness 5,
  GUI enabled, default rate/block matrix, final detailed log SUCCESS. See the
  follow-up validation for command/tool hash and limitations. Repeat on exact
  RC. Separate Steinberg validator now PASS (47/47) on the extracted-source
  Windows build; see the source-packaging report for exact identity. Real host
  acceptance and final-RC repetition remain open.
- [x] Clarify the local-ROM contract in CMake and the HU/EN guide: one shared
  product accepts 16 KB firmware (optional sibling factory voices) or a 48 KB
  combined image, but the complete opt-in suite requires a combined 48 KB v1.8
  image. This corrects the previous broader fixture claim. All 26 local-ROM
  registrations are now covered by a complete inventory checker; placeholder
  CI never executes firmware tests.

### P3 — naming and source-quality polish

- [ ] `vdx7_all_tests` currently builds test executables but does not execute
  them; docs explain that CTest must follow. Consider renaming the target or
  adding a separate build-and-run target without changing current commands
  silently.
- [ ] Consider a consistent warning interface target for project/test sources
  (not third-party JUCE files). Keep `-Werror` out of release-critical builds
  until cross-platform warning cleanliness is established.

### Already dispositioned by the review

- The current CI invocation runs all registered tests with `--no-tests=error`;
  the zero-tests/vacuous-green concern is already guarded.
- Integration executables compiling on public CI is useful, but does not mean
  their ROM-dependent runtime tests ran. Keep compile and firmware-runtime
  status separate.
- The reported 10/10 public CTest result is a ROM-free result, not the full local
  suite, exact-SHA private-ROM acceptance, REAPER acceptance, or release PASS.

## 6. Exact candidate and release — original gates retained

- [x] F17 — [identity/package review](IDENTITY_AND_PACKAGE_1.0.md) documents
  numeric host 1.0.0 versus displayed dev/RC/stable labels, exact-SHA artifact
  naming and tested `rcN` mode resolution. Historical bundle/plugin IDs are
  preserved. Cosmetic prototype DESCRIPTION cleanup remains a separate
  candidate decision.
- [x] Freeze test-only RC1 SHA `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684`;
  exact-SHA Windows/macOS candidate workflow `36451459751` passed and uploaded
  both platform artifacts. This does not close the separate full local-ROM
  suite or host matrix.
- [x] Document single USER-library limitation and supported formats/hosts:
  HU/EN guides and the identity/package review distinguish primary VST3,
  compile-only AU/Standalone coverage and dated host evidence. Exact RC host
  acceptance is still required; no new runtime support claim was made.
- [~] Exact-RC packages and corresponding-source archives were downloaded and
  inspected; source manifests verified and binary/source hashes recorded.
  Windows is unsigned and macOS is ad-hoc signed (not notarized). Final HU/EN
  guide/release-note review and owner-side acceptance remain open.
- [ ] Separate approval before merge/publication as applicable; never bypass
  required checks, auto-tag or auto-publish a stable release from this plan.

## Guardrails and evidence

Preserve 148 parameter IDs/order, plugin identity, legacy project state, Native
default/optional Correct mode, Note 12–120, bounded MIDI, state epochs, USER
compare-before-overwrite and immutable export. No new callback allocation or
filesystem I/O. No ROM in commits or artifacts. No general refactor or GUI redesign.

Each implementation round: exact baseline → failing regression/reproduction →
minimal fix → targeted/full relevant tests → Windows/macOS CI → reviewed merge.
Use CONFIRMED, REPRODUCED, SOURCE-DERIVED CANDIDATE, HOST-DEPENDENT,
CHARACTERIZATION, NOT RUN, FIXED and PASS accurately. PASS requires execution;
source inspection, prior runs and owner reports retain their specific scope.

Next concrete order (after PR #93; subject to the active audit ledger above):
1. Keep the approved GUI appearance frozen. Close the remaining F12 evidence:
   real Settings/About interaction and Windows/HiDPI verification using a safe
   desktop test or explicit owner verification; do not count headless modal
   attempts as PASS.
2. [x] Select test-only `1.0.0-rc1` at exact SHA
   `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` and pass the exact-candidate
   workflow. It created CI artifacts only; it did not publish a release or tag.
3. On that exact RC, run the local-ROM regression suite and close
   the host/audio checks still marked open: F7 offline-render comparison, F15
   envelope/release characterization, F16 dense MIDI/contention recovery, and
   the listed REAPER feature/platform matrix. Record unsupported cells rather
   than silently treating them as PASS. Older owner-reported REAPER runs remain
   evidence for their recorded builds, not this RC.
4. [~] Inspect the matching VST3/source artifacts, pinned dependencies, notices,
   checksums and archive contents; finalize HU/EN guides and release notes with
   exact-SHA evidence and accepted limitations. Artifact inspection and hashes
   are recorded; owner review and final document cleanup remain.
5. Present the version, SHA, asset list, checksums and unresolved/accepted items
   for owner review. Only a separate explicit publication authorization permits
   creating the stable tag/release; verify its links and checksums afterward.

F5/F6 are already fixed and merged together in PR #78; do not list them as the
next implementation step. F2's general wheel-automation host test is accepted;
the optional Write/Touch/Latch boundary characterization can be done separately
and is not a failure or blocker for that result. Local ROM-free PASS is not
firmware-runtime or host acceptance. PR #91 is merged and the listed platform
Actions passed, but this plan does not authorize a tag or release publication.
