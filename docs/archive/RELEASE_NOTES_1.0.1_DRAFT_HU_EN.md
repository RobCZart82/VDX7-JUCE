# VDX7 Mk1. 1.0.1 — draft / tervezet

> ARCHIVED 2026-10-05: dated preparation history, not current instructions. [Active plan](../development/DEVELOPMENT_PLAN.md). Original status and checkboxes retain their original scope.

> **2026-10-05 publication checkpoint:** [1.0.1 is published](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1), with [matching source](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source).
> Az 1.0.1 megjelent. [Current acceptance, owner-authorized deferrals and publication record](../validation/VALIDATION_20261005_PUBLICATION_101.md) supersede earlier open publication gates below. Earlier candidate/test statements are retained as dated preparation history, not current release status.

Not published. General owner-reported Windows and M1 Mac operation is PASS;
detailed matrix acceptance/deferrals remain open. This records the corrective candidate; it does not
replace the [single execution plan](EXECUTION_PLAN_1.0.md) or grant publication
approval. Published v1.0.0 and its assets remain unchanged.

2026-10-05 current candidate: main `96267f7304e657821ce35c54536689981f41ef27`
contains the single-voice export acknowledgement correction merged in #128.
All older package hashes and approvals below are historical, not this candidate.
Preparation has passed; the owner explicitly approved exact A=B 96267f7 on
2026-10-05. Policy #130 and distinct post-merge main checks PASS at C
`edcb7471bf2ee08d3f8430e317cb9d17274f7576`. Canonical accepted-mode final
packaging `37276345684` and independent actual-file/source/offline verification
PASS. The actual final Windows VST3 validator PASS 47/47. The owner reports
successful use of this named final bundle on Windows 11 Pro 26H2 build
26300.9457 with REAPER 7.82 x64, and PKG installation/use on Mac mini M1 (2020),
16 GB RAM, macOS 26.7.1 with REAPER 7.82 Universal. These are human reports,
not independently verified local hashes or complete matrix coverage. Intel Mac,
active native/Rosetta mode and remaining detailed cases are not established.
Remaining acceptance evidence/explicit deferrals and separate publication permission
remain open. See the [owner feedback record](../validation/AUDIT_REPORT_20261005_CODE_AND_REPOSITORY_REVIEW.md).
The [current final test report](../validation/VALIDATION_20261005_FINAL_EXPORT_FIX_PACKAGE_101.md)
records exact identities and remaining gates.
The actual new preparation files and inspection limits are recorded in the
[current preparation report](../validation/VALIDATION_20261005_EXPORT_FIX_PACKAGE_PREP_101.md).

Historical 2026-10-04 bank-folder candidate: dbad14a includes the legacy-bank correction,
content-identified folder and bounded project-data decoding. Its preparation
packages and fresh offline source verification passed; see the
[new exact preparation report](../validation/VALIDATION_20261004_BANK_FOLDER_PACKAGE_PREP_101.md).
The owner authorized continuing final test packaging for exact A=B dbad14a
on 2026-10-04. Policy #126 merged at C3766035 after green checks/reviews;
post-merge main and final accepted-mode packaging `37233357714` passed.
Independent actual-file/source/offline verification passed; see the
[previous bank-folder final hashes and test handoff](../validation/VALIDATION_20261004_FINAL_BANK_FOLDER_PACKAGE_101.md).
Owner host acceptance and separate publication permission remain open.

2026-10-04 update: the legacy-bank acceptance fix changes product code. The
tuple, packaging run and hashes below belong to the previous b7fce05 candidate,
not to the fixed build. Rebuild, reverify and obtain exact final-candidate host
acceptance before using these notes for publication; retain old evidence as history.

Owner-approved product/tooling A=B: `b7fce0503059c8c2e3c61d6ccf5af33612739e32`.
Approval C: `828320d78e5d2e193356af0485a15c12f0279824`.
Final packaging: [37195536400](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/37195536400),
accepted metadata enabled for the reviewed pair; this does not publish.
The owner will manually install/test final Windows/macOS packages in REAPER.
Exact candidate preparation and source offline build passed; see
[source build](../validation/VALIDATION_20261002_SOURCE_BUILD_101.md).
Final artifact inspection and host acceptance remain separate gates.
Final artifact inspection is now PASS for run 37195536400; host acceptance
remains pending. [Exact final evidence](../validation/VALIDATION_20261004_FINAL_PACKAGE_101.md).
Final acceptance and hashes must be reconciled before turning this
draft into public notes. No 1.0.1 host acceptance is inferred
from the owner's earlier 1.0.0 REAPER tests.

## English

