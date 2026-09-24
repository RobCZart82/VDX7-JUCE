# Desktop processor regression follow-up — 2026-09-24

Desktop-enabled execution now passes the display prerequisite. It exposed the
previously unreached `RAM round trip before rendering` assertion: the test
required every byte of the 6144-byte snapshot to survive project restoration.
This is incompatible with the explicit fresh-runtime restoration contract.

Diagnostic byte comparison found no differences in the packed4096-byte bank or
the16 persistent-setting bytes checked below. Differences included20a7,
2375/2376,23f4..24f3,2582 and27e0..27ff. They are not blanket-labelled safe
for arbitrary firmware: the project projection is restricted to verifiedv1.8.

Only the two cross-project restore assertions now compare persistent RAM:
all4096 packed bank bytes, play settings, tuning, pitch-bend settings and four
controller ranges/assignments. Existing full-RAM comparisons for in-place
nonmutation remain unchanged. All host-parameter comparisons remain unchanged.
The separate held-snapshot tests continue to require clean ownership/silence.

Oracle controls independently flip each bank byte (4096 cases) and each checked
setting byte (16 cases): every mutant must fail comparison; unchanged input must
pass. This does not claim that unknown/unexposed firmware settings are covered.

Rebuilt `vdx7_processor_tests`, then desktop-enabled CTest: **1/1 PASS9.00s**
(total9.01s). Includes SAVE AS, SETTINGS and ABOUT modal assertions, USER storage,
parameter compatibility, legacy restore, editor bounds/diagrams and finite
non-silent renders. No production source change or installed binary replacement.

Earlier full-suite result remains27/29 with GUI blocked and native Note0 failing;
this follow-up is a separate execution, not a new single full-suite28/29 claim.
Actual DAW, live scheduler and suspended-host coverage remain open.

Follow-up MONO CTest2/3, exit8,45.55s: profile PASS; corrected processor PASS
45.38s including held-snapshot cleanup; native acceptance remains FAIL0/1/16.
