# VDX7-JUCE repository review — corrected evidence summary

Date: 2026-10-05. Reviewed baseline: `63994e3c1fda900662685d8344cb7acba05ca7ef`.
This document corrects the owner-supplied Copilot overview. It is a bounded
documentation/source cross-check, not a new exhaustive runtime or security audit.
No new executable tests were run for this documentation round.

## Supported conclusions

The repository has separated engine, processor, GUI and validation components,
pinned dependencies and ROM-free public testing. Export acknowledgement fixes
and final package checks are documented with exact provenance in the
[final package report](VALIDATION_20261005_FINAL_EXPORT_FIX_PACKAGE_101.md).
Its recorded results include hosted 14/14 ROM-free CTests and 77/77 Python tests,
local 76 Python PASS plus one symlink-capability SKIPPED, offline source build,
package checks and Windows VST3 validator 47/47. These are different suites and
scopes, not interchangeable coverage counts. The previous 47/47 local source
CTest result is tied to PR #128, including private firmware coverage, not a new
execution on every subsequent documentation commit.

This review identified no new proven product defect. That statement does not
establish absence of defects, memory leaks or security vulnerabilities.

## Corrections to the supplied overview

| Original claim | Evidence-bounded correction |
| --- | --- |
| Deferred MIDI dynamically resizes and needs a memory pool | `Source/VDX7DeferredMidi.h` already uses fixed arrays: 256 events and 65,536 bytes, with overflow reconciliation. Remove this recommendation. |
| Allocation probe prevents all heap allocations | `Tests/VDX7AllocationProbe.h` counts ordinary C++ new/delete on the calling thread. It neither prevents allocation nor intercepts direct malloc/free or aligned allocation. |
| Allocation checks prove no memory leaks | They do not. Sanitizer results apply only to exercised cases and configurations. |
| 47 CTests plus 76 Python tests are 123 assertions | They are test registrations/cases from different scopes; individual tests contain multiple assertions. No coverage percentage is established. |
| Zero issues proves no blocking defects | Issue count is administrative state, not a defect-freedom proof. |
| Merge ancestry proves no force-push and all authorizations | Commit ancestry alone cannot prove repository history or human authorization. |
| Secure, production-ready, complete compliance | Replace categorical guarantees with scoped evidence and outstanding gates. No exhaustive security review was performed here. |

Do not change stable parameter IDs/order or add logging/pools merely because the
overview suggested them. Optimization requires measured need; callback logging
must not introduce blocking file I/O or allocation.

## Owner Windows and macOS acceptance updates

On 2026-10-05 the owner reported downloading the latest installer and that the
release-intended software works correctly on **Windows 11 x64 with REAPER**.
Status: **PASS — owner-reported general Windows/REAPER operation**.
This is human acceptance evidence, not an independently reproduced test.

The owner's supplied Windows About screenshot identifies Windows 11 Pro 26H2,
OS build 26300.9457, x64, Intel Core i5-8500T and 16 GB RAM. This is screenshot
evidence of the test environment, not evidence of individual plugin test cases.
Computer/domain names and device/product identifiers are intentionally omitted;
the original screenshot is not committed.

The owner subsequently identified **REAPER 7.82 x64** and downloaded bundle
`VDX7-1.0.1-Four-Downloads-96267f7304e657821ce35c54536689981f41ef27-accepted`.
This identifies the final accepted four-download bundle by its owner-reported
name; it is not an EXE filename or an independently verified local checksum.
Local installer/plugin SHA256 and individual test cases were not supplied.
Therefore local binary hash verification and detailed sample-rate/buffer,
automation, multiple-instance, recovery and
render checks remain **NOT RUN / not individually documented in this report**;
this label describes missing scoped evidence, not a claim that the owner did not
perform them. The Windows general-operation feedback is now associated with the
named final bundle, with this checksum limitation. Expected final EXE:
`VDX7-1.0.1-Windows-x64-Setup.exe`, SHA256
`d9472981ee8d651ca1b609e3e94683470e19be2a7d841c21e1b62edc394ac84e`.

The owner also reports testing the contents of the **same named bundle** on a
**Mac mini M1 (2020), 16 GB RAM**, with successful operation.
Status: **PASS — owner-reported general macOS operation**. The owner confirmed
**PKG installation, macOS 26.7.1 and REAPER 7.82 Universal** (described by the
owner as "7.82 x64 universal"). This is an owner-supplied environment description,
not independent verification of OS/version or the host's active architecture.
Native arm64 versus Rosetta execution and the local installed-binary checksum
were not supplied. Do not extend M1 evidence to Intel hardware.
Detailed per-case matrix coverage is not individually documented.

No permission to publish,
create tags/releases or replace assets was given. Windows EXE remains unsigned;
macOS PKG is unsigned, plugin ad-hoc signed, without Developer ID/notarization.
No Yamaha ROM or factory-bank contents are included in this documentation.

## Next gates

Use the [execution plan](../release/EXECUTION_PLAN_1.0.md) as the single ledger:
optionally confirm the installed Windows binary checksum and document remaining
individual checks and local checksum/active host architecture details or explicit
permitted deferrals. General owner operation feedback is PASS on both platforms;
this is not complete matrix acceptance. Finalize R6 release text and obtain separate publication
authorization; only then execute R7 publication and download verification.
No product fixes are justified solely by the supplied overview.
