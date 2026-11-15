# Missile smoke temporal interpolation — 2026-11-15

The64-frame smoke flipbook was sampled with `floor(frame+variation)` only.
A typical8-second puff traverses roughly56frames: its shape changes about7times
per second even when particle positions and the scene update smoothly.
Spatial bilinear filtering does not interpolate consecutive animation frames.

The shader now blends adjacent independently clamped atlas cells in premultiplied
RGBA before calculating material detail/opacity. The last frame clamps to itself;
7→8tile transitions cannot sample across neighboring cells. This adds one bilinear
texture lookup per smoke fragment, not another pass, particle or texture.

## CPU evidence

From any working directory:

```sh
python3 docs/evidence/missile-vfx-temporal-20261115/check.py
```

Uses the repository's actual shader and
`src/assets/cluster-vfx/WispySmoke02_8x8.png`, with Python Pillow and NumPy.
For a sparse checkout, `--atlas /path/WispySmoke02_8x8.png` accepts a materialized
copy only with SHA256
`8833f2a05ffb8bb2214fae15c1e5c4fc9e04b2b5b7c7bfdc7589b72b1a0418e9`.
No atlas is duplicated into evidence. Missing assets/dependencies yield explicit
SKIP and exit77, never an actual-pixel PASS.

At60Hz for8seconds, the actual atlas alpha replay had424/480stationary intervals
before interpolation and0after. Mean alpha jump across frame20 at±1µs changed
from0.0116797 to0.000000143. Cell-bound checks include7→8andframe63.
The test reproduces shader arithmetic on CPU, not a compiled MSL invocation.

## Risks and open acceptance

Temporal blending can soften rapid shape changes. GPU cost and visible quality
need native dense-salvo/closeup/slow-motion comparison. No Metal build, FPS result
or visual acceptance is claimed. This repair removes temporal stepping but does
not invent spatial detail: the original atlas still has128×128pixels per frame.

## Shared missile smoke / impact clock

The old ship0-ms tick supplied zero while ordinary ordnance supplied rounded milliseconds. Both feed the once-per-frame backend gate, which advances missile plume age and cluster impact age. The ship owner now supplies existing `core.GetVisualDeltaTime()` every frame, including sub-ms slow motion; duplicate integer tick removed. Backend gate and draw-only zero fallback remain unchanged. Actual extracted bridge/owner with real TIMER passes120 advancing frames at0.1x, duplicate-ship deduplication, pause and sea reset (`python3 .../clock_probe.py`). This does not advance ballistics or damage.

## Surface impact integration

The airborne volume already clipped ray integration to its transformed UV cube. Surface explosions instead spent16 steps across the larger spray/shock bounding box and skipped samples outside the actual volume. A center ray has8 usable samples (or4 with the large shrapnel bounds) despite the16-step loop. Surface and airborne paths now use their existing affine UV mappings for both interval clipping and sampling. No asset upscale or larger march ceiling; analytic ejecta/shock layers keep their bounds. `volume_probe.py` checks8/4→16 usable samples plus184 randomized valid intervals and old/new UV equivalence.

This is a CPU mathematical reference, not compiled MSL evidence. Useful texture fetches can increase up to the existing16-step limit; it is not a measured GPU optimization. Higher sampling density can change apparent opacity/detail toward the underlying volume integral. Metal compile and in-game smoke, surface/air explosion, pause and rapid-salvo checks remain user-owned. Spatial source detail128²/atlas frame and64³/volume is unchanged.
