#!/usr/bin/env python3
"""Validate CI outputs and prepare the four-file prebuilt tree."""
import hashlib
import json
import pathlib
import re
import sys

source = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
version = sys.argv[3]
commit = sys.argv[4]
if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version):
    raise SystemExit("Invalid project version")
if not re.fullmatch(r"[0-9a-f]{40}", commit):
    raise SystemExit("Invalid source commit")
installer = "Focusboard-Setup-x64.exe"
portable = "Focusboard-Portable-x64.zip"
expected = {}
checksum_file = source / f"Focusboard-{version}-SHA256SUMS.txt"
for line in checksum_file.read_text(encoding="utf-8-sig").splitlines():
    match = re.fullmatch(r"([0-9a-fA-F]{64})  (Focusboard-(?:Setup-x64\.exe|Portable-x64\.zip))", line)
    if not match or match[2] in expected:
        raise SystemExit("Invalid checksum list")
    expected[match[2]] = match[1].lower()
if set(expected) != {installer, portable}:
    raise SystemExit("Checksum list must contain exactly both packages")
packages = {}
for name in (installer, portable):
    path = source / name
    size = path.stat().st_size
    if size <= 0 or size >= 100_000_000:
        raise SystemExit(f"{name}: GitHub Git object limit (100 MB) exceeded or file empty: {size} bytes")
    digest = hashlib.file_digest(path.open("rb"), "sha256").hexdigest()
    if digest != expected[name]:
        raise SystemExit(f"{name}: SHA-256 mismatch")
    packages[name] = {"filename": name, "size": size, "sha256": digest}
measurements = json.loads((source / f"Focusboard-{version}-measurements.json").read_text())
if measurements.get("version") != version or measurements.get("installerBytes") != packages[installer]["size"] or measurements.get("portableZipBytes") != packages[portable]["size"]:
    raise SystemExit("Build measurement metadata disagrees with packages")
out.mkdir(parents=True, exist_ok=True)
for name in (installer, portable):
    (out / name).write_bytes((source / name).read_bytes())
(out / "SHA256SUMS.txt").write_text("".join(f"{packages[name]['sha256']}  {name}\n" for name in (installer, portable)))
(out / "latest.json").write_text(json.dumps({"schemaVersion": 1, "version": version, "platform": "windows", "architecture": "x64", "sourceCommit": commit, "installer": packages[installer], "portable": packages[portable]}, indent=2) + "\n")
print(f"Prepared Focusboard {version} from {commit}")
