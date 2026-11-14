# High-altitude sea rendering, 5 November 2026

Follow-up for large square foam patches: [foam-pixel-20261105](foam-pixel-20261105/README.md).

The installed game report describes flickering during high flight; the exact
in-game frame is not reproduced here. The isolated `sea-fleet-benchmark` uses
the real SEA and Metal paths, eight fixture ships and a camera moving 2 m per
frame. It does not operate the game or prove gameplay acceptance.

At 3,800 m, looking down, the old LOD omitted 1–4 sea blocks per frame when the
65,500-vertex limit was exceeded. The candidate applies the same altitude term
to block selection and geomorph: no blocks were omitted in the sampled 16
frames. At 40 m the before/after 1280×800 image SHA-256 is identical
(`b85fa548d698f2d83ad14d88318c0788daf9735504f4a75d62171ef48d399448`).
`base.png` and `new.png` retain the high-altitude scene; the blue square missing
from the candidate is visible in the baseline. Texture-aliasing remains in
those images and is a separate material issue.

Disabling only the foam pass removed the high-frequency grid (`nofoam.png`).
The final material instead blends the subpixel texture towards a 16-sample
texture-derived average, preserving its large-scale coverage. The earlier
16-sample pilot reduced the average absolute channel change between two moving
frames from 0.854 to 0.461 out of 255; `standard-16/17-*` and `mean-16/17-*`
retain the paired CSV inputs. `hires-base.png` and `threshold.png` show the
modern-water before/after at 4112×2580 and 2,000 m. A 60-frame warm interval of
that 80-frame run measured median GPU 5.60/5.16 ms and CPU 8.13/8.00 ms; this
short sequential comparison does not establish a performance improvement or
represent a battle. At 1280×800 and 2,000 m, the corresponding modern-water GPU
medians were 3.59/5.90 ms; the additional cost is material and varies with
coverage and resolution. No claim is made about nuclear-effect combinations,
other antialiasing modes, or actual flight FPS.

Inputs: dirty checkout at `8db8ed848b754d8b02840e5cf13d5926c3542e2b`,
Apple M5 Max, selected Command Line Tools SDK, original/modern water as noted.
`baseline-sea.cpp.txt` and `baseline-sea-geomorph.h.txt` preserve the exact
pre-fix source used for the isolated rebuild; the compared shader baseline and
fixture are retained in `../nuclear-foam-20261105/footprint-antitile-20261105/`.
`candidate-sea-shaders.hpp.txt` is the candidate shader snapshot. Backend hash
at capture was `a92e1d19bf2f68e0fbce9bf18c8a4079fe2f1c5a25f1dcd511e04b9282b02385`;
the preserved comparison requires the corresponding backend and build libraries.

`altitude-probe.py` recompiles SEA and the driver into task scratch without
writing shared build outputs. It requires the prepared native build and fixture,
the installed application resources, and an allocated `CODEX_SCRATCH_DIR`.
Example for the two 4K cases, each in its own scratch run:

```sh
STORM_METAL_MODERN_WATER=1 CANDIDATE=1 FOAMFIX=base PROBE_FRAMES=80 PROBE_HEIGHT=2000 PROBE_WIDTH=4112 PROBE_VIEW_HEIGHT=2580 /Users/md/.codex/bin/codex-run-scratch run --max-bytes 2147483648 --max-files 30000 --seconds 300 --reserve-bytes 10737418240 -- python3 docs/evidence/sea-high-altitude-20261105/altitude-probe.py
STORM_METAL_MODERN_WATER=1 CANDIDATE=1 FOAMFIX=prod PROBE_FRAMES=80 PROBE_HEIGHT=2000 PROBE_WIDTH=4112 PROBE_VIEW_HEIGHT=2580 /Users/md/.codex/bin/codex-run-scratch run --max-bytes 2147483648 --max-files 30000 --seconds 300 --reserve-bytes 10737418240 -- python3 docs/evidence/sea-high-altitude-20261105/altitude-probe.py
```
