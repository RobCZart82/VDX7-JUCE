# VDX7 Mk1. 1.0.0 — draft release notes and test matrix

**DRAFT — not an accepted release and not publication authorization.**
The exact release-candidate commit, artifacts and final acceptance results have
not yet been selected. Replace the candidate-specific placeholders only after
the exact RC is frozen and tested; do not turn owner reports into CI results.

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
| RC version and full source SHA | NOT SELECTED |
| Windows/macOS exact-SHA CI and candidate workflow | NOT RUN |
| Matching source archive, manifest, package inspection and checksums | NOT RUN for an RC |
| Windows 10 x64 REAPER | Owner previously reported testing build #219 (`29ab5e3`) without known functional issues; not the final RC matrix |
| macOS REAPER | Owner reported testing on macOS 26.7; exact tested build/hash and complete matrix not recorded |
| Windows VST3 build #247 installation | Owner reports installed artifact from source `fbea5ea`; reported VST3 SHA-256 `1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`; installation only, no new functional result stated |
| Final host/audio/GUI acceptance | OPEN; record each executed cell against the exact RC |

## Magyar

A VDX7 Mk1. nyílt forrású, hatoperátoros FM hangszer a VDX7 DX7 Mk I
emulációs magjára és a JUCE-ra építve. Az 1.0.0 a tulajdonos által jóváhagyott
GUI-t és az aktuális implementációt készíti elő a végső kompatibilitási és
host-elfogadási ellenőrzésre.

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
| RC-verzió és teljes forrás-SHA | MÉG NINCS KIJELÖLVE |
| Windows/macOS pontos SHA-jú CI és jelöltworkflow | NEM FUTOTT LE |
| Egyező forrásarchívum, manifest, csomagvizsgálat és ellenőrzőösszegek | RC-re MÉG NEM FUTOTT LE |
| Windows 10 x64 REAPER | A tulajdonos korábban a #219-es (`29ab5e3`) buildet ismert probléma nélkül tesztelte; ez nem a végleges RC tesztmátrixa |
| macOS REAPER | A tulajdonos macOS 26.7-en végzett tesztről számolt be; a pontos build/hash és a teljes mátrix nincs rögzítve |
| #247 Windows VST3 telepítése | A tulajdonos a `fbea5ea` forrásból készült csomag telepítését jelezte; megadott VST3 SHA-256: `1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`; ez telepítési adat, új funkcionális teszteredményt nem közölt |
| Végső host/audio/GUI elfogadás | NYITOTT; minden tényleges tesztet a pontos RC-hez kell rögzíteni |

E dokumentum tervezet marad a pontos RC tesztelése, a kiadási fájlok ellenőrzése
és a külön publikálási jóváhagyás előtt.
