# Contributing

Focusboard targets KDE Plasma 6 on Wayland. Keep changes small, preserve Markdown storage and settings paths, and add a regression test for each confirmed bug. Please open an issue before introducing a new dependency or changing the window integration.

```sh
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel 2
ctest --test-dir build-debug --output-on-failure
/usr/lib/qt6/bin/qmllint -I build-debug qml/Main.qml qml/components/*.qml
desktop-file-validate build-debug/focusboard.desktop
git diff --check
```

For desktop behavior, run the build in a real KDE Plasma Wayland session and check visibility on the exposed desktop, occlusion by normal windows, moving, resizing, tray recovery, and autostart. Headless tests cannot verify KWin stacking.

Do not commit build trees, packages, user documents, or generated logs. Version changes belong in CMakeLists.txt and CHANGELOG.md; increment the release PKGBUILD `pkgrel` for packaging-only revisions. Submit a pull request with the behavior changed, tests run, and any remaining manual checks.
