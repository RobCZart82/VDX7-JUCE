# Four public user downloads / Négy felhasználói letöltés

Current / Aktuális: [v1.2.0 stable Latest](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.2.0),
published / megjelent 2026-10-14. Four listed VST3 formats are now public;
matching source is in both Manual ZIPs. The older 1.0.1 checkpoints below
are historical. A négy VST3 csomag elérhető; a forrás mindkét Manual ZIP-ben,
az alábbi 1.0.1 checkpointok történetiek.

> **2026-10-05 publication checkpoint:** [1.0.1 is published](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1), with [matching source](https://github.com/RobCZart82/VDX7-JUCE/releases/tag/v1.0.1-source).
> Az 1.0.1 megjelent. [Current acceptance, owner-authorized deferrals and publication record](../validation/VALIDATION_20261005_PUBLICATION_101.md) supersede earlier open publication gates below. Earlier candidate/test statements are retained as dated preparation history, not current release status.

Owner decision, 2026-10-01: future releases starting with the planned 1.0.1
have exactly these four manually uploaded user-download assets (`<version>`
is the accepted stable version):

| Platform | Download | Name |
| --- | --- | --- |
| Windows x64 | EXE installer | `VDX7-<version>-Windows-x64-Setup.exe` |
| Windows x64 | Manual Install ZIP | `VDX7-<version>-Windows-x64-Manual.zip` |
| macOS Universal (arm64 + x86_64) | PKG installer | `VDX7-<version>-macOS-universal.pkg` |
| macOS Universal (arm64 + x86_64) | Manual Install ZIP | `VDX7-<version>-macOS-universal-Manual.zip` |

For the published v1.2.0 release, all four contain the accepted VST3 build. No AU
or Standalone download is added. Each v1.2.0 Manual ZIP contains the complete
plugin bundle plus the matching corresponding-source archive and its checksum
manifest under `Source/`; it is not an outer Actions artifact ZIP. The EXE and
PKG installers remain binary-only. This source-in-Manual-ZIP policy does not
describe the already-published v1.0.1 archives; their source remains in the
separate historical `v1.0.1-source` release below.
Unsigned Windows installer and unsigned/not-notarized macOS package warnings
remain; macOS plugin ad-hoc signing is not Developer ID signing.

## Packaging and publication boundary — standing safeguards

For v1.2.0 these checks and owner authorization were completed, and the four
unchanged final artifacts are public. The preparation workflow itself remains
non-publishing; publication was a separate authorized operation. Requirements
below apply to future publication operations, not outstanding v1.2.0 tasks.

The v1.2.0 candidate workflow is preparation-only. It verifies both installers,
both Manual ZIPs (each with the matching source ZIP and its checksum inside),
two BUILD-INFO files and SHA256SUMS before uploading a combined validation
artifact. It does not stage or publish a GitHub Release. The older
`scripts/stage_release_downloads.py` allowlist remains a safeguard for its
original workflow; actual v1.2.0 acceptance is recorded in
[the final owner record](../validation/FINAL_120_OWNER_ACCEPTANCE_20261014.md).

Any future publication implementation must use exactly this four-file allowlist
from the reviewed staged payload, never `release-assets/*`, all ZIPs, the outer
Actions ZIP or the complete validation artifact. Recheck the final four hashes
and approval before upload. No SHA256SUMS, BUILD-INFO or separate
corresponding-source ZIP is added as a fifth manually uploaded user-download
asset; the source archive travels inside each Manual ZIP.

Release notes must include the four SHA-256 values, accepted source/tooling
identities, the nested source archive's SHA-256 and location, dependency
identities, install/manual guidance, unsigned/not-notarized status and explicit
deferred-test disclosures. Corresponding source generation and verification
are retained, not removed by this policy.
Durable matching-source access must be reviewed before publication; an expiring
private/Actions artifact alone must not be treated as that access. For published
v1.2.0, matching source is durably delivered in both public Manual ZIPs and
final owner acceptance was obtained separately from the delivery decision.

Historical 1.0.1 source-delivery decision, 2026-10-01: the published
`v1.0.1-source` GitHub release contains the matching corresponding-source ZIP
and checksum manifest. This historical 1.0.1 release remains unchanged.

Historical delivery decision for v1.2.0, 2026-10-13: include the exact matching
corresponding-source ZIP and its checksum manifest inside `Source/` in both
Manual Install ZIPs. Keep the EXE/PKG installers binary-only and preserve the
four public download assets; do not publish a separate fifth source download.
The internal source archive must still be verified against the exact source
commit and toolchain identities before publication. That delivery decision
alone was not authorization; separate explicit authorization was subsequently
given for the published v1.2.0. Older releases/tags/assets remain unchanged.

The historical 1.0.1 non-publishing workflow staged a separate
`Source-Downloads` artifact. The v1.2.0 candidate workflow instead places the
source ZIP and its checksum inside each platform Manual ZIP, then verifies both
embedded copies match. For v1.2.0 the final native workflow, verification,
owner acceptance and authorized publication are complete.

GitHub may additionally display its automatically generated **Source code
(zip/tar.gz)** links. Those are platform-generated source links, not extra
manually uploaded user-download assets or offline dependency bundles.

## Historical 1.0.1 scope and acceptance

Published v1.0.0 is untouched. The coordinated version update targets
1.0.1-labelled preparation assets; this is not final binary acceptance.
Do not dispatch this workflow to replace existing 1.0.0 assets. The exact A/B
packaging approval for export-fixed A=B
`96267f7304e657821ce35c54536689981f41ef27` is committed through #130 at C
`edcb7471bf2ee08d3f8430e317cb9d17274f7576`; #120/#126 are historical pairs.
Null or mismatched approval still fails closed. Read-only workflow permissions
and separate publication authorization remain. See the
[current final test report](../validation/VALIDATION_20261005_FINAL_EXPORT_FIX_PACKAGE_101.md).
Use the [single execution plan](../development/DEVELOPMENT_PLAN.md) for remaining work.

Magyarul: a v1.2.0 release négy felhasználói csomagból áll. A megfelelő
forrásarchívum és saját SHA-256 jegyzéke mindkét Manual Install ZIP-en belül, a
`Source/` mappájában elérhető; külön ötödik forrásletöltés nincs. Az EXE/PKG
telepítők változatlanul csak a plugint telepítik. Az 1.0.1 külön forrás-release
történeti tény, nem módosul. A GitHub automatikus forráslinkjei továbbra sem
helyettesítik a teljes, függőségekkel együtt csomagolt forrást. A történeti
csomagtartalmi döntést külön publikálási engedély követte; az 1.2.0 megjelent,
a korábbi kiadások változatlanok.
