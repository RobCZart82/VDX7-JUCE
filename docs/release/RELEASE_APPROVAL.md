# Reviewed release approval and provenance

This is the procedure for AUDIT-20260930-A4, not a second development plan.
The [execution plan](EXECUTION_PLAN_1.0.md) remains the active ledger.
The guard creates validation artifacts only; it cannot publish a tag, Release
or assets. Published 1.0.0 is unchanged. No 1.0.1 publication is authorized.

## Default state

The committed `docs/release/RELEASE_APPROVAL.json` currently contains:

```json
{
  "schema": 1,
  "approved_release": null
}
```

With this record, accepted packaging fails before platform work or archive
output. Non-accepted preparation remains available. An input flag, a dirty
local edit or an untracked policy file cannot supply approval.

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
    "package_label": "1.0.0",
    "source_commit": "<reviewed-product-SHA>",
    "packager_commit": "<frozen-packager-SHA>",
    "workflow_ref": "RobCZart82/VDX7-JUCE/.github/workflows/prepare-stable-package.yml@refs/heads/main"
  }
}
```

This is an illustrative shape, not a usable approval. The existing workflow
still labels stable artifacts 1.0.0. Do not dispatch it to regenerate or
replace the published release. A future 1.0.1 candidate needs a separately
reviewed version/asset-label update before its approval record can be used.
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
forrás–csomagoló páros és a main hivatalos munkafolyamata szükséges. A jelenlegi
üres jóváhagyási rekord nem enged új elfogadott csomagot. Az ellenőrzőösszeg
épséget igazol, nem kiadói hitelesítést; publikálni továbbra is csak külön
engedéllyel lehet.
