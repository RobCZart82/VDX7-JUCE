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
private/Actions artifact alone must not be treated as that access. This change
does not approve a replacement source-delivery mechanism or any new release.

GitHub may additionally display its automatically generated **Source code
(zip/tar.gz)** links. Those are platform-generated source links, not extra
manually uploaded user-download assets or offline dependency bundles.

## Current scope and acceptance

Published v1.0.0 is untouched. The workflow and guarded packager remain
1.0.0-labelled until the separately reviewed coherent 1.0.1 version update.
Do not dispatch this workflow to replace existing 1.0.0 assets. Null approval,
read-only workflow permissions and separate publication authorization remain.
Use the [single execution plan](EXECUTION_PLAN_1.0.md) for remaining work.

Magyarul: a release-re négy felhasználói csomag kerül fel. A forráscsomag,
buildleírások és hashfájl továbbra is elkészülnek, de külön validációs anyagok.
A hashértékek és tartós, verzióhoz illeszkedő forráslinkek a release leírásába
kerülnek. A GitHub automatikus forráslinkjei nem kézzel feltöltött további
csomagok. Ez nem publikálási engedély és nem módosít meglévő kiadást.
