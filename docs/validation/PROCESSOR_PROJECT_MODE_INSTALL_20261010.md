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
