# VDX7 Mk1. 1.0.1 — draft / tervezet

Not published or host-accepted. This records the corrective candidate; it does not
replace the [single execution plan](EXECUTION_PLAN_1.0.md) or grant publication
approval. Published v1.0.0 and its assets remain unchanged.

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

- All four final binary hashes and installer/upgrade evidence are verified;
  complete matching source and checksum verified. Provide the approved
  separate durable `v1.0.1-source` release link before/no later than binaries.
- Exact reviewed source/tooling/approval tuple is recorded above. Complete
  owner final REAPER acceptance and explicit disposition of other unrun checks.
- Obtain separate authorization for tags, source/product releases and uploads;
  preserve the four-download policy and do not modify v1.0.0.

## Verified final download hashes

These are the intended final test/release files, not old preparation hashes.
No public upload has occurred; owner host acceptance and publication remain open.

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
