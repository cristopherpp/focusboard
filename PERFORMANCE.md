# Performance notes

Environment on 2026-11-09: Arch Linux, kernel 7.2.14-arch1-1, Intel Core i3-1215U (8 logical CPUs), Qt 6.12.0, Plasma/KWin 6.8.2, LayerShellQt 6.8.2.

A Release build with `FOCUSBOARD_PROFILE_STARTUP=1` logs elapsed milliseconds at bootstrap, window initialization, and first `show()`. A controlled Wayland run using empty temporary `XDG_DATA_HOME` and `XDG_CONFIG_HOME` timed out after 15 seconds without reaching the first log point in this tool environment. It produced no valid startup or idle measurements. The offscreen test suite passed in about 1.2 seconds; that measures tests, not application startup.

No renderer or QML optimization was made without a measured bottleneck. To profile in a directly accessible KDE session:

1. Quit other Focusboard copies. Set temporary XDG data/config directories, then start the Release binary with `FOCUSBOARD_PROFILE_STARTUP=1 QT_QPA_PLATFORM=wayland` and capture stderr. Repeat at least five times, separating first launch from warm launches.
2. Once visible, sample `ps -C focusboard -o pid,etime,time,rss,%cpu` twice at least 30 seconds apart while idle. Report CPU time delta and resident memory, not just cumulative `%cpu`.
3. Use the same documents for preview, large-document scrolling, repeated resizing, compact toggles, and hide/show. Record document byte counts, display scale, GPU, and timings. `perf` or `heaptrack` can then locate any confirmed hot path or allocation issue.

The current renderer uses `QQuickPaintedItem` and `QTextDocument`. Keep it until profiling shows a specific problem and a replacement passes visual and data-integrity checks.

## Windows measurement status

No Windows executable, installer, or ZIP has been built or run in the Linux
implementation environment. Installer size, portable size, installed disk usage,
cold startup, idle CPU, and memory are **not yet measured**.

The Windows CI packaging script writes
`Focusboard-<version>-measurements.json` containing actual installer and ZIP
bytes, staged file bytes/count, and (after the installer smoke test) installed
file bytes including the uninstaller. These are file lengths, not filesystem
allocation sizes, and exclude user data. Preserve the report with the release.

For desktop performance measurements, use a clean VM or standard-user account,
quit any running instance, record OS/hardware/Qt/compiler/package version, and:

1. Set FOCUSBOARD_PROFILE_STARTUP=1; capture Qt debug output with a debugger or
   DebugView. The GUI subsystem may not have an attached stderr. Bootstrap/first
   show timings do not measure first painted frame or complete cold process launch.
2. Measure cold launch separately after reboot, then at least five warm launches.
   Record the measurement method and do not present warm launches as cold.
3. Once idle, use PowerShell Get-Process Focusboard to sample WorkingSet64
   and TotalProcessorTime twice 30 seconds apart. CPU utilization of one core is
   100 * CPU-seconds-delta / elapsed-seconds; divide by logical CPU count if
   reporting whole-machine utilization. Record which convention is used.
4. Repeat with the same documents and one versus multiple/mixed-DPI displays.
   Verify visually that the app is interactive and settled before sampling.

App-local runtimes prioritize reliable no-admin deployment; no measured size or
performance benefit is claimed for components that have not been tested.
