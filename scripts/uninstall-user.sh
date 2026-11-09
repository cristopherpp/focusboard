#!/usr/bin/env bash
set -euo pipefail
rm -f "$HOME/.local/bin/focusboard" "$HOME/.local/share/applications/focusboard.desktop" "$HOME/.local/share/xdg/autostart/focusboard.desktop" "$HOME/.config/autostart/focusboard.desktop"
printf 'Removed Focusboard executable and launch entries. Markdown files and settings remain in ~/.local/share/Focusboard and ~/.config/Focusboard.\n'
