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

The package contains the executable with embedded QML, a KDE launcher, the scalable icon and six PNG sizes, documentation, and the license. It does not contain user data or enable autostart.

`aur/focusboard-git/PKGBUILD.in` is a template. After publishing this project, replace both URL placeholders with the real upstream URLs, copy it to a separate AUR checkout as `PKGBUILD`, test `makepkg`, and generate `.SRCINFO` with `makepkg --printsrcinfo > .SRCINFO`. No AUR package has been published by this project.