Supported firmware is the original Yamaha DX7 Mk I v1.8 (IG11469).
Special Edition / SER-7 firmware is not supported. Supply your own legally
obtained ROM; successful loading alone does not establish compatibility.

The corrective source includes pending-project edit/state protection, deferred
MIDI CC120/CC121 ordering and controller-reset fixes, full factory-bank input
validation, and source-package/checksum/provenance guard corrections. Exact
findings and evidence are linked from the execution plan, not treated as a new
feature or stability guarantee here.

Single-voice SysEx export now clears the unexported-edit warning correctly for
unchanged snapshots with reserved VMEM bits. It preserves working RAM, later
edits and failed-write warnings. Private bank-folder integration is now an
optional CTest with filename-independent fixtures; no proprietary data is shipped.

The new bank-compatibility fix accepts operator EG rate/level 127 and fine
frequency 100 found in archived banks, without rewriting downloaded files.
USER/project storage and SysEx export retain those values. Other semantic,
seven-bit, structure and checksum checks remain enforced. Editor ranges stay 0–99.

The candidate includes the requested factory bank folder: SETTINGS opens the folder and
refreshes content-identified ROM1A–ROM4B slots from user-supplied SysEx files.
Partial libraries are supported; projects retain their own catalog and latest
edited sound even if local bank files change/disappear. No Yamaha bank bytes
are distributed. This feature and the export correction are included in the
current 96267f7 packages; the historical hashes below do not identify them.
General owner operation is PASS; detailed acceptance/disposition remains open.

Upgrade warning: invalid factory data in a combined 48 KB ROM is now rejected
in full without replacing the previous instrument state. An old project tied
to that ROM may remain pending. A manually edited or different ROM is not a
guaranteed matching-ROM recovery. Back up the original plug-in, projects, ROM
and edited banks; verify recall on copies. Invalid optional separate 32 KB
factory data is ignored with a warning. No Yamaha firmware/data is supplied.

Planned downloads: Windows x64 EXE installer and Manual Install ZIP; macOS
Universal PKG installer and Manual Install ZIP. Windows has no publisher
signature; macOS uses technical ad-hoc signing, not Developer ID or notarization.
Loading/installation warnings may occur; do not disable system-wide protections.
See the [English guide](../guides/GUIDE_EN.md).

## Magyar

A támogatott firmware az eredeti Yamaha DX7 Mk I v1.8 (IG11469).
A Special Edition / SER-7 firmware nem támogatott. Saját, jogszerűen használható
ROM szükséges; a sikeres betöltés önmagában nem jelent támogatott kompatibilitást.

A javítóforrás a függőben lévő projektek szerkesztési/állapotvédelmét, a CC120/CC121
halasztott MIDI-sorrendjét és vezérlő-visszaállítását, a teljes gyári bank
ellenőrzését, valamint a forráscsomag, ellenőrzőösszeg és eredetigazolási kapuk
javításait tartalmazza. A bizonyítékokat a fejlesztési terv vezeti; ez nem
általános hibamentességi garancia.

Az egyhangszínes SysEx-export a fenntartott VMEM biteket tartalmazó, az export
óta változatlan hangszínnél is helyesen törli a nem exportált módosítás jelzőjét.
A munkahangszínt nem írja át; a későbbi szerkesztés és sikertelen írás jelzője
megmarad. A privát bankmappás integráció külön, fájlnévfüggetlen CTestként is
futtatható; jogvédett adatot nem mellékelünk.

Az új bankkompatibilitási javítás elfogadja az archivált bankok 127-es
operátor EG sebesség/szint és 100-as finomhangolási értékét, a letöltött fájlok
átírása nélkül. A USER-/projekttárolás és a SysEx-export ezeket megőrzi.
A többi paraméterhatár, a 7 bites adatok, a szerkezet és a checksum ellenőrzése
megmarad. A szerkesztő tartománya továbbra is 0–99.

Tulajdonosi kérésre gyári bankmappa is készült: a SETTINGS megnyitja a mappát
és saját SysEx fájlok tartalma alapján frissíti a ROM1A–ROM4B helyeket.
Részleges készlet is használható; a projekt saját bankmásolata és legutóbbi
szerkesztett hangja megmarad a helyi fájlok változásától/eltűnésétől függetlenül.
Yamaha bankadatot nem terjesztünk. A funkció és az exportjavítás az aktuális
96267f7 csomag része; az alábbi történeti hash-ek nem ezt azonosítják.
A tulajdonosi általános működés PASS, a tételes elfogadás/halasztás még nyitott.

