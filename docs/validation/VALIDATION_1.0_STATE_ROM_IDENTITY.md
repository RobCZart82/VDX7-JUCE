# Save state and ROM identity interleaving

Status: implementation and deterministic opt-in processor regression added;
ROM-backed execution is **NOT RUN** in this environment.

## Risk under investigation

`getStateInformation()` snapshots RAM and program selection while holding
`engineMutex_`, but the previous implementation read `romFile_` later under a
separate metadata lock. A successful ROM reload could occur between those
operations, potentially pairing the detached state from the old engine image
with the new image's path.

This was previously an unconfirmed processor-level candidate. Static inspection
shows the vulnerable ordering; runtime reproduction still requires the local
ROM-backed test below.

## Change

The successfully installed ROM path is now kept as engine-generation metadata
under `engineMutex_`. State capture copies that path in the same critical
section as RAM. XML/base64 encoding remains outside the engine lock. UI-facing
`romFile_` metadata continues to be published independently after the engine
installation.

The host-reset test executable has a deterministic scheduling gate that pauses
after capturing state from image A and before serialization completes. The test
then installs image B and asserts that the saved `romPath` still identifies A.
The two test paths contain the same supplied ROM bytes: this isolates image
generation/path pairing without editing or synthesizing firmware data.

## Local verification

Run with the validated v1.8 ROM and `VDX7_ENABLE_ROM_TESTS=ON`:

```text
vdx7_host_reset_tests <path-to-local-ROM> --state-rom-identity-only
```

The corresponding CTest is `vdx7_state_rom_identity`. It is intentionally
opt-in; CI must never fetch or package private ROM data.

Current run result: **NOT RUN** — CMake/CTest is unavailable in the current
environment. Do not treat this note or a green compile-only CI job as proof of
the runtime interleaving test. The test must pass on a recorded commit before
the candidate is called runtime-verified.
