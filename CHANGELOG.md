# Changelog

Version numbers come from `project(Focusboard VERSION …)` in CMakeLists.txt. A release tag is `v<version>`; local Arch packaging revisions use `pkgrel` without changing the app version. The AUR `-git` package derives its version from Git tags and commits.

## 1.0.1 (unreleased)

- Add a shared desktop-window interface with the existing Linux LayerBottom implementation and a Windows Qt/Win32 bottom-stacked tool-window backend.
- Add independent Windows monitor geometry, native per-user autostart, embedded icon/version resources, and common AppData storage for installed and portable copies.
- Add per-user Inno Setup and self-contained ZIP packaging through Qt deployment and app-local MSVC runtimes.
- Extend CI with Windows tests, staged/extracted runtime checks, installer smoke tests, measurements, and idempotent draft-release attachments.
- Isolate cross-platform test storage and cover logical monitor geometry and cross-process single-instance locks.
- Windows build, package execution, and desktop integration remain pending Windows validation; no package sizes or Windows performance results are claimed.

- Protect local edits from external Markdown changes and save a recovery copy on conflicts.
- Retry recoverable save failures with bounded backoff and show unsaved status.
- Clamp malformed saved settings and report normal-shutdown settings failures.
- Add headless CI, draft release automation, and AUR packaging metadata.
- Add user-controlled installer and first-launch autostart choices while preserving existing entries.
- Create one bottom-layer gadget surface per connected monitor, sharing Markdown, commands, settings, and the single tray icon.
- Fix CI whitespace validation for root commits and shallow checkouts.

## 1.0.0

- Initial public source release: KDE Plasma Wayland gadget, Markdown goals and notes, command palette, atomic autosave, tray, autostart, and Arch packaging.
