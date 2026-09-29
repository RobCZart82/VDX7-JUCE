# VDX7 Mk1. 1.0.0 — draft release notes and test matrix

**DRAFT — not final public release assets.** The exact stable product source is main commit `d79ed5214d82caf70e3941e5a620bab137d3f9ca`. The owner reports successful installation and REAPER use of the Windows x64 installer and macOS Universal package; the owner reports the plugin works and sounds good. The macOS installation required the per-app Gatekeeper “Open Anyway” action because the package is unsigned/not notarized. This is owner-reported acceptance, not a full host/audio/GUI matrix; unrun/deferred checks remain explicit below.

The non-publishing installer workflow [36621909919](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36621909919) passed on that exact source SHA. Windows/macOS build, ROM-free tests, platform packaging checks and combined SHA-256 verification passed. Combined validation artifact ID `11059321610`; outer ZIP SHA-256 `65d3ac7195a00f7d3040816cbbd257ea247f0cb65d487c6823cc8c02675712a3`. This artifact is not a public Release: BUILD-INFO labels it as not approved for publication. Independent content inspection and final release copies of BUILD-INFO/checksum files remain pending. AU is not part of 1.0.0 distribution.

## English

VDX7 Mk1. is an open-source six-operator FM instrument built around the VDX7
DX7 Mk I emulation core and JUCE. The 1.0.0 release is being prepared from the
owner-approved GUI and implementation. Stable release identity, final assets
and publication are not yet approved or published.

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
| Local-ROM suite on exact product source | PASS — 36/36 CTests; desktop-dependent `vdx7_processor` save-dialog test excluded; no ROM in CI/artifacts |
| Windows REAPER exact-RC1 smoke/listening | Owner reports successful operation and excellent, DX7-faithful audio on Windows 10 Pro 22H2 (build 19045.7663), Core i7-6600U, 8 GB RAM; REAPER version and installed binary hash not recorded |
| macOS REAPER exact-RC1 smoke/listening | Owner reports successful operation and excellent, DX7-faithful audio; screenshot shows REAPER 7.80 and RC1 label; machine reported as Mac mini M1 (2020), 16 GB, macOS Tahoe 26.7. Installed VST3 hash matches the downloaded macOS candidate artifact |
| macOS local REAPER smoke | Assistant confirmed `v1.0.0-rc1`, local ROM/factory-bank loading and visible host output-meter activity from a MIDI note; not a subjective listening or full matrix test |
| Five instances and project persistence | Owner reports five VDX7 instances in one REAPER project, successful save and reopen; OS, duration and CPU details not supplied |
| Windows 10 x64 REAPER | Owner previously reported testing build #219 (`29ab5e3`) without known functional issues; not the final RC matrix |
| macOS REAPER | Owner reported testing on macOS 26.7; exact tested build/hash and complete matrix not recorded |
| Windows VST3 build #247 installation | Owner reports installed artifact from source `fbea5ea`; reported VST3 SHA-256 `1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`; installation only, no new functional result stated |
| Remaining broad host/audio/GUI matrix | NOT RUN / DEFERRED by owner's decision to wind down open-ended testing; see checklist for specific cells and limits |
| Stable `1.0.0` Windows/macOS installer build and checksum verification | PASS — workflow `36621909919` on main SHA `d79ed5214d82caf70e3941e5a620bab137d3f9ca`; artifact `11059321610`. Owner reports both installers and REAPER plugins work; independent archive review remains pending. |
| Exact stable Windows/macOS installer and REAPER test | OWNER REPORT: both platforms install and work in REAPER; macOS Gatekeeper required per-app Open Anyway. Exact installed checksums and host versions were not supplied. |
| Independent inner-archive/hash review | NOT YET DONE; Actions artifact metadata digests are recorded in execution plan but are not inner ZIP hashes |
| Publication | NOT PUBLISHED. Validation artifact remains preparation-only; independent content review and final release asset regeneration remain pending. |

## Magyar

A VDX7 Mk1. nyílt forrású, hatoperátoros FM hangszer a VDX7 DX7 Mk I
emulációs magjára és a JUCE-ra építve. Az 1.0.0 kiadást a tulajdonos által
jóváhagyott GUI-val és implementációval készítjük elő. A stabil kiadási
azonosság, végleges csomagok és publikálás még nincs jóváhagyva vagy közzétéve.

