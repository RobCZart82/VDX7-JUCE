# VDX7 Mk1. 1.0.0 — draft release notes and test matrix

**DRAFT — not an accepted release and not publication authorization.**
Test-only RC1 has been built from an exact SHA and passed automated CI, but
private-ROM runtime, final host acceptance and owner publication approval are
still outstanding. Do not turn owner reports into CI results.

Preparation checkpoint: PR #93 is merged on main at
`aeb4d5ee8439ba6a7346bfe7caba54ad90b21684`. Exact-RC workflow `36451459751`
passed for Windows x64 and macOS universal; artifacts are test downloads, not a
published GitHub release or stable release approval.

## English

VDX7 Mk1. is an open-source six-operator FM instrument built around the VDX7
DX7 Mk I emulation core and JUCE. Version 1.0.0 packages the owner-approved GUI
and current implementation as a release candidate for final compatibility and
host acceptance.

### Highlights

- Six operators, 32 algorithms, operator envelopes, pitch envelope and LFO.
- EDIT and PERFORMANCE pages, fixed GUI size presets, on-screen keyboard,
  pitch/mod wheels and stereo output meters.
- POLY/MONO play modes, pitch-bend range/step, portamento and performance
  controller assignments.
- Voice and bank SysEx import/export, one persistent 32-slot USER bank, and
  148 host automation parameters with DAW project-state recall.
- Windows x64 and macOS Universal VST3 build targets.

### Requirements and known limits

- A compatible, legally obtained DX7 Mk I ROM is required for sound. Firmware
  and factory voice data are not included.
- Supported MIDI note range is 12–120. The USER area is one 32-slot bank, not a
  named-library manager.
- Live MIDI Out/SysEx transmission is not implemented. Envelope graphs are
  visualizations, not calibrated time displays.
- Current Windows packages have no publisher signature. macOS packages use an
  ad-hoc signature, not Developer ID signing or notarization; operating-system
  warnings may appear. Do not disable system-wide security protections.
- AU and Standalone builds may compile, but compilation alone is not a claim of
  runtime acceptance or distribution support.

### Candidate validation record — fill for the frozen RC

| Item | Evidence / status |
| --- | --- |
| RC version and full source SHA | `1.0.0-rc1`, `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` |
| Windows/macOS exact-SHA CI and candidate workflow | PASS — run `36451459751` |
| Matching source archive, manifest, package inspection and checksums | PASS for CI artifacts; see exact-RC validation report for SHA-256 and limits |
| Windows REAPER exact-RC1 smoke | Owner reports exact RC1 works; REAPER/Windows versions, binary hash confirmation and detailed procedure not recorded; full matrix remains open |
| macOS REAPER exact-RC1 smoke | Owner reports exact RC1 works; supplied screenshot shows REAPER 7.80 and RC1 label; full matrix remains open |
| Windows 10 x64 REAPER | Owner previously reported testing build #219 (`29ab5e3`) without known functional issues; not the final RC matrix |
| macOS REAPER | Owner reported testing on macOS 26.7; exact tested build/hash and complete matrix not recorded |
| Windows VST3 build #247 installation | Owner reports installed artifact from source `fbea5ea`; reported VST3 SHA-256 `1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`; installation only, no new functional result stated |
| Final host/audio/GUI acceptance | OPEN; run and record each required cell against this exact RC |

## Magyar

A VDX7 Mk1. nyílt forrású, hatoperátoros FM hangszer a VDX7 DX7 Mk I
emulációs magjára és a JUCE-ra építve. Az 1.0.0 a tulajdonos által jóváhagyott
GUI-t és az aktuális implementációt készíti elő a végső kompatibilitási és
host-elfogadási ellenőrzésre.

Előkészítési állapot: a #93 PR beolvadt a `main` ágba a
`aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` commitban. A pontos RC-workflow
(`36451459751`) sikeres Windows x64 és macOS Universal platformon. Ezek teszt-
letöltések, nem publikált GitHub Release és nem végleges kiadási jóváhagyás.

### Főbb jellemzők

- Hat operátor, 32 algoritmus, operátorburkolók, pitch envelope és LFO.
- EDIT és PERFORMANCE oldalak, rögzített GUI-méretek, képernyő-billentyűzet,
  pitch/mod kerekek és sztereó kivezérlésmérők.
- POLY/MONO mód, pitch-bend tartomány/lépés, portamento és előadói
  vezérlő-hozzárendelések.
- Hangszín- és bank-SysEx import/export, egy tartós, 32 helyes USER-bank, valamint
  148 host-automatizálási paraméter DAW-projektállapot-visszaállítással.
- Windows x64 és macOS Universal VST3 fordítási cél.

### Követelmények és ismert korlátok

- A megszólaláshoz saját, jogszerűen beszerzett, kompatibilis DX7 Mk I ROM kell.
  Firmware és gyári hangadat nincs mellékelve.
- A támogatott MIDI-hangtartomány 12–120. A USER-terület egy 32 helyes bank,
  nem elnevezhető bankokat kezelő könyvtár.
- Élő MIDI Out/SysEx-küldés nincs. A burkológörbe-ábrák szemléltetnek, nem
  kalibrált idődiagramok.
- A jelenlegi Windows-csomagokon nincs kiadói aláírás. A macOS-csomag ad-hoc
  aláírású, nem Developer ID-aláírt és nem notarizált; az operációs rendszer
  figyelmeztethet. A rendszer egészére kiterjedő védelmet ne kapcsold ki.
- Az AU és Standalone fordíthatósága önmagában nem jelent futásidejű elfogadást
  vagy hivatalos terjesztési támogatást.

### Jelöltverzió tesztjegyzéke — a befagyasztott RC-hez kitöltendő

| Tétel | Bizonyíték / állapot |
| --- | --- |
| RC-verzió és teljes forrás-SHA | `1.0.0-rc1`, `aeb4d5ee8439ba6a7346bfe7caba54ad90b21684` |
| Windows/macOS pontos SHA-jú CI és jelöltworkflow | PASS — `36451459751` |
| Egyező forrásarchívum, manifest, csomagvizsgálat és ellenőrzőösszegek | PASS a CI artifactokra; részletek a pontos-RC validációban |
| Windows REAPER exact-RC1 próba | A tulajdonos szerint a pontos RC1 működik; a REAPER/Windows-verzió, a használt bináris hash-egyezése és a részletes eljárás nincs rögzítve; a teljes mátrix nyitott |
| macOS REAPER exact-RC1 próba | A tulajdonos szerint a pontos RC1 működik; a csatolt képen REAPER 7.80 és az RC1 felirat látszik; a teljes mátrix nyitott |
| Windows 10 x64 REAPER | A tulajdonos korábban a #219-es (`29ab5e3`) buildet ismert probléma nélkül tesztelte; ez nem a végleges RC tesztmátrixa |
| macOS REAPER | A tulajdonos macOS 26.7-en végzett tesztről számolt be; a pontos build/hash és a teljes mátrix nincs rögzítve |
| #247 Windows VST3 telepítése | A tulajdonos a `fbea5ea` forrásból készült csomag telepítését jelezte; megadott VST3 SHA-256: `1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`; ez telepítési adat, új funkcionális teszteredményt nem közölt |
| Végső host/audio/GUI elfogadás | NYITOTT; minden tényleges tesztet a pontos RC-hez kell rögzíteni |

E dokumentum tervezet marad a privát-ROM-os regressziók, a host/audio/GUI
elfogadás, a végleges útmutató- és csomagellenőrzés, valamint a külön
publikálási jóváhagyás előtt.
