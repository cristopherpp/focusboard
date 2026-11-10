#!/usr/bin/env python3
"""Wrap the existing SVG-derived PNG exports in a multi-resolution ICO."""
from pathlib import Path
import struct

icons = Path(__file__).resolve().parent.parent / "assets" / "icons"
sizes = (16, 32, 48, 64, 128, 256)
images = [(icons / f"{size}x{size}.png").read_bytes() for size in sizes]
offset = 6 + 16 * len(sizes)
directory = struct.pack("<HHH", 0, 1, len(sizes))
for size, data in zip(sizes, images):
    directory += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(data), offset)
    offset += len(data)
(icons / "focusboard.ico").write_bytes(directory + b"".join(images))
