# Documentation / Dokumentáció

## Start here / Kezdőpontok

- [English user and build guide](guides/GUIDE_EN.md)
- [Magyar használati és fordítási útmutató](guides/GUIDE_HU.md)
- [Current 1.0 execution plan](release/EXECUTION_PLAN_1.0.md)
- [Release-candidate checklist](release/RELEASE_CHECKLIST_1.0_RC.md)
- [Roadmap and milestone history](release/ROADMAP_1.0.md)

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

## History and evidence

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
