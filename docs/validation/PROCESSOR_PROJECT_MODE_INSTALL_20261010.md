# Processor project-mode installation — 2026-10-10

Baseline: `69c56fe7707558feef685c9cef8dbfc97932f35b` (#178).
Branch: `feature/processor-project-mode-install`.

Compatible pending project completion now installs current owner sound-mode
intent after restoring the payload, while the existing engine mutex excludes
rendering, and before publishing project readiness. It uses the explicit
quiescent API to discard previous output history. No Clean admission/UI or
state writer is enabled by this step.

Local checkpoint: Release actual processor target rebuilt successfully.
Actual processor `--snapshot-only` with private original v1.8 passed, including
the added assertion that compatible Classic completion schedules cold native
installation before resumed rendering. All 22 registered ROM-free CTest tests
passed (existing other targets; not a full fresh rebuild in this checkpoint).
Whitespace check passed. No Yamaha bytes committed.

Still required before PR acceptance: stronger interrupted-Clean-to-Classic
project regression, complete rebuild, sanitizer checks and platform CI/review.
Prepare/release/reset/reload lifecycle integration, Clean admission/writer,
APVTS atomicity and user-interface integration remain separate open gates.
No release, installed plugin, DAW project or GUI screenshot changed.

## Follow-up — 2026-10-11

Added actual-processor private-ROM regression: establish active Clean in the
real engine, start a Classic down-ramp, verify it remains interrupted while
Clean is active, then recall a compatible Classic project. Completion must
schedule cold Classic installation before readiness resumes. Clean setup is
test-only renderer injection, not positive public Clean-state admission.

PASS: fresh `vdx7_ci_checks` build and 22/22 ROM-free CTest; private original
v1.8 snapshot suite including interrupted transition. Targeted ASan/UBSan
processor rebuild and the same private snapshot suite PASS, with leak detection
disabled; no TSAN/leak or REAPER acceptance is claimed. The first private attempt
failed its file-exists prerequisite because the earlier temporary fixture was
gone; re-extraction and SHA256 verification restored the same original fixture.
No runtime test gate was weakened.
