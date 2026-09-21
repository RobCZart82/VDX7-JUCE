# RC checks and current documentation (2026-09-21)

Base: main 20faa3c63b53440731dbce4a383ef82ea35b40a4 (merged PR27).

The RC workflow previously built only three of the five registered ROM-free
test executables. USER-bank and resampling executables were missing on a fresh
runner. Normal macOS/Windows CI and RC now use one `vdx7_ci_checks` dependency
target, including all five plus compile-only stress coverage. `vdx7_all_tests`
also builds every registered local-ROM runner when enabled. CTest fails for an
empty test set. Candidate macOS builds are Universal, with an explicit lipo
architecture check before ad-hoc signing. No workflow dispatch or release here.

Local existing Ninja/arm64 build: VST3 and vdx7_all_tests succeeded.
Full local CTest: 10/10 passed, 59.69 seconds, including five local-ROM tests.
User ROM remained outside the repository. No audio-engine or GUI code changed.

A separate fresh build configuration was attempted but stopped at the compiler
probe because /usr/bin/cc requires Xcode license acceptance. No license was
accepted on the user's behalf. Consequently this local run does not establish
fresh-build, Windows, Universal or exact-commit RC execution success; GitHub PR
CI must pass, and the RC workflow still needs later exact-commit acceptance.

HU/EN READMEs describe current USER/Performance/Settings/SRC functions and build
targets. Historical screenshots and initial development notes are labelled.
Audit risks and requested GUI adjustments are tracked separately in the roadmap.
No installed plugin replacement, version promotion, tag or publication.
