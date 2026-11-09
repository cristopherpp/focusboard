# Changelog

Version numbers come from `project(Focusboard VERSION …)` in CMakeLists.txt. A release tag is `v<version>`; local Arch packaging revisions use `pkgrel` without changing the app version. The AUR `-git` package derives its version from Git tags and commits.

## 1.0.1 (unreleased)

- Protect local edits from external Markdown changes and save a recovery copy on conflicts.
- Retry recoverable save failures with bounded backoff and show unsaved status.
- Clamp malformed saved settings and report normal-shutdown settings failures.
- Add headless CI, draft release automation, and AUR packaging metadata.

## 1.0.0

- Initial public source release: KDE Plasma Wayland gadget, Markdown goals and notes, command palette, atomic autosave, tray, autostart, and Arch packaging.
