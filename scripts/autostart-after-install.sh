#!/usr/bin/env bash
# User-owned post-install choice; makepkg has already completed successfully.
set -euo pipefail

focusboard_cli=${FOCUSBOARD_AUTOSTART_CLI:-/usr/bin/focusboard}
if [[ ! -x $focusboard_cli ]]; then
    printf 'Focusboard was installed, but %s is unavailable for autostart setup.\n' "$focusboard_cli" >&2
    exit 1
fi
if ! state=$("$focusboard_cli" --autostart-state); then
    printf 'Focusboard was installed, but its autostart status could not be read.\n' >&2
    exit 1
fi
case $state in
    enabled|disabled)
        printf 'Existing Focusboard autostart preference: %s. Keeping it.\n' "$state"
        exit 0
        ;;
    unset) ;;
    *) printf 'Unexpected Focusboard autostart status: %s\n' "$state" >&2; exit 1 ;;
esac
if [[ ! -t 0 || ! -t 1 ]]; then
    printf 'Noninteractive install: autostart was not changed. Focusboard will ask on first launch.\n'
    exit 0
fi
while true; do
    if ! read -r -p 'Start Focusboard automatically at KDE login? [Y/n]: ' answer; then
        printf '\nNo autostart choice was recorded. Focusboard will ask on first launch.\n'
        exit 0
    fi
    case ${answer,,} in
        ''|y|yes) choice=--autostart-enable; break ;;
        n|no) choice=--autostart-disable; break ;;
        *) printf 'Please enter Y or N.\n' ;;
    esac
done
if ! "$focusboard_cli" "$choice"; then
    printf 'Focusboard was installed, but the autostart choice could not be saved.\n' >&2
    exit 1
fi
printf 'Autostart takes effect at your next KDE login. Focusboard was not started now.\n'
