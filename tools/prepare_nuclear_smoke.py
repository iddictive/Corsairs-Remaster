#!/usr/bin/env python3
"""Embed the canonical RGBA smoke sprite in the offline native renderer."""
import base64
from pathlib import Path
import struct
import sys

source, destination = map(Path, sys.argv[1:])
data = source.read_bytes()
if data[:8] != b'\x89PNG\r\n\x1a\n':
    raise SystemExit('Expected PNG source')
width, height, depth, color = struct.unpack('>IIBB', data[16:26])
if width != height or width > 1024 or depth != 8 or color != 6:
    raise SystemExit('Expected square RGBA8 PNG up to 1024 pixels')
encoded = base64.b64encode(data).decode('ascii')
destination.write_text('#pragma once\nnamespace storm_metal {\n'
    '// Generated from assets/nuclear-smoke.png by tools/prepare_nuclear_smoke.py.\n'
    'inline constexpr char nuclearSmokePNG[] =\n'
    + ''.join('"' + encoded[n:n+120] + '"\n' for n in range(0, len(encoded), 120))
    + ';\n}\n')