Előkészítési állapot (2026-09-29): a stabil csomagoló workflow
(`36484917908`) sikeresen lefutott a `f7a1248b2cfff0b6fb159c189ca6a5b2cff766ec`
termék-SHA-n Windows x64 és macOS Universal platformra. A tulajdonos jelzése
szerint mindkét pontos Actions-csomag működik. Ez tulajdonosi futtatási
elfogadás; a telepített csomagok hash-e, host/verzió és részletes tesztmátrix
nincs megadva. A belső archívumok és checksum-manifestek független ellenőrzése
még hátravan. A jelenlegi main dokumentációs utófrissítés, nem változtat a
csomagolt termékforráson. Stabil GitHub Release, tag és publikálási jóváhagyás
továbbra sincs; a korábbi nyilvános kiadások történeti pre-beta verziók.

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
| Helyi ROM-os tesztcsomag a pontos termékforráson | PASS — 36/36 CTest; az asztali `vdx7_processor` mentési párbeszédablakot nyitó teszt kimaradt; ROM nem került CI-ba/csomagba |
| Windows REAPER exact-RC1 próba/hallgatás | A tulajdonos szerint működik, hangminősége kiváló és DX7-hű; Windows 10 Pro 22H2 (19045.7663), Core i7-6600U, 8 GB RAM; REAPER-verzió és telepített bináris hash nincs rögzítve |
| macOS REAPER exact-RC1 próba/hallgatás | A tulajdonos szerint működik, hangminősége kiváló és DX7-hű; képernyőkép: REAPER 7.80, RC1 felirat; Mac mini M1 (2020), 16 GB, macOS Tahoe 26.7. A telepített VST3 hash egyezik a letöltött macOS jelöltcsomagéval |
| Helyi macOS REAPER-próba | Az asszisztens ellenőrizte az `v1.0.0-rc1` verziót, a helyi ROM/gyári bank betöltését és MIDI hangra a host kivezérlésmérőjének mozgását; nem teljes mátrix és nem szubjektív hangteszt |
| Öt példány és projektállapot | A tulajdonos szerint öt VDX7 egy REAPER-projektben sikeresen menthető és újranyitható; az OS, időtartam és CPU-adatok nincsenek megadva |
| Windows 10 x64 REAPER | A tulajdonos korábban a #219-es (`29ab5e3`) buildet ismert probléma nélkül tesztelte; ez nem a végleges RC tesztmátrixa |
| macOS REAPER | A tulajdonos macOS 26.7-en végzett tesztről számolt be; a pontos build/hash és a teljes mátrix nincs rögzítve |
| #247 Windows VST3 telepítése | A tulajdonos a `fbea5ea` forrásból készült csomag telepítését jelezte; megadott VST3 SHA-256: `1602ea61092728538498303d1eb60f090bbdd718a0c43f626722dafddf45b1f6`; ez telepítési adat, új funkcionális teszteredményt nem közölt |
| A teljes host/audio/GUI mátrix | NEM FUTOTT / TULAJDONOSI DÖNTÉS ALAPJÁN HALASZTVA; a konkrét tételek és korlátok a jegyzékben |
| Stabil `1.0.0` Windows/macOS telepítő build és checksum ellenőrzés | PASS — `36621909919` workflow a `d79ed5214d82caf70e3941e5a620bab137d3f9ca` main SHA-n; artifact `11059321610`. A tulajdonos szerint mindkét telepítő és REAPER plugin működik; független archívumvizsgálat még hátra van. |
| Stabil Windows/macOS telepítő és REAPER-próba | TULAJDONOSI JELENTÉS: mindkét telepítő és plugin működik REAPER-ben; macOS-en per-app Open Anyway kellett. Pontos telepített checksumok és host-verziók nincsenek megadva. |
| Belső archívumok/hash-ek független vizsgálata | MÉG HÁTRA VAN; az Actions külső artefaktumhash nem a belső ZIP hash-e |
| Publikálás | MÉG NINCS KÖZZÉTÉVE. Az ellenőrző artifact előkészítő csomag; a független tartalmi vizsgálat és a végleges release csomag újragenerálása hátra van. |

Ez a dokumentum kiadási jegyzet-tervezet marad a stabil csomag véglegesítése,
a hátralévő kockázatok áttekintése és a külön publikálási jóváhagyás előtt.
