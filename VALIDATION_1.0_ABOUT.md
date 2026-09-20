# Branded About dialog

SAWSTAR-inspired information hierarchy, not its code, licenses or version:
VDX7 Mk 1. vector heading; GYR vector; developer credit; actual development
version; clickable GitHub source link; existing AGPL/core/JUCE and ROM notices.
Warm panel gradient, off-white text and turquoise dividers. Main layout unchanged.

Owned DialogWindow content holds its own drawable resources and look-and-feel.
OK, Enter and Escape provide close actions; normal window close remains available.
No processor references or editor-owned rendering resources are retained.

Local macOS arm64 VST3 build and strict ad-hoc signature verification passed.
Processor regression/screenshot test passed, including valid vector assets,
content bounds, exact source-link URL, OK close and survival after editor deletion.
1x dialog and 2x content snapshots generated for visual review. Entire CTest suite
was not rerun for this GUI-only chapter; Windows/physical HiDPI/host acceptance,
keyboard shortcuts and actual external link launch remain manual acceptance items.

The existing Xcode license warning remains unchanged. No installed plugin change,
tag/release or firmware redistribution. GYR vector contour fidelity remains subject
to owner review. Build hash is not invented; only the configured version is shown.
