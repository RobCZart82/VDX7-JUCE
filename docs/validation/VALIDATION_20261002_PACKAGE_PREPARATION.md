# Unaccepted 1.0.1 installer preparation — 2026-10-02

Exact product/tooling/approval-workflow SHA:
`6cc8cda30e9e66d3ab97699bcb9d2014a78b8b14` (merged #115).
Run [36998278475](https://github.com/RobCZart82/VDX7-JUCE/actions/runs/36998278475),
9m 37s, all four jobs PASS. `accepted_for_publication=false`,
`release_accepted=false`; committed approval remains null.

## Evidence inspected

| Check | Status / boundary |
| --- | --- |
| Final #115 Windows/macOS/ASan-UBSan; post-merge Windows/macOS | PASS |
| Local exact packaging code Python suite | 77 discovered, 76 PASS, one Windows symlink capability SKIP |
| Native Windows/macOS ROM-free CTest | PASS, 13/13 each |
| Windows Inno engine 6.7.1 guard and EXE creation | PASS, job `110809921227`; no numeric PE metadata dependency |
| Hosted Windows fresh install/uninstall | PASS |
| Hosted checksum-pinned published 1.0.0 -> 1.0.1 upgrade/uninstall | PASS; same AppId/payload checks and synthetic user-file preservation |
| macOS Universal/signature and PKG payload guards | PASS, job `110809921379`; ad-hoc only, no notarization claim |
| Seven-file asset SHA-256 verification | PASS, assembly job `110812533075`; both BUILD-INFO, source ZIP and four binary downloads |
| Exactly four user-download staging and two-file source staging | PASS, same assembly job |
| Source embedded-manifest verification | PASS, 5117 files on hosted runners |
| Independent local downloaded-payload/hash/provenance inspection | NOT RUN; browser download timed out, alternative UI click produced no identifiable local ZIP |
| Installed local REAPER / physical Intel Mac / subjective audio acceptance on these binaries | NOT RUN |
| Publication approval, new tags/releases/uploads | NOT RUN, not authorized by preparation |

Corresponding-source ZIP SHA-256 from the actual Windows generation log:
`37f62d48dac516ccc1e5e85fc5d460a0412309352ae3cf5d568268f5fc2e8fa9`.
This is the inner source ZIP digest, not an outer Actions artifact digest.

## Artifact identity (temporary, not public release assets)

- Complete validation: artifact `11222504465`, outer ZIP SHA-256
  `0eada5e0c73d06330ddd5433baf9f6ffc9cfb50d959fc0e726be26e4a0fc8b89`.
- Four downloads: artifact `11222698809`, outer ZIP SHA-256
  `540058178f9dc283c7dcc1b4a5c815b00dcee8c222e66483ad7174014a4aaafd`.
- Source downloads: artifact `11222783226`, outer ZIP SHA-256
  `ac32fc3d146072b04a2a67e0b14485f48a94fcde6e0fbcea70d4c68c0ca33563`.

Do not use outer ZIP digests as the four binaries' release-note hashes.
Read the inner `SHA256SUMS.txt` and independently reconcile payloads before
approval. Expiring Actions artifacts are not durable corresponding-source access.

## Remaining release boundary

Review the downloaded candidate payload/provenance, supply durable matching
source access using the selected separate source release, decide exact-candidate
host/platform acceptance or explicit deferrals, then obtain separate publication
authorization and an exact approval tuple. Do not publish these prep assets as
accepted merely because all jobs are green. Keep original invalid-ROM evidence
and legacy-project compatibility risk explicit; no ROM was uploaded or edited.

The run also reports Node 20 action-runtime deprecation and an upcoming
ubuntu-latest image migration. These are maintenance warnings, not failed
packaging tests; action/runner migration requires its own tested change.
