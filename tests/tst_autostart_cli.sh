#!/usr/bin/env bash
set -euo pipefail
focusboard=$1
temporary=$(mktemp -d)
trap 'rm -rf -- "$temporary"' EXIT
export XDG_CONFIG_HOME="$temporary/config"
export XDG_DATA_HOME="$temporary/data"
[[ $("$focusboard" --autostart-state) == unset ]]
[[ $("$focusboard" --autostart-disable) == disabled ]]
[[ $("$focusboard" --autostart-state) == disabled ]]
[[ ! -e $XDG_CONFIG_HOME/autostart/focusboard.desktop ]]
[[ $("$focusboard" --autostart-enable) == enabled ]]
[[ $("$focusboard" --autostart-state) == enabled ]]
entry="$XDG_CONFIG_HOME/autostart/focusboard.desktop"
[[ -f $entry ]]
grep -qx '\[Desktop Entry\]' "$entry"
grep -q '^Exec=' "$entry"
[[ $("$focusboard" --autostart-disable) == disabled ]]
[[ $("$focusboard" --autostart-state) == disabled ]]
grep -qx 'Hidden=true' "$entry"
[[ $("$focusboard" --autostart-enable) == enabled ]]
[[ $("$focusboard" --autostart-state) == enabled ]]
[[ $(find "$XDG_CONFIG_HOME/autostart" -maxdepth 1 -name '*.desktop' | wc -l) -eq 1 ]]
printf 'autostart CLI tests passed\n'
