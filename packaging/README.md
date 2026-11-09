# Packaging

`../scripts/install-arch.sh` snapshots the source, inserts its version and SHA-256 into `arch/PKGBUILD.in`, then runs `makepkg -si`. Package files are staged under `build-arch/`, which Git ignores. To inspect a package without installing:

```sh
./scripts/install-arch.sh --prepare-only
cd build-arch/package.*  # choose the printed directory
makepkg --verifysource
makepkg --packagelist
makepkg
pacman -Qlp ./focusboard-*.pkg.tar.zst
```

The package contains the executable with embedded QML, a KDE launcher, the scalable icon and six PNG sizes, documentation, and the license. It contains no user data and its package hooks do not enable autostart. After successful `makepkg -si`, the **local installer script** asks in an interactive terminal and records the user's answer without sudo. Existing preferences are preserved. Noninteractive installs do not prompt or change autostart. AUR users get an optional first-launch prompt in the gadget when no preference exists.

`aur/focusboard-git/PKGBUILD` uses the public GitHub repository. Its `.SRCINFO` reflects the current committed version; regenerate it with `makepkg --printsrcinfo > .SRCINFO` after finalizing the upstream commit and before AUR submission. Check [AUR name availability](https://aur.archlinux.org/packages/focusboard-git) first. If available, clone the AUR package repository, copy `PKGBUILD` and `.SRCINFO`, test with `makepkg -si`, then commit and push there manually. Do not overwrite another maintainer's package. After publication, users can run `yay -S focusboard-git` or clone the AUR Git repository and run `makepkg -si` without a helper.

For a release, update CMakeLists.txt and CHANGELOG.md, commit, tag `v<version>`, and push only after review. The tag workflow builds and tests, then creates a **draft** GitHub release with the Arch package. Review the draft and publish it manually. Packaging-only revisions increment release `pkgrel`; do not change historical app versions for that purpose.
