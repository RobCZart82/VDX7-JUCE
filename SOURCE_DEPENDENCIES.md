# v0.6.6 Pre-Beta 2 corresponding source

- JUCE repository: https://github.com/juce-framework/JUCE
  Revision: e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8 (local JUCE 9.0.1 checkout).
- dx7Lib repository: https://github.com/reales/retromulator
  Revision: d5473776a0449d60a997b91bdc888598a33265ac.
  Included source subset: source/dx7Lib, compiled HD6303R.cpp,
  HD6303R_inst.cpp and dx7.cpp plus headers.
- Wrapper/resources: the release tag v0.6.6.

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
The tested release uses the same dependency revisions. No firmware is needed
to compile; running the instrument requires the user's external firmware.

Magyar: a teljes forrás ZIP tartalmazza a rögzített JUCE- és dx7Lib-forrást,
amelyet a CMake automatikusan felismer. A fenti fordítás nem tölt le
függőségeket; a rendszer SDK-ja, fordítója és a CMake külön előfeltétel.
Fordításhoz ROM nem szükséges, a hangszer használatához saját firmware kell.
