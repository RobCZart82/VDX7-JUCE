# Observed installer build toolchain

This describes A6's packaging evidence, not a second development plan.
Use the [single execution plan](EXECUTION_PLAN_1.0.md) for completion status.

The stable packaging workflow selects Chocolatey Inno Setup **6.7.1** explicitly
and verifies ISCC's product-version components before compiling the installer.
A failed package installation or mismatched compiler version stops the job.
This is a deliberate 6.x pin, not a claim to select the newest Inno release.
References: [Inno 6 revision history](https://jrsoftware.org/files/is6-whatsnew.htm),
[Chocolatey package](https://community.chocolatey.org/packages/innosetup/6.7.1).
The exact ISCC executable SHA-256 is recorded as observed evidence; it is not a
preapproved executable digest or a substitute for package-source authentication.

`scripts/write_build_provenance.py` reads the configured build's generated
compiler metadata and CMake cache rather than guessing the compiler from PATH.
For Visual Studio it reads the SDK actually selected in `VDX7.vcxproj`; for
Xcode it records the selected SDK version, Xcode build, deployment target and
both configured Universal architectures. Ambiguous/missing compiler or SDK
metadata, missing runner evidence and wrong platform/architecture fail closed.
JUCE/Retromulator HEADs must match the product CMake pins, with no tracked edits.

The whitelist of recorded runner fields is OS, architecture, image name/version
and workflow run/attempt. No environment dump, credentials, private firmware,
user paths or build directories are copied into release assets. The collector
does not execute the generated CMake metadata. No output is written until all
required observations pass; failure preserves an existing evidence file.

The resulting text is appended to each platform's existing BUILD-INFO file
before artifact upload and final SHA256SUMS generation. The seven-asset
inventory is unchanged. Existing source/packager/approval SHAs, read-only
permissions, unsigned/notarization warnings and the no-publication boundary
are retained. The source archive's dependency identities remain separate.

This records what was observed, not an attestation of all installed tools:
compiler/CMake/SDK and hosted runner images are recorded, not independently
frozen. Chocolatey and Python/runtime transitive components are not fully
locked. A deterministic source ZIP does not prove bit-identical VST3/EXE/pkg
output. Required final-candidate payload, license, checksum, installer migration
and host acceptance remain separate. The null release approval stays null.

The workflow still uses 1.0.0 package labels. Do not dispatch it to regenerate
or replace published 1.0.0 assets. A separately reviewed coordinated 1.0.1
version/label update must precede the new candidate's packaging and acceptance.
