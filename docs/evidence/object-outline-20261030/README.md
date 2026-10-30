# MSAA foreground outlines at captured water

The native manual-aim water receiver switched the scene from multisampled color
and depth to single-sample integer receiver metadata. Minimum-resolved depth
retained the foreground depth for an entire mixed-coverage pixel; resolved color
still included its uncovered sky samples. Water failed depth testing for that
whole pixel, leaving a bright sky fringe. The previous refraction-shader fix
could not execute for those rejected fragments. Its old probe omitted the actual
`StormMetalBeginAimWater` scope, so it missed this consumer.

The candidate keeps the main water draw and its overlays in the existing MSAA
attachments. A texture-free fragment stamps existing receiver depth and identity
using the same uploaded vertices and indices. That auxiliary pass writes neither
scene color nor scene depth, so subsequent passes retain the original samples.
AA Off/FXAA retain their existing single-draw MRT path. No effects are disabled.

## Native component evidence

`receiver.cpp.txt` uses the real device, shaders and receiver scope. A black
foreground edge moves across four subpixel phases against a white cleared sky
and black water. Under MSAA4 the old scoped path leaves RGB `bfbfbf`, `808080`,
`404040`; the candidate equals the normal MSAA draw exactly, including coverage
alpha, with RGB `000000` at every phase. AA Off remains identical. Actual metadata
readback retains water token `20000000`, color zero and depth bits `3f41f07c`;
foreground and partly covered silhouettes remain excluded. The existing
depth-water fixture passes AA Off/MSAA4, including attenuation, foreground and
alpha rejection, invalidation and weather lighting.

The active graphics record selects mode2, mapped to MSAA4 by `antialiasing.hpp`;
the launcher applies it after legacy engine settings. The screenshot shows
bright edges against water. Whether manual-aim capture was active in that exact
quay frame is not observable from its pixels. This proves the component defect
and repair, not every possible outline on dry-land backgrounds or storm motion.

## Cost and limits

Apple M5 Max, actual hidden-component backbuffer/drawable2056×1329, mode2,
210 interleaved frames, first30 excluded,60 samples per path:

| Path | Median GPU ms | p95 GPU ms |
| --- | ---: | ---: |
| Old single-sample receiver water | 0.5444 | 1.7867 |
| Normal water with intact MSAA | 0.7232 | 2.1331 |
| Candidate water plus metadata stamp | 0.7749 | 2.4089 |

The candidate adds0.2305ms median against the broken receiver path; metadata
stamping adds0.0517ms against intact MSAA. This synthetic scene contains one
water chunk; real vertex counts/chunk counts and other scene work differ.
The p95 values are noisy. Earlier separate-process runs are retained in
`baseline.csv`, `control.csv`, `candidate.csv`; they were too noisy to isolate
stamp cost. `paired.csv` is the stronger same-process comparison. No player FPS,
retina4112×2580 timing, or installed-game acceptance is claimed.

## Reproduction

Base source `a7f9f9ca85c3e3543854ea69518e527bb9fb61a3`; candidate changes
`backend.mm` and `sea_shaders.hpp`. `replay.py.txt` builds private fixture objects
from the configured native compile/link commands, with frozen sources and
disposable readback/old-path instrumentation. It never writes the shared build.
Run it with the repository as its argument and `CODEX_SCRATCH_DIR` pointing to an
existing disposable directory. The receiver output and native command timestamps
provide the assertions and raw timings; `observations.json` preserves this run's
selected values. Temporary instrumentation is not shipped in the runtime.
