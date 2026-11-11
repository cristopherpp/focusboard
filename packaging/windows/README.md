# Windows deployment and validation

The Windows backend is implemented but has not been compiled or run on Windows
in the Linux implementation environment. CI and manual checks below are gates
for claiming support, not a record that those checks have passed.

## Architecture and behavior

DesktopWindowController is the small QML-facing interface for initialization,
dragging, resizing, palette expansion, and bounded sizing. CMake selects either
src/platform/linux/LinuxDesktopBackend.cpp (the existing LayerShellQt behavior)
or src/platform/windows/WindowsDesktopBackend.cpp. Windows SDK headers and
user32 never enter Linux builds; LayerShellQt never enters Windows builds.

DocumentStore, SettingsManager, MarkdownPreview, CommandController, QML,
MultiMonitorController, file watching/conflicts, debounce, tray, and process
lifecycle remain shared. A second DocumentStore constructor accepts a directory
for tests; production still uses the same QStandardPaths location as before.
Tests use temporary INI settings. Native registry tests use a unique disposable
key, never the real Run registration.

Windows sets Qt::Tool, Qt::FramelessWindowHint, and
Qt::WindowStaysOnBottomHint, reinforces WS_EX_TOOLWINDOW, removes
WS_EX_APPWINDOW, and uses SetWindowPos(HWND_BOTTOM) without activation.
It restacks after show and deactivation, not on a polling timer.
There is no WS_EX_NOACTIVATE, so editing can receive focus. Windows selects the
customizable Fusion control style to retain the existing QML backgrounds; Linux
style selection is unchanged. Qt's Windows
platform plugin translates logical QScreen/QWindow geometry to native coordinates;
Win32 positioning calls use SWP_NOMOVE | SWP_NOSIZE.
Negative desktop origins and fractional-scale logical rectangles are tested
without pretending those tests verify physical mixed-DPI hardware.

This is a stable-API bottom-stacking approach, **not desktop embedding**.
It neither enumerates WorkerW/Progman nor reparents to Explorer. The intended
taskbar/Alt+Tab exclusion comes from tool-window styles. Win+D may hide the app,
activation may affect stacking, and Windows controls virtual-desktop membership.
No claim is made that a bottom-stacked HWND always remains above every wallpaper
implementation. Explorer restart should not orphan windows because they are not
Explorer children, but tray recovery and visibility need real desktop testing.

References:

