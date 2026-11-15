#!/usr/bin/env python3
"""Compile and run the actual ship-fire projection helper (CPU only)."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parents[2]
with tempfile.TemporaryDirectory(prefix="ship-fire-near-clip-") as directory:
    binary = Path(directory) / "probe"
    flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror"]
    if sanitizers := os.environ.get("SANITIZERS"):
        flags += ["-fsanitize=" + sanitizers, "-fno-omit-frame-pointer"]
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + flags +
                   ["-I", str(root / "experiments/native-metal"),
                    str(here / "probe.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
