# Reviewed release approval and provenance

This is the procedure for AUDIT-20260930-A4, not a second development plan.
The [execution plan](EXECUTION_PLAN_1.0.md) remains the active ledger.
The guard creates validation artifacts only; it cannot publish a tag, Release
or assets. Published 1.0.0 is unchanged. The owner requests work toward 1.0.1
publication. On 2026-10-04, after the exact next gate was identified in the
handoff, the owner authorized continuing final test packaging for product A
and packager B at `dbad14a2ef8307e565675893b6b3b837cfab9b02`.
This is not final binary/host acceptance or permission to publish assets.

## Current exact pair and policy review

The legacy-bank and bank-folder changes are now merged at dbad14a. Its
preparation-only packages and source passed independent verification; see the
[new preparation report](../validation/VALIDATION_20261004_BANK_FOLDER_PACKAGE_PREP_101.md).
This change records the new exact pair separately from the older b7fce05
approval. The policy PR and post-merge main checks must pass before accepted-mode
final test packaging. The resulting C is recorded after the protected merge;
it is not the frozen source/packager A=B.

The committed `docs/release/RELEASE_APPROVAL.json` currently contains:

```json
{
  "schema": 1,
  "approved_release": {
    "package_label": "1.0.1",
    "source_commit": "dbad14a2ef8307e565675893b6b3b837cfab9b02",
    "packager_commit": "dbad14a2ef8307e565675893b6b3b837cfab9b02",
    "workflow_ref": "RobCZart82/VDX7-JUCE/.github/workflows/prepare-stable-package.yml@refs/heads/main"
  }
}
```

The prior record for A=B b7fce05 was committed in
`fa28fcc05107ca059c8119f364b97ed13d0c2f24`
and merged through #120 at `828320d78e5d2e193356af0485a15c12f0279824` (C).
Final-head Windows/macOS/ASan-UBSan and post-merge main Windows/macOS PASS.
Accepted-mode workflow `37195536400` was dispatched from that C on 2026-10-04;
authorization, both platform jobs and assembly PASS. Independent final hash,
source and payload verification PASS; see the
[final package report](../validation/VALIDATION_20261004_FINAL_PACKAGE_101.md).
Those results belong to the old pair only, not the new bank-folder build.
The owner will install and test the new exact final packages in REAPER;
no installed plugin is changed here.
An input flag, a dirty local edit or an untracked policy file cannot supply approval.
Null approval remains fail-closed; it is the former default, not the current record.

## Separate source, tooling and approval commits

Use three identities. The product source is A; the frozen release packager
is B; the later reviewed policy/workflow commit is C. A and B must be real
ancestors of C, and the approval checkout must be exactly C. B must differ
from C: a policy cannot contain the SHA of its own future commit. The running
packager script must match the script committed at B.

1. Freeze and validate the intended product source A. Record its full SHA.
2. Freeze the packaging tools B and validate their tests, workflow and payload
   boundaries. Record the full SHA; identical script bytes at another commit
   are not a substitute for the reviewed B identity.
3. After reviewing the exact A/B pair, commit the approval record separately
   and merge it through protected main. Record the resulting C identity.
4. Run the canonical stable workflow from main with A and accepted mode. It
   reads the policy at C, resolves the reviewed B and validates the actual
   GitHub workflow/ref context before starting platform jobs. Those jobs use
   only validated outputs and recheck their exact checkouts.
5. Review the resulting binaries, installer behavior, source archive,
   checksums, license notices and deferred risks. Separately obtain publication
   authorization. Accepted metadata does not perform or authorize publication.

The approved record has exactly these fields, with full lowercase 40-character
commit SHAs substituted for the placeholders:

```json
{
  "schema": 1,
  "approved_release": {
    "package_label": "1.0.1",
    "source_commit": "<reviewed-product-SHA>",
    "packager_commit": "<frozen-packager-SHA>",
    "workflow_ref": "RobCZart82/VDX7-JUCE/.github/workflows/prepare-stable-package.yml@refs/heads/main"
  }
}
```

This is an illustrative shape, not a usable approval. The coordinated update
targets 1.0.1-labelled stable preparation. Before platform work, the guard
checks the immutable source's project and Windows installer versions against
1.0.1. Do not dispatch it to regenerate or replace the published 1.0.0 release.
Review the exact candidate and freeze the packager before committing approval.
Missing, malformed, duplicate-key, null or mismatched records are rejected.

## Trust boundary

Accepted mode requires the canonical repository/workflow and
`refs/heads/main`. The policy is read from immutable Git objects, not the
working tree. Commit and ancestry checks ignore local Git replacement refs.
Preparation mode may use a matching branch/tag or fork workflow context;
that does not grant accepted status.

The authority is the reviewed policy on protected main and the canonical
workflow passing its actual GitHub context. A person invoking the local CLI
can type context strings: those arguments are not an authenticated identity.
Repository protection, review and final human approval are still necessary.
The workflow retains `contents: read` and performs no release publication.

## Verification and compatibility

New accepted archives record the source, packager, approval commit, workflow
context and SHA-256 of the exact policy bytes. The bundled standalone checker
can verify the archive without Git, a checkout or network access. It validates
the manifest's internal binding and file integrity, not a digital signature
or independent proof that the human approval occurred.

Historical schema-1 archives without the new approval fields remain
integrity-verifiable. Their legacy accepted flag is explicitly reported as
unvalidated approval metadata; it must not be treated as newly certified.
Neither mode changes the product snapshot or repairs the already published
1.0.0 archive/checker. See [source packaging](SOURCE_PACKAGING_1.0.md).

Toolchain pinning/provenance (A6), installer migration (A5), final asset
reconciliation (A7) and exact-candidate acceptance remain separate work.
This approval guard does not make compiled installers bit-reproducible.

Magyarul: az elfogadott csomaghoz egy külön commitban jóváhagyott, pontos
forrás–csomagoló páros és a main hivatalos munkafolyamata szükséges. A párost
a tulajdonos az új dbad14a párosra 2026-10-04-én engedélyezte a végső
tesztcsomagolás folytatását; a védett rekord/main ellenőrzése, a kézi REAPER-próba
és a publikálási engedély még külön lezárandó. Az ellenőrzőösszeg
épséget igazol, nem kiadói hitelesítést; publikálni továbbra is csak külön
engedéllyel lehet.
