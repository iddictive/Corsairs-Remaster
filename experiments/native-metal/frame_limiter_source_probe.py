#!/usr/bin/env python3
from pathlib import Path
import sys

source = Path(sys.argv[1])
main = (source / "src/apps/engine/src/main.cpp").read_text()
header = source / "src/apps/engine/src/frame_limiter.hpp"

assert header.is_file(), "frame limiter helper was not delivered to the engine source"
assert "frameLimiter.NextFrame" in main
assert "std::this_thread::sleep_until" in main
assert "1000u / dwMaxFPS" not in main
assert "if (dwNewTime - dwOldTime < dwMS)" not in main

print("PASS: engine uses sleep_until with the precise limiter; integer-ms busy loop is absent")
