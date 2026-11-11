# Focusboard

A quiet native Markdown desktop gadget for goals and notes. One process creates
one gadget per connected monitor, with synchronized Markdown, themes, zoom,
compact mode, commands, autosave, and a single tray icon. No display setup is required.

**Linux:** KDE Plasma 6 Wayland with KWin and LayerShellQt, using the bottom
desktop layer. X11 and other desktops are unsupported or untested.
**Windows:** a new bottom-stacked Qt/Win32 backend targeting Windows 11 x64 and
Windows 10 x64 (1809+ with the selected Qt 6.8 build). Windows compilation,
packaging, and desktop behavior still require the first Windows CI/manual validation.
Do not interpret this implementation as verified Windows 7 gadget parity.

## Install on Windows

When Windows artifacts are available on the [releases page](https://github.com/cristopherpp/focusboard/releases):

1. Download **Focusboard-Setup-x64.exe**.
2. Run the installer.
3. Launch **Focusboard** from Start.

Installation is per-user, normally in %LOCALAPPDATA%\Programs\Focusboard.
No administrator rights, Qt SDK, compiler, or separate runtime download is required.
The Start Menu shortcut is included; the desktop shortcut is optional.
The installer neither launches Focusboard nor enables autostart.

For the portable alternative, download **Focusboard-Portable-x64.zip**, extract
the entire archive, and run **Focusboard.exe**. Keep its DLLs, plugins, and QML
directories together. Portable and installed copies use the same per-user data
and single-instance lock; “portable” describes the application files, not a
separate portable-data mode.

On first launch, choose whether to start at the next Windows login. Change the
choice later using **Launch at login** in the tray. The application writes one
quoted executable path to
HKCU\Software\Microsoft\Windows\CurrentVersion\Run\Focusboard;
it remembers a declined preference and does not repeatedly prompt. If you move
a portable folder, toggle autostart off and on from the new location. Windows
Startup settings can independently disable startup.

Windows documents are %APPDATA%\Focusboard\Focusboard\goals.md and notes.md
(Qt's QStandardPaths::AppDataLocation). Settings are under
HKCU\Software\Focusboard\Focusboard. Quit from the tray before upgrading
or uninstalling. Remove the installed app through Windows Settings → Apps;
for portable use, disable autostart and delete the extracted folder. Documents
and settings are preserved. The uninstaller removes only an autostart entry
that points to that installation.

Unsigned downloads may show an unknown-publisher or Microsoft Defender
SmartScreen warning. Confirm the repository/release source and published
SHA-256 checksum before deciding whether to run a download. A matching checksum
checks file integrity, not publisher identity. Do not disable Windows security
protections; managed devices may require approval from your administrator.

Windows uses a frameless tool window, not an Explorer-embedded desktop object.
It requests bottom stacking without always-on-top or Explorer reparenting.
Win+D/Show Desktop may hide it; use **Show Focusboard** from the tray to recover.
Virtual desktops follow Windows' normal window placement; the app does not pin
itself to all virtual desktops. Clicking permits keyboard activation, whose
interaction with stacking must be verified on Windows. Per-display drag/resize
geometry is retained independently. Monitor identity changes can reset placement.
See the [Windows validation checklist](packaging/windows/README.md).

## Install on Arch Linux

From this directory, as your normal user:

```sh
./scripts/install-arch.sh
focusboard
```

The installer builds a local pacman package with `makepkg -si`. It needs `base-devel`, `cmake`, and `ninja`; install missing tools with `sudo pacman -S --needed base-devel cmake ninja`. Focusboard also appears in KDE's application menu. The installed app does not depend on this source directory.

To update, pull or copy the new source and rerun `./scripts/install-arch.sh`. To uninstall:

```sh
sudo pacman -Rns focusboard
```

Uninstalling leaves your documents and settings untouched. The tray also provides Show, Hide, and Quit; Hide keeps Focusboard running.

After a successful interactive local installation, the installer asks **“Start Focusboard automatically at KDE login? [Y/n]:”**. Enter or **Y** enables it; **N** records a disabled preference. Noninteractive installs skip the question. An existing enabled or disabled preference is kept without prompting. The installer never starts Focusboard immediately.

An AUR installation does not edit your home directory. On the first launch of an installed copy with no existing preference, a small in-gadget prompt offers **Yes** or **No**; neither is chosen automatically. Change your choice later with **Launch at login** in the tray menu or **KDE System Settings → Autostart**. Disabling it leaves a `Hidden=true` entry so the choice is remembered. Starting Focusboard now is separate from autostart, which launches it at the **next KDE login**.

The entry is `${XDG_CONFIG_HOME:-$HOME/.config}/autostart/focusboard.desktop`. Check the effective choice with `/usr/bin/focusboard --autostart-state` (`enabled`, `disabled`, or `unset`). Legacy Focusboard entries are migrated to this name without enabling a second copy.

## Use

Click the gadget, then press **Ctrl+K** or **:** to open commands. Use **Tab** to complete, **Up/Down** for history, and **Escape** to close. Available commands:

```text
:help                 :edit goals          :edit notes
:goal add <text>      :goal toggle <n>     :goal delete <n>
:zoom +               :zoom -              :zoom reset
:theme light          :theme dark          :theme system
:collapse             :expand
:position top-left    :position top-right  :position bottom-left
:position bottom-right
:resize <width> <height>   :hide            :quit
```

Task numbers start at 1 in the current `goals.md` source order. While editing, **Ctrl+Enter** or **Escape** saves and returns to preview. Edits also autosave after about 400 ms. **Ctrl+Plus**, **Ctrl+Minus**, and **Ctrl+0** change text size. Hover to reveal the move grip and resize handle.

On Linux, the unchanged data files are `~/.local/share/Focusboard/Focusboard/goals.md` and `notes.md`; settings are in `~/.config/Focusboard/Focusboard.conf`. Qt follows `XDG_DATA_HOME` and `XDG_CONFIG_HOME` if set. Focusboard saves Markdown as UTF-8 using atomic writes. It has no network service or telemetry.

Clean external edits reload automatically. If a file changes while Focusboard has unsaved edits, it keeps the external file, writes a `*.local-conflict-*.md` copy of the local text beside it, and shows an error. Resolve both versions manually; Focusboard will not guess a merge. A failed save remains dirty and is retried up to three times. Quit asks before discarding unsaved changes.

## Build from source

Linux dependencies: Qt 6.8+, LayerShellQt, CMake, Ninja, and a C++20 compiler.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=wayland ./build/focusboard
```

Use `-DCMAKE_BUILD_TYPE=Debug` or `Release` for a specific build. The QML is embedded in the executable. The desktop entry is in `packaging/focusboard.desktop.in`.

For Windows source builds, use an x64 MSVC 2022 developer shell, Qt 6.8.3
msvc2022_64, CMake, Ninja, and Inno Setup 6.3+:

~~~powershell
cmake -S . -B build-windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build-windows --parallel 2
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_QUICK_BACKEND = 'software'
ctest --test-dir build-windows --output-on-failure
pwsh -File scripts/package-windows.ps1
~~~

Use a fresh stage-windows directory. The packaging script uses
cmake --install and Qt's QML deployment API, generates both artifacts in
release-artifacts, and writes actual size/checksum reports. PowerShell 7 and
the development tools are needed by maintainers only. Remove the two test
environment variables before launching the normal GUI.

The icon source is `assets/icons/focusboard.svg`; PNG exports for 16–256 px live beside it. After editing the SVG, regenerate them with `rsvg-convert`:

```sh
for size in 16 32 48 64 128 256; do
  rsvg-convert -w "$size" -h "$size" assets/icons/focusboard.svg \
    -o "assets/icons/${size}x${size}.png"
done
```

The package installs the SVG at `/usr/share/icons/hicolor/scalable/apps/focusboard.svg` and PNGs in matching hicolor directories.

Focusboard uses LayerShellQt's bottom layer with no reserved workspace. KWin controls final stacking and output assignment. Check the target KDE Wayland session with multiple monitors, including hot-plug, unplug, resolution changes, and mixed scaling. Disconnected outputs lose their gadget surface; connecting them creates one automatically. It cannot cover the lock screen or compositor-controlled overlays. Offscreen tests do not verify compositor behavior.

To test manually, expose the desktop and overlap the gadget with a normal window; then check dragging, resizing, hide/show, autostart after login, and monitor or scale changes. The automated tests use temporary data directories and do not touch your documents.

The repository is [on GitHub](https://github.com/cristopherpp/focusboard). CI builds and tests in an Arch container and on a Windows/MSVC runner; its compositor behavior still needs a KDE session. See [CONTRIBUTING.md](CONTRIBUTING.md) for pull requests, [CHANGELOG.md](CHANGELOG.md) for version policy, and [PERFORMANCE.md](PERFORMANCE.md) for profiling notes.

The `focusboard-git` AUR recipe is prepared in `packaging/aur/focusboard-git/`. Once submitted to AUR, install it with `yay -S focusboard-git`, or clone its AUR Git repository and run `makepkg -si` as your normal user. See [packaging/README.md](packaging/README.md) for packaging and submission steps. It has not been published to AUR by this repository.
