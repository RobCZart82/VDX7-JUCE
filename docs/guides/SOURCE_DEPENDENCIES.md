# Corresponding source dependencies

Current automated creation, hash verification and extracted-source check:
[reproducible source packaging](../release/SOURCE_PACKAGING_1.0.md).

## Current development provenance (2026-10-09)

The current development candidate pins JUCE 9.0.3 at the revision below.
This is not a published release or new host acceptance. The current CMake configure
emits `VDX7DependencySources.cmake` in the build directory with the resolved
wrapper/JUCE/core source paths, including FetchContent overrides and vendored
source selection. The current `scripts/write_build_provenance.py` requires
those paths (and the cache's source directory) to match its supplied checkouts
before the existing pinned Git identity/cleanliness checks. Missing evidence
requires reconfiguration, not assuming that a default `_deps` path was used.

For an older product checkout with the newer tooling, configure with
`-DCMAKE_PROJECT_VDX7_JUCE_INCLUDE=/absolute/path/to/approval-tools/cmake/VDX7DependencySources.cmake`
from the exact reviewed workflow/approval checkout. The packaging workflow
passes this on both platforms, without changing the historical product or
its frozen packager. An older packager's own checker remains historical;
its output does not become retroactively verified by the new guard.

This observes configuration, not signing or a complete fresh rebuild. The
provenance CLI uses Git dependency checkouts; offline corresponding-source
archives still require their separate manifest/file-hash verification.
[Validation and JUCE update gates](../validation/BUILD_PROVENANCE_SOURCE_BINDING_20261009.md).
The separate [9.0.3 compatibility experiment](../validation/JUCE_903_COMPATIBILITY_20261009.md)
records the current candidate, its tests and the still-unrun host gates.

## Current development pins and offline build

The wrapper source is the exact development commit recorded with the package,
not a historical tag. The published 1.0.1 retains JUCE 9.0.1 at
e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8; old v0.6.6 descriptions are historical.

- JUCE repository: https://github.com/juce-framework/JUCE
  Revision: be29c81492b6151c8ea8d14c840e1311963b3a83 (JUCE 9.0.3).
- dx7Lib repository: https://github.com/reales/retromulator
  Revision: d5473776a0449d60a997b91bdc888598a33265ac.
  Included source subset: source/dx7Lib, compiled HD6303R.cpp,
  HD6303R_inst.cpp and dx7.cpp plus headers.
- Wrapper/resources for 1.0 development/RC packages: the exact 40-character
  source commit recorded in the package manifest. Historical v0.6.6 packages
  used the v0.6.6 tag; that tag does not identify current development sources.

The corresponding-source ZIP places dependencies at third_party/JUCE and
third_party/dx7Lib. CMake detects these folders automatically and does not
fetch dependency repositories when they are present. Their license notices
are retained. OS SDKs, compiler and CMake are system prerequisites.

macOS Apple Silicon offline build from the extracted source root:

```sh
cmake -S . -B build-offline -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-offline --config Release --target VDX7_VST3
```

With Ninja instead of Xcode, additionally use -DCMAKE_BUILD_TYPE=Release.
The existing stable release does not use the new development JUCE pin. No firmware is needed
to compile; running the instrument requires the user's external firmware.

Magyar: a teljes forrás ZIP tartalmazza a rögzített JUCE- és dx7Lib-forrást,
amelyet a CMake automatikusan felismer. A fenti fordítás nem tölt le
függőségeket; a rendszer SDK-ja, fordítója és a CMake külön előfeltétel.
Fordításhoz ROM nem szükséges, a hangszer használatához saját firmware kell.
