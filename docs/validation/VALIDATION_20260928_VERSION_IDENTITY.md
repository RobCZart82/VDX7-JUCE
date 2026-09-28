# Version identity and exact-candidate label validation — 2026-09-28

## Scope

Local validation of the release-preparation work based on main `817987b`
(`PR #90`). The working-tree change introduces explicit `rcN` display identity
and updates the non-publishing exact-candidate workflow. This is not validation
of a frozen RC, not host acceptance, and not authorization to publish.

## Implementation

- Existing default remains `1.0.0-dev` (`VDX7_RELEASE_BUILD=OFF`, no RC label).
- Exact candidate mode uses `VDX7_RELEASE_CANDIDATE=rcN` while stable mode is
  OFF; the UI displays `1.0.0-rcN`.
- Stable mode requires `VDX7_RELEASE_BUILD=ON` and an empty RC label; UI displays
  `1.0.0`.
- Malformed RC labels and simultaneous stable+RC settings are rejected.
- The manual candidate workflow requires the exact 40-character source SHA and
  RC label; its artifacts include the version label, platform and full SHA. It
  has no publication, tag or release-asset permissions.

## Results

- CMake unit script: dev, `rc1`, `rc12`, stable, malformed-label rejection and
  stable/RC mutual-exclusion rejection — **PASS**.
- Clean local CMake configuration with `VDX7_RELEASE_CANDIDATE=rc1` — **PASS**.
- macOS arm64 Release VST3 build in RC mode — **PASS**. Binary strings include
  both `VDX7 Mk I     v1.0.0-rc1` and `Version 1.0.0-rc1`.
- Reconfigure and macOS arm64 Release VST3 build with stable mode — **PASS**.
  Binary strings include `VDX7 Mk I     v1.0.0` and `Version 1.0.0`.
- Stable-mode ROM-free CTest suite, including the version-identity regression:
  **11/11 PASS**.
- Release-candidate workflow YAML parsed successfully; `git diff --check` passed.

## CI registration inventory follow-up

The first PR #91 Windows and macOS runs confirmed their product builds, ROM-free
tests and packaging tests passed, but the registration checker rejected the new
test as unexpected: its hard-coded inventory still expected the prior 10-test
baseline. This is a test inventory contract failure, not a product/build failure.
The checker now includes `vdx7_version_identity`, and the documented expected
inventories are updated to 37 ROM-on / 11 ROM-off. Local full inventory
verification passed for both ROM-off and placeholder-ROM registration modes;
the placeholder is used only for configuration and no firmware test executes.
Both platform CI jobs must re-run on the fix before considering merge.

The first CTest invocation occurred before test executables were built and
therefore reported them as Not Run. This was corrected by building
`vdx7_ci_checks`; the complete final 11-test suite then passed. The initial
Not Run result is not counted as a test failure or as evidence of execution.

## Not run / boundaries

- No Windows build of this branch yet; remote branch CI is required.
- No exact-candidate workflow dispatch or archive/binary package review yet.
- No private-ROM runtime suite, DAW/Standalone interaction, offline render or
  frozen-candidate host matrix was run in this round.
- These local binaries were temporary verification outputs, not release assets;
  no plugin was installed, no tag was created and nothing was published.
