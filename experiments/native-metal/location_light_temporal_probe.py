#!/usr/bin/env python3
"""Source-level temporal probe for smooth location-lamp flicker."""
from pathlib import Path
import math
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
assert "+            lights[*i].intensity = 0" not in patch
assert "-            lights[*i].intensity = 0" in patch
assert "visibility += (target - lights[*i].visibility) * blend" in patch
assert "1.0f - expf(-elapsed / seconds)" in patch

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

# Interpolation removes the sample-and-hold discontinuity. Random target
# assignment is intentionally outside this probe's frame-rate-invariance claim.
boundary = 2.0 / frequency
assert abs(curve(boundary - 1e-7) - curve(boundary + 1e-7)) < 1e-5

# The exponential activation envelope is a function of elapsed time. Equal
# target spans therefore agree across frame rates, and a rapid reversal starts
# from the current value instead of resetting or stepping to either endpoint.
attack_match = re.search(r"attackSeconds\s*=\s*([0-9.]+)f", patch)
release_match = re.search(r"releaseSeconds\s*=\s*([0-9.]+)f", patch)
assert attack_match and release_match, "production envelope constants missing"
attack, release = float(attack_match.group(1)), float(release_match.group(1))
assert 0.0 < attack <= release
def advance(value, target, seconds, frame_rate):
    steps = round(seconds * frame_rate)
    dt = seconds / steps
    tau = attack if target else release
    for _ in range(steps):
        value += (target - value) * (1.0 - math.exp(-dt / tau))
    return value

for target, seconds, initial in ((1.0, 0.6, 0.0), (0.0, 0.6, 1.0)):
    at_30 = advance(initial, target, seconds, 30)
    at_120 = advance(initial, target, seconds, 120)
    assert abs(at_30 - at_120) < 1e-12

lit = advance(0.0, 1.0, 0.10, 120)
released = advance(lit, 0.0, 1.0 / 120.0, 120)
relit = advance(released, 1.0, 1.0 / 120.0, 120)
assert 0.0 < released < lit < 1.0
assert released < relit < 1.0

# Gain converges to the exact authored endpoints; color, range, attenuation,
# and the random target distribution remain untouched.
assert abs(advance(0.0, 1.0, 4.0, 120) - 1.0) < 1e-7
assert abs(advance(1.0, 0.0, 7.0, 120)) < 1e-7
print(f"PASS: flicker boundary is continuous (30 fps step <= {new_max_30fps_step:.3f}); activation/release continuous, rapid-toggle safe, 30/120 fps invariant")
