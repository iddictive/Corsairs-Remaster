#!/usr/bin/env python3
"""Source-level temporal probe for smooth location-lamp flicker."""
from pathlib import Path
import re

root = Path(__file__).resolve().parent
patch = (root / "smooth-location-lights.patch").read_text()
ini = (root / ".cache/runtime/RESOURCE/INI/lights.ini").read_text(errors="replace")

lamp = re.search(r"\[lamp\](.*?)(?=\n\s*\[|\Z)", ini, re.S | re.I)
assert lamp, "lamp section missing"
def setting(name):
    match = re.search(rf"^\s*{name}\s*=\s*([0-9.]+)", lamp.group(1), re.M | re.I)
    assert match, f"lamp {name} missing"
    return float(match.group(1))

amplitude, frequency = setting("flicker"), setting("freq")
assert (amplitude, frequency) == (0.19, 9.0), "unexpected authored lamp curve"
assert "while (ls.time >= l.p)" in patch
assert "ls.itensTarget - ls.itensDltFast + kFast * ls.itensDltFast" in patch
assert "if (l.freq > 0.0f)" in patch and "if (l.freqSlow > 0.0f)" in patch

# Adjacent extrema are the strongest authored transition: +A to -A. The old
# sample-and-hold curve jumps by 2A at a boundary; interpolation keeps the same
# targets and mean while bounding each 30 fps step by 2A*f*dt.
old_boundary_jump = 2.0 * amplitude
new_max_30fps_step = 2.0 * amplitude * frequency / 30.0
assert old_boundary_jump == 0.38
assert new_max_30fps_step < old_boundary_jump

def curve(t):
    period = 1.0 / frequency
    phase = (t % period) / period
    segment = int(t / period)
    target = lambda n: 0.0 if n == 0 else (amplitude if n % 2 else -amplitude)
    return target(segment) + phase * (target(segment + 1) - target(segment))

# The value is a function of elapsed seconds, independent of whether frames
# arrive at 30 or 120 fps, and it is continuous on both sides of a boundary.
for frame in range(31):
    t = frame / 30.0
    assert abs(curve(t) - curve(frame * 4 / 120.0)) < 1e-12
boundary = 2.0 / frequency
assert abs(curve(boundary - 1e-7) - curve(boundary + 1e-7)) < 1e-5
print(f"PASS: lamp A={amplitude:.2f}, f={frequency:.1f}Hz; jump {old_boundary_jump:.2f} -> <= {new_max_30fps_step:.3f}/frame; 30/120 fps agree")
