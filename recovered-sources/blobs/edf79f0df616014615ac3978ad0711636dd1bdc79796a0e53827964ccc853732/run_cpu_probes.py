#!/usr/bin/env python3
"""Replay retained, source-only CPU evidence; binaries are task-local scratch."""
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

evidence = Path(__file__).resolve().parent
root = Path(subprocess.check_output(["git", "-C", str(evidence), "rev-parse", "--show-toplevel"], text=True).strip())
ninja = (root / "experiments/native-metal/.cache/build/build.ninja").read_text()
match = re.search(r"^build .*?/ship_phonk_laser.cpp\.o:.*?\n(?P<body>(?:  .*\n)+)", ninja, re.M)
assert match
values = dict(re.findall(r"^  (\w+) = (.*)$", match["body"], re.M))
includes = shlex.split(values["INCLUDES"])
scratch = Path(tempfile.mkdtemp(prefix="corsairs-debris-replay-", dir="/private/tmp"))
try:
    for source, extra in [("probe.cpp", ["-I" + str(root / "experiments/native-metal")]),
                          ("fleet_fixture.cpp", includes + ["-DSTORM_METAL_SHIP_HULL_DAMAGE=1"])]:
        binary = scratch / source.removesuffix(".cpp")
        subprocess.run(["clang++", "-std=c++20", "-O1", "-g", "-w", "-fsanitize=address,undefined",
                        *extra, str(evidence / source), "-o", str(binary)], check=True, timeout=30)
        subprocess.run([str(binary)], check=True, timeout=30)
finally:
    subprocess.run(["/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp", str(scratch)], check=True, timeout=30)
