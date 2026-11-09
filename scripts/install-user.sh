#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build="${root}/build-release"
cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build "$build"
cmake --install "$build"
mkdir -p "$HOME/.config/autostart"
printf 'Installed Focusboard to ~/.local. Enable Launch at login in the tray menu to create its user autostart entry.\n'
