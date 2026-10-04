# Four public user downloads / Négy felhasználói letöltés

Owner decision, 2026-10-01: future releases starting with the planned 1.0.1
have exactly these four manually uploaded user-download assets (`<version>`
is the accepted stable version):

| Platform | Download | Name |
| --- | --- | --- |
| Windows x64 | EXE installer | `VDX7-<version>-Windows-x64-Setup.exe` |
| Windows x64 | Manual Install ZIP | `VDX7-<version>-Windows-x64-Manual.zip` |
| macOS Universal (arm64 + x86_64) | PKG installer | `VDX7-<version>-macOS-universal.pkg` |
| macOS Universal (arm64 + x86_64) | Manual Install ZIP | `VDX7-<version>-macOS-universal-Manual.zip` |

All four contain the accepted VST3 build. No AU or Standalone download is added.
Manual ZIPs contain the complete plugin bundle, not an outer Actions artifact
ZIP. Unsigned Windows installer and unsigned/not-notarized macOS package
warnings remain; macOS plugin ad-hoc signing is not Developer ID signing.

## Packaging and publication boundary

The stable workflow still verifies and retains the complete internal
validation artifact: both installers, both manual ZIPs, corresponding-source
ZIP, two BUILD-INFO files and SHA256SUMS. It then calls
`scripts/stage_release_downloads.py` using the guarded approval-tools checkout.
The collector checks the four expected basenames and SHA-256 values before
copying into a new `publishable-downloads` directory. Existing output is never
overwritten. Its separate `Four-Downloads` Actions artifact contains only those
four files. Neither artifact nor the script publishes a Release.

Any future publication implementation must use exactly this four-file allowlist
from the reviewed staged payload, never `release-assets/*`, all ZIPs, the outer
Actions ZIP or the complete validation artifact. Recheck the final four hashes
and approval before upload. No SHA256SUMS, BUILD-INFO or corresponding-source
ZIP is added as a fifth manually uploaded user-download asset.

Release notes must include the four SHA-256 values, accepted source/tooling
identities, matching source/dependency access links, install/manual guidance,
unsigned/not-notarized status and explicit deferred-test disclosures. Corresponding
source generation and verification are retained, not removed by this policy.
Durable matching-source access must be reviewed before publication; an expiring
private/Actions artifact alone must not be treated as that access. Final
candidate acceptance remains separate from the owner's delivery decision.

Owner source-delivery decision, 2026-10-01: use a separate public
`v1.0.1-source` GitHub release in this repository. It contains the complete
matching corresponding-source ZIP and its checksum manifest; it is not the
latest product release. The main `v1.0.1` notes link directly to this durable
source download and record its source/tooling identities and SHA-256.
Do not include source in either Manual Install ZIP. The main product release
keeps exactly four manually uploaded user downloads and remains the latest.
This selects the delivery mechanism, not the final candidate or approval tuple.
Publish verified matching source no later than the corresponding binaries;
an expired Actions URL or GitHub's dependency-free automatic source ZIP is
not a substitute.

The non-publishing workflow also stages a separate `Source-Downloads`
artifact with exactly the matching source ZIP and its one-file SHA256SUMS
manifest. It verifies the complete source archive against guarded source/tool
identities and acceptance status; accepted mode additionally requires the
exact approval commit proof. This is the future source-release payload,
not an extra asset for the four-download main release. Final native workflow
execution and publication review remain required.

GitHub may additionally display its automatically generated **Source code
(zip/tar.gz)** links. Those are platform-generated source links, not extra
manually uploaded user-download assets or offline dependency bundles.

## Current scope and acceptance

Published v1.0.0 is untouched. The coordinated version update targets
1.0.1-labelled preparation assets; this is not final binary acceptance.
Do not dispatch this workflow to replace existing 1.0.0 assets. The exact A/B
packaging approval is now committed through #120; null approval still fails
closed. Read-only workflow permissions and separate publication authorization remain.
Use the [single execution plan](EXECUTION_PLAN_1.0.md) for remaining work.

Magyarul: a release-re négy felhasználói csomag kerül fel. A forráscsomag,
buildleírások és hashfájl továbbra is elkészülnek, de külön validációs anyagok.
A hashértékek és tartós, verzióhoz illeszkedő forráslinkek a release leírásába
kerülnek. A GitHub automatikus forráslinkjei nem kézzel feltöltött további
csomagok. Ez nem publikálási engedély és nem módosít meglévő kiadást.
