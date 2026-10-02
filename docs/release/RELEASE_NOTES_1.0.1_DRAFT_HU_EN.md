# VDX7 Mk1. 1.0.1 — draft / tervezet

Not published or accepted. This records the corrective candidate; it does not
replace the [single execution plan](EXECUTION_PLAN_1.0.md) or grant publication
approval. Published v1.0.0 and its assets remain unchanged.

Candidate product/tooling source: `6cc8cda30e9e66d3ab97699bcb9d2014a78b8b14`.
Preparation: [36998278475](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36998278475),
acceptance disabled, all jobs PASS. Hosted Windows install/upgrade/uninstall,
macOS payload/signature guards, source verification and staged checksums PASS.
Independent download/hash, provenance, source and macOS payload review now
passes for that preparation artifact; see the
[2026-10-02 review](../validation/VALIDATION_20261002_RELEASE_REVIEW.md).
Final archive offline-build and host acceptance remain open.
Final acceptance and hashes must be reconciled before turning this
draft into public notes. No 1.0.1 host acceptance is inferred
from the owner's earlier 1.0.0 REAPER tests.

## English

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

- Reconcile all four final binary hashes and installer/upgrade evidence.
- Verify complete matching source and its checksum; provide the approved
  separate durable `v1.0.1-source` release link before/no later than binaries.
- Record exact reviewed source/tooling/approval tuple and explicit deferred
  host/platform checks. Preparation artifacts alone are not accepted assets.
- Obtain separate authorization for tags, source/product releases and uploads;
  preserve the four-download policy and do not modify v1.0.0.