Hibás gyári adatot tartalmazó kombinált 48 KB-os ROM teljes betöltése elutasítva,
a korábbi hangszerállapot megőrzésével. Az ehhez kötött régi projekt függőben
maradhat; módosított vagy másik ROM nem garantált helyreállítás. Készíts mentést
az eredeti plug-inről, projektekről, ROM-ról és szerkesztett bankokról; másolatokon
ellenőrizd a visszatöltést. Hibás külön opcionális 32 KB-os gyári bankot
figyelmeztetéssel kihagy. Yamaha firmware/hangadat nincs mellékelve.

A tervezett négy letöltés: Windows x64 EXE + kézi ZIP, macOS Universal PKG +
kézi ZIP. Windows kiadói aláírás nincs; macOS-en technikai ad-hoc aláírás van,
Developer ID és notarizáció nincs. Biztonsági figyelmeztetés/betöltési akadály
előfordulhat; ne kapcsold ki a rendszer védelmét. [Magyar kézikönyv](../guides/GUIDE_HU.md).

## Publication gate — still open

The dbad14a accepted-mode results above describe the previous candidate.
For the export-fixed 96267f7 candidate, exact-pair approval is now recorded
through #130; final packaging and actual-file inspection PASS in the current
report. Preparation metadata is not substituted for that evidence. Remaining
gates are exact-final owner host acceptance/disposition, final public text and
durable source links, and separate publication authorization.

- Actual new exact accepted-mode hashes, installer/upgrade evidence and complete
  matching source/checksum are verified for run 37276345684, not inferred from
  the older dbad14a or b7fce05 evidence. Provide the planned
  separate durable `v1.0.1-source` release link before/no later than binaries.
- A=B 96267f7 and C edcb747 are recorded with full SHAs in the current report.
  General owner Windows/M1 operation is PASS; complete the remaining detailed
  REAPER acceptance or explicitly permitted disposition of unrun checks.
- Obtain separate authorization for tags, source/product releases and uploads;
  preserve the four-download policy and do not modify v1.0.0.

## Current final export corrected download hashes

Verified actual files from accepted-mode run 37276345684. A=B is
`96267f7304e657821ce35c54536689981f41ef27`; C is
`edcb7471bf2ee08d3f8430e317cb9d17274f7576`. These are test artifacts, not
published files or fully matrix-accepted binaries. General owner Windows/M1
operation is PASS. See the current report above for checks and
limitations. The old hashes below must not be copied into the new release.

| File | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `d9472981ee8d651ca1b609e3e94683470e19be2a7d841c21e1b62edc394ac84e` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `ebe161d3826cb885cfa2c62aa85458cdad28414d80e1f1b59fcfe056a87bda88` |
| `VDX7-1.0.1-macOS-universal.pkg` | `08be0e9d93db219f0516a42c2848ba5e4b10f72a9e57cfca1d2902f711817b69` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `18e518e1855c53d2f0bf9d6260a287ea05ac5c2624dfe91fb942fca25e506fbc` |

Matching full corresponding source SHA-256:
`d916d3d0cba018736a0a8202270b373f5f2d9afbbf32b9b58c9bb190a3bba4ba`.
The planned durable `v1.0.1-source` release is still NOT CREATED, not an available
public source download. The final public notes must link verified durable source
no later than binary publication and disclose any owner-approved deferred tests.

## Previous candidate download hashes not for the current build

These are verified files from run 37195536400 for b7fce05, retained as historical
evidence. They do not contain the new legacy-bank fix and must not be used as its
final release hashes. No public upload has occurred. New package verification
is in the current report above; owner host acceptance and publication remain open.

| File | SHA-256 |
| --- | --- |
| `VDX7-1.0.1-Windows-x64-Setup.exe` | `2001a7f494580e45db706b50c1a9342991dcc3d90868ad1ad8ec9b179de948a0` |
| `VDX7-1.0.1-Windows-x64-Manual.zip` | `31471ec4e7fd61e76fef1da5c1c948e37f4f1d25eb2fe70eab9aee8b8626efac` |
| `VDX7-1.0.1-macOS-universal.pkg` | `97ea4a9ebcba4166d2677f69e668d163e0b3b9f5a12c497c1853bf6613da2791` |
| `VDX7-1.0.1-macOS-universal-Manual.zip` | `a2f17e467b6e8ff5901b3a311be9faadf4dc394ee0e982b097a8d76ddb4c6e39` |

Separate full corresponding source SHA-256:
`8d4da112f4565c49ab9128cb84bf5e4527470aa3486492397cfcb11b93ff8784`.
The planned durable source-release location is
`https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source`;
it is not yet created or verified and must not be presented as an available download.
