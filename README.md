# Focusboard

A quiet native Markdown desktop gadget for goals and notes, with synchronized Markdown, themes, zoom,
compact mode, commands, autosave, and a single tray icon.

**Linux:** KDE Plasma 6 Wayland with KWin and LayerShellQt, using the bottom
desktop layer.
**Windows:** a new bottom-stacked Qt/Win32 backend targeting Windows 11 x64 and
Windows 10 x64 (1809+ with the selected Qt 6.8 build). Windows compilation,
packaging, and desktop behavior still require the first Windows CI/manual validation.

## Windows

After the first successful main build publishes the precompiled package to the repository's `prebuilt` branch:

```powershell
git clone https://github.com/cristopherpp/focusboard.git
cd focusboard
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1
```

The script verifies the package from this repository and runs its per-user installer. Users need Git, but no compiler, Qt SDK, or administrator privileges. It creates a Start Menu entry and does not launch the app or enable autostart.

Update, inspect versions, or uninstall with the same script:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1 -Update
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1 -Status
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-windows.ps1 -Uninstall
```

Windows Settings → Installed apps can also uninstall Focusboard after the checkout is deleted. Documents and settings remain in the current user's profile. Same-origin checksums verify integrity, not independent publisher identity; unsigned downloads may show a Windows security warning. Check the repository origin before installing and keep Windows security protections enabled.

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

The repository is [on GitHub](https://github.com/cristopherpp/focusboard). CI builds and tests in an Arch container and on a Windows/MSVC runner; its compositor behavior still needs a KDE session. See [CONTRIBUTING.md](CONTRIBUTING.md) for pull requests, [CHANGELOG.md](CHANGELOG.md) for version policy, and [PERFORMANCE.md](PERFORMANCE.md) for profiling notes.
