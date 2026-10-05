# Documentation / Dokumentáció

## Start here / Kezdőpontok

- [Corrected repository review and owner Windows feedback (2026-10-05)](validation/AUDIT_REPORT_20261005_CODE_AND_REPOSITORY_REVIEW.md)

- [English user and build guide](guides/GUIDE_EN.md)
- [Magyar használati és fordítási útmutató](guides/GUIDE_HU.md)
- [Current 1.0 execution plan](release/EXECUTION_PLAN_1.0.md)
- [Release-candidate checklist](release/RELEASE_CHECKLIST_1.0_RC.md)
- [Roadmap and milestone history](release/ROADMAP_1.0.md)
- [1.0 build identity and package boundaries](release/IDENTITY_AND_PACKAGE_1.0.md)
- [Reproducible source packaging](release/SOURCE_PACKAGING_1.0.md)
- [Reviewed release approval and provenance](release/RELEASE_APPROVAL.md)
- [Observed installer build toolchain](release/BUILD_TOOLCHAIN_PROVENANCE.md)
- [Four public user downloads](release/PUBLIC_DOWNLOADS.md)
- [Candidate instructions / jelölt útmutató](release/CANDIDATE_README_HU_EN.md)
- [Draft 1.0.0 release notes and test matrix / kiadási jegyzet és tesztmátrix](release/RELEASE_NOTES_1.0.0_DRAFT_HU_EN.md)
- [Current draft 1.0.1 release notes](release/RELEASE_NOTES_1.0.1_DRAFT_HU_EN.md)
- [Current runtime GUI resources](design/GUI_RUNTIME_ASSETS.md)

## Document groups

| Directory | Purpose |
| --- | --- |
| [guides](guides/) | Usage, installation, building and pinned dependencies |
| [release](release/) | Active plan, roadmap and exact-candidate acceptance |
| [validation](validation/) | Retained regression and test evidence; dates/SHAs limit each claim |
| [design](design/) | MIDI-range and MONO policy decisions |
| [archive](archive/README.md) | Superseded audits, handoffs and old release documents |
| [screenshots](screenshots/) | Owner-approved current GUI screenshots |

Asset-specific design specifications remain beside the resources in
[Resources/GUI](../Resources/GUI/). Licences and agent instructions stay at the
repository root. No firmware belongs in these directories.

New validation reports belong in `docs/validation/`, not at the repository root.
The [v0.6.6 validation summary](validation/VALIDATION.txt) and
[packed voice range validation](validation/VALIDATION_1.0_PACKED_VOICE_RANGES.md)
were moved here on 2026-09-28; their original results and status wording are
preserved as historical evidence, not fresh acceptance claims.

## History and evidence

- [Current export corrected 1.0.1 final test packages](validation/VALIDATION_20261005_FINAL_EXPORT_FIX_PACKAGE_101.md)
- [Export corrected 1.0.1 package preparation checkpoint](validation/VALIDATION_20261005_EXPORT_FIX_PACKAGE_PREP_101.md)
- [SysEx export acknowledgement and bank library regression](validation/VALIDATION_20261005_SYSEX_EXPORT_ACKNOWLEDGEMENT.md)
- [Archived DX7 bank acceptance and data preservation](validation/VALIDATION_20261004_LEGACY_BANK_COMPATIBILITY.md)

Old test reports are not deleted or automatically marked obsolete: they may
explain a regression and its fix, but do not establish PASS on a new candidate.
Historical plans do not override the current execution plan or reopen the
owner-approved GUI design. Paths written in backticks generally refer to the
repository root; shell commands are run from that root unless stated otherwise.

The September 2026 reorganisation preserves the document bodies and Git history;
relative links and packaging paths are updated. Old external links to root-level
documents can be located via the [path migration index](DOCUMENT_PATHS.md).

Magyarul: a részletes tesztbizonyítékok megmaradnak, a régi tervek az archívumba
kerültek. Az aktuális teendőket az egységes fejlesztési terv határozza meg;
a korábbi PASS eredmények csak a jelentésben megadott verzióra és körre érvényesek.
