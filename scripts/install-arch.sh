#!/usr/bin/env bash
# Build a local source snapshot and let makepkg/pacman own installation.
set -euo pipefail

fail() { printf 'Focusboard: %s\n' "$*" >&2; exit 1; }
[[ $EUID -ne 0 ]] || fail 'Run this script as your normal user, never as root. makepkg requests sudo only for pacman.'
[[ $# -le 1 ]] || fail 'Usage: ./scripts/install-arch.sh [--prepare-only]'
mode=${1:-install}
[[ $mode == install || $mode == --prepare-only ]] || fail 'Usage: ./scripts/install-arch.sh [--prepare-only]'
[[ -r /etc/os-release ]] || fail 'Cannot identify the operating system.'
# System-owned OS metadata, not a downloaded script.
. /etc/os-release
[[ ${ID:-} == arch || " ${ID_LIKE:-} " == *' arch '* ]] || fail 'This installer requires Arch Linux or an Arch-based distribution.'
for tool in pacman makepkg fakeroot cmake ninja c++ tar xz sha256sum sed mktemp; do
    command -v "$tool" >/dev/null || fail "Missing $tool. Install the development tools with: sudo pacman -S --needed base-devel cmake ninja"
done
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
version=$(sed -nE 's/^project\(Focusboard VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)$/\1/p' "$root/CMakeLists.txt")
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || fail 'Cannot read the release version from CMakeLists.txt.'
mkdir -p "$root/build-arch"
stage=$(mktemp -d "$root/build-arch/package.XXXXXXXX")
trap 'printf "Focusboard packaging failed (line %s). Build files remain in: %s\n" "$LINENO" "$stage" >&2' ERR
archive="$stage/focusboard-$version.tar.xz"
# Allowlisted source roots include uncommitted edits but never the developer checkout metadata,
# build trees, user documents, package outputs, or credentials at the project root.
tar --create --xz --file "$archive" --directory "$root" \
    --transform="s,^,focusboard-$version/," \
    --exclude='.git' --exclude='.svn' --exclude='build*' \
    --exclude='CMakeFiles' --exclude='CMakeCache.txt' \
    --exclude='.cache' --exclude='__pycache__' --exclude='*.log' --exclude='.SRCINFO' \
    --exclude='*.pkg.tar.*' --exclude='*.src.tar.*' \
    CMakeLists.txt LICENSE README.md CHANGELOG.md CONTRIBUTING.md PERFORMANCE.md \
    .clang-format .gitignore \
    src qml assets tests scripts packaging
read -r checksum _ < <(sha256sum "$archive")
sed -e "s/@PKGVER@/$version/g" -e "s/@SOURCE_SHA256@/$checksum/g" \
    "$root/packaging/arch/PKGBUILD.in" > "$stage/PKGBUILD"
printf 'Prepared local package sources: %s\n' "$stage"
cd "$stage"
makepkg --verifysource
if [[ $mode == --prepare-only ]]; then
    printf 'Build without installing: cd %q && makepkg\n' "$stage"
    exit 0
fi
export CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL:-2}
makepkg -si
printf '\nInstalled through pacman. Quit any running development copy, then launch Focusboard from KDE or run: /usr/bin/focusboard\n'
printf 'Enable Launch at login from the tray menu. Package files remain in: %s\n' "$stage"
