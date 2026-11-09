#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
temporary=$(mktemp -d)
trap 'rm -rf -- "$temporary"' EXIT
command -v script >/dev/null
mkdir -p "$temporary/bin" "$temporary/project"
cat > "$temporary/bin/fake-focusboard" <<'FAKE'
#!/usr/bin/env bash
set -euo pipefail
case $1 in
    --autostart-state) cat "$FOCUSBOARD_STATE_FILE" ;;
    --autostart-enable) printf 'enabled\n' > "$FOCUSBOARD_STATE_FILE" ;;
    --autostart-disable) printf 'disabled\n' > "$FOCUSBOARD_STATE_FILE" ;;
    *) exit 2 ;;
esac
FAKE
chmod +x "$temporary/bin/fake-focusboard"
export FOCUSBOARD_AUTOSTART_CLI="$temporary/bin/fake-focusboard"
export FOCUSBOARD_STATE_FILE="$temporary/state"
helper="$root/scripts/autostart-after-install.sh"

printf 'unset\n' > "$FOCUSBOARD_STATE_FILE"
output=$("$helper")
[[ $(cat "$FOCUSBOARD_STATE_FILE") == unset && $output == *Noninteractive* ]]

printf '\n' | script -qefc "$helper" /dev/null > "$temporary/output"
[[ $(cat "$FOCUSBOARD_STATE_FILE") == enabled ]]
[[ $(cat "$temporary/output") == *'Start Focusboard automatically at KDE login? [Y/n]:'* ]]
output=$("$helper")
[[ $(cat "$FOCUSBOARD_STATE_FILE") == enabled && $output == *'Existing Focusboard autostart preference: enabled'* ]]
script -qefc "$helper" /dev/null > "$temporary/output"
! grep -q 'Start Focusboard automatically at KDE login?' "$temporary/output"

printf 'unset\n' > "$FOCUSBOARD_STATE_FILE"
printf 'Y\n' | script -qefc "$helper" /dev/null > "$temporary/output"
[[ $(cat "$FOCUSBOARD_STATE_FILE") == enabled ]]

printf 'unset\n' > "$FOCUSBOARD_STATE_FILE"
printf 'n\n' | script -qefc "$helper" /dev/null > "$temporary/output"
[[ $(cat "$FOCUSBOARD_STATE_FILE") == disabled ]]
output=$("$helper")
[[ $(cat "$FOCUSBOARD_STATE_FILE") == disabled && $output == *'Existing Focusboard autostart preference: disabled'* ]]

# A failed makepkg -si must stop before the post-install choice is invoked.
cp -a "$root"/{CMakeLists.txt,LICENSE,README.md,CHANGELOG.md,CONTRIBUTING.md,PERFORMANCE.md,.clang-format,.gitignore,src,qml,assets,tests,scripts,packaging} "$temporary/project/"
cat > "$temporary/bin/makepkg" <<'FAKEPKG'
#!/usr/bin/env bash
printf '%s\n' "$1" >> "$FOCUSBOARD_MAKEPKG_LOG"
[[ $1 == --verifysource ]] && exit 0
exit 42
FAKEPKG
chmod +x "$temporary/bin/makepkg"
export FOCUSBOARD_MAKEPKG_LOG="$temporary/makepkg-log"
if [[ $EUID -eq 0 ]]; then
    chown -R nobody:nobody "$temporary"
    if runuser -u nobody -- env PATH="$temporary/bin:$PATH" \
        FOCUSBOARD_MAKEPKG_LOG="$FOCUSBOARD_MAKEPKG_LOG" \
        "$temporary/project/scripts/install-arch.sh" > "$temporary/install-output" 2>&1; then
        printf 'Installer unexpectedly succeeded after makepkg failure\n' >&2
        exit 1
    fi
else
    if PATH="$temporary/bin:$PATH" "$temporary/project/scripts/install-arch.sh" > "$temporary/install-output" 2>&1; then
        printf 'Installer unexpectedly succeeded after makepkg failure\n' >&2
        exit 1
    fi
fi
grep -qx -- '--verifysource' "$FOCUSBOARD_MAKEPKG_LOG"
grep -qx -- '-si' "$FOCUSBOARD_MAKEPKG_LOG"
! grep -Eq 'Start Focusboard automatically at KDE login|Autostart takes effect' "$temporary/install-output"
printf 'autostart installer tests passed\n'
