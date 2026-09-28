# VDX7 development and backup policy

User instruction (2026-09-13): after each completed, verified development round,
commit the work locally and push it to GitHub main as a second backup.

- Check remote main before pushing; preserve changes made elsewhere.
- Never force-push main or rewrite its history.
- Source backup is not release publication: do not create tags, GitHub Releases,
  or replace release assets without a separate publication request.
- Never commit Yamaha firmware/factory ROM, local credentials, build caches,
  or installed binaries. User-supplied ROM stays local for integration tests.
- Keep development builds visibly marked until 1.0.0 acceptance is complete.
- Keep new validation reports in `docs/validation/` and other detailed documents
  in the appropriate `docs/` subdirectory. Keep root README files, licence notices
  and agent instructions in place; update links and `docs/DOCUMENT_PATHS.md`
  whenever an existing document moves.
