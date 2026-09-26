# Bounded ROM and SysEx file reads

## Finding

The file-import helper previously used `File::loadFileAsData()` before the
format decoder could reject the input. A malformed or unexpectedly large ROM
or SysEx file could therefore request an unbounded allocation even though the
accepted formats have small, fixed maximum sizes. This was an input-hardening
finding; no crash or memory-exhaustion incident was reproduced.

## Change

`VDX7BoundedFile::read` checks the opened stream's length before allocating,
reads no more than the caller-provided format limit, and probes for an extra
byte so a file that grows during the read is rejected. Failed reads clear the
output. The processor applies limits of 48 KiB for combined firmware images,
32 KiB for the optional factory-voice companion, and 4104 bytes for a bank
SysEx file (single-voice SysEx is smaller and remains validated by the decoder).

## Regression coverage

The ROM-free `vdx7_bounded_file` test covers a file exactly at its limit, one
byte over the limit, a missing file, and clearing of prior output on failure.
CI compiles this test through `vdx7_ci_checks` and registers it with CTest.

Local execution: **NOT RUN** — CMake is not installed in this environment.
The fix does not claim ROM-backed, DAW, or memory-pressure acceptance. Await
the macOS and Windows CI results for this source revision.
