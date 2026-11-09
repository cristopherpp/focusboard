# Focusboard

A quiet Markdown gadget for KDE Plasma 6 on Wayland. It sits on the desktop; ordinary windows cover it. Goals and notes stay in local files.

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

Uninstalling leaves your documents and settings untouched. Enable or disable **Launch at login** in the tray menu or KDE System Settings → Autostart. The tray also provides Show, Hide, and Quit; Hide keeps Focusboard running.

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

The usual data files are `~/.local/share/Focusboard/Focusboard/goals.md` and `notes.md`; settings are in `~/.config/Focusboard/Focusboard.conf`. Qt follows `XDG_DATA_HOME` and `XDG_CONFIG_HOME` if set. Focusboard saves Markdown as UTF-8 using atomic writes. It has no network service or telemetry.

## Build from source

Dependencies: Qt 6.8+, LayerShellQt, CMake, Ninja, and a C++20 compiler.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=wayland ./build/focusboard
```

Use `-DCMAKE_BUILD_TYPE=Debug` or `Release` for a specific build. The QML is embedded in the executable. The desktop entry is in `packaging/focusboard.desktop.in`.

The icon source is `assets/icons/focusboard.svg`; PNG exports for 16–256 px live beside it. After editing the SVG, regenerate them with `rsvg-convert`:

```sh
for size in 16 32 48 64 128 256; do
  rsvg-convert -w "$size" -h "$size" assets/icons/focusboard.svg \
    -o "assets/icons/${size}x${size}.png"
done
```

The package installs the SVG at `/usr/share/icons/hicolor/scalable/apps/focusboard.svg` and PNGs in matching hicolor directories.

Focusboard uses LayerShellQt's bottom layer with no reserved workspace. Visibility and stacking depend on KWin and should be checked in the target KDE Wayland session, especially with multiple monitors. Offscreen tests do not verify compositor behavior.

Local package and AUR template details are in [packaging/README.md](packaging/README.md). No public Git remote is configured yet.
