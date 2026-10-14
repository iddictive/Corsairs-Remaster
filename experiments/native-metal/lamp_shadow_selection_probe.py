#!/usr/bin/env python3
"""Compile the tracked engine selector header."""
from pathlib import Path
from tempfile import TemporaryDirectory
from subprocess import run

root = Path(__file__).resolve().parent
headers = root / "engine/src/libs/location/src"
with TemporaryDirectory() as directory:
    executable = Path(directory) / "probe"
    run(["/usr/bin/clang++", "-std=c++20", "-Wall", "-Werror", "-I", str(headers), str(root / "lamp_shadow_selection_probe.cpp"), "-o", str(executable)], check=True)
    run([str(executable)], check=True)
print("PASS lamp influence/view overlap, stable membership across movement, reset, deletion and eight-slot pressure")