- [Microsoft SetWindowPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
- [Microsoft extended window styles](https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles)
- [Qt 6.8 supported Windows configurations](https://doc.qt.io/qt-6.8/windows.html)

## Packages

CMake sets the Windows output name to Focusboard.exe and installs it at the
stage root. qt_generate_deploy_qml_app_script() deploys Qt shared libraries,
QML imports, plugins, graphics dependencies, and a relative qt.conf.
Release deployment omits translations and debug symbols. It does not arbitrarily
strip plugins that may be needed by the renderer or controls.

Qt's default MSVC runtime installer is deliberately disabled. CMake's
InstallRequiredSystemLibraries deploys redistributable MSVC DLLs beside the
executable, so the installer and ZIP need neither elevation nor a separate
runtime installation. Windows 10/11 supply the Universal CRT. Rebuild packages
with serviced compiler runtimes when updates are needed; app-local deployment
does not inherit all central runtime servicing.

References:

- [Qt 6.8 Windows deployment](https://doc.qt.io/qt-6.8/windows-deployment.html)
- [CMake runtime installation](https://cmake.org/cmake/help/latest/module/InstallRequiredSystemLibraries.html)
- [Microsoft local deployment](https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method)

focusboard.iss consumes only the CMake installation tree. Its stable AppId
supports upgrades. It uses HKCU/per-user installation, optional desktop shortcut,
Start Menu shortcut, and executable/installer/uninstaller branding. It creates
no startup entry, launch action, service, or scheduled task. Uninstall preserves
AppData and settings and removes a Run value only when it names this installed
executable. Quit the running gadget before upgrades/uninstall.

The ICO wraps the existing 16, 32, 48, 64, 128, and 256 pixel PNG exports of
assets/icons/focusboard.svg, without changing the logo. Regenerate it after
updating the PNG exports with python3 scripts/make-windows-icon.py; Python is
only an optional asset-maintenance tool, never a build or end-user dependency.

package-windows.ps1:

1. Reads build-windows/version.txt generated from the CMake project version.
2. Requires a fresh stage, installs the application and dependencies.
3. Checks runtime files, executable version/icon, and embedded QML using a child
   process with Qt/SDK paths removed from its environment.
4. Builds Focusboard-Portable-x64.zip, extracts into a path with spaces and Unicode, and
   repeats runtime checks.
5. Compiles Focusboard-Setup-x64.exe with Inno Setup.
6. Writes SHA-256 checksums and measured installer/ZIP/staged bytes.

On the disposable CI account, test-windows-installer.ps1 also installs into a
temporary directory, verifies metadata and runtime, checks that installation did
not enable autostart or launch the app, records installed file bytes including
the uninstaller, then uninstalls. It refuses to replace an existing installation.
Do not run this installer smoke test on an account with a real Focusboard install.

The runner has developer tools and system runtimes. A restricted child PATH
catches accidental Qt SDK dependencies but is not proof of a clean-machine
deployment. Keep a Windows Sandbox/VM test as a release gate.

## Manual Windows checklist

Record OS build, Qt/compiler version, GPU/driver, screen geometry and scales,
package version/hash, observations, and any failures. All items are **pending**.

- [ ] Clean Windows 11 x64 standard-user account: installer and extracted ZIP run offline without Qt, developer tools, or a separately installed MSVC runtime.
- [ ] Windows 10 x64 1809+ compatibility; repeat essential launch/edit/save checks.
- [ ] Installer uses no elevation, makes Start shortcut and only the selected desktop shortcut; icon/version metadata appear in Explorer, tray and uninstall UI.
- [ ] Installation does not launch the app or enable startup; first-launch Yes and No persist, and tray toggles affect the next login without duplicate Run values.
- [ ] Unicode paths and documents; atomic autosave, external edits, conflict copies, failed save/retry, normal quit and Windows logout preserve data.
- [ ] Only one process/writer/tray when starting installer and portable copies; second invocation leaves the running instance intact.
- [ ] Desktop exposed: visibility above wallpaper; ordinary and maximized windows cover it. Verify keyboard activation, typing, commands and focus handoff without unexpected raising.
- [ ] No taskbar button or Alt+Tab entry; no always-on-top behavior.
- [ ] Win+D twice, taskbar Show Desktop and Peek; record whether the gadget disappears and whether tray Show restores it.
- [ ] Switch/create/remove virtual desktops; record normal per-desktop placement and recovery. No all-desktop pinning is promised.
- [ ] Restart Explorer; verify tray reappearance, visibility and editing.
- [ ] Two or more displays: one gadget per screen, synchronized goals/notes, one shared autosave stream and tray; independent dragging/resizing survives restart.
- [ ] Mixed 100/125/150/200% scales, negative left/above origins, taskbar changes, resolution changes, hotplug/unplug/reconnect, primary-display changes and sleep/resume.
- [ ] Compact mode and command popup stay within each monitor; expanding restores size.
- [ ] Upgrade while preserving documents/settings; quit before upgrade/uninstall. Uninstall leaves Markdown/conflict copies and preferences; only its own Run entry is removed.
- [ ] Record actual package/installed sizes, cold startup and 30-second idle memory/CPU using PERFORMANCE.md; do not infer performance from CTest duration.

## Producing a release

1. Review and commit the implementation separately; this task does not commit,
   push, tag, dispatch CI, or modify GitHub releases.
2. Run the normal CI workflow on the reviewed branch. Inspect Arch and Windows
   results and download both Windows artifacts.
3. Complete the manual checklist on real Windows and the existing KDE checklist.
4. Set the CMake version and update CHANGELOG.md. With maintainer authorization,
   create and push the matching v<version> tag.
5. The tag workflow calls both CI jobs, builds the Arch package and both Windows
   packages, verifies the version, and creates or updates one draft release.
   Reruns replace draft attachments; a published release is not changed.
6. Review the versioned measurement report/checksums and actual artifacts,
   record manual results, then publish the draft manually.

The release tag and binary metadata identify the version; download filenames
remain Focusboard-Setup-x64.exe and Focusboard-Portable-x64.zip for each release.

## Validation recorded during implementation (2026-11-10)

On Arch Linux with GCC 16.2.1 and Qt 6.12.0:

- Release configure/build succeeded.
- All five CTest suites passed (66 core Qt Test cases and 5 presentation cases,
  plus installer shell tests, autostart CLI tests and embedded QML verification).
- QML lint, desktop-file validation, whitespace checks and workflow YAML parsing passed.
- Staged Linux installation retained the executable, SVG/six PNG icons, documents,
  and embedded QML, with no RPATH/RUNPATH or Windows binaries.
- The local Arch package source/checksum, build, test and packaging steps passed;
  no package was installed on the host.
- The ICO was checked against all six existing PNG exports (15,488 bytes).
- Windows compilation, native tests, PowerShell/Inno execution, installer/ZIP
  sizes, clean-machine deployment and interactive behavior were not executed.
  No CI workflow was dispatched and no remote repository/release was modified.
