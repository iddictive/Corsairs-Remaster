# Synthetic headless renderer depth replay — 2026-11-13

This is an independent diagnostic, not a game capture or performance test. It
reuses the project's `storm_metal` fixed-function renderer and the configured
offscreen-probe build commands. A temporary copy of `backend.mm` is adapted to
allow a null window and never creates an SDL view; no game process, save,
installation, GUI, production build or game assets are accessed. The normal
fleet runner is unsuitable here: it creates a hidden SDL window and reads the
installed application's resources and user settings. The cannon-darts fixture
uses the same engine bootstrap and requires those real resources; its scene is
not run in this synthetic replay.

At 256×256, a synthetic coloured island pair is 8 km away, with a 0.2 m depth
separation, a nearer ship-shaped silhouette and a flat water-coloured plane.
These are simple opaque triangles through the renderer's fixed-function path,
not the engine's sea, ship or island entities, shaders, materials or aim drape.
There is no actual water snapshot/refraction, MSAA or postprocessing. Each of
21 frames advances the camera 0.01 m and changes projection scale by 0.001
(initial scale 6). For each identical frame, only the island pair's draw order
is swapped. The ordinary between-frame pixel difference is counted separately.
An exact coplanar pair is a negative control; its order dependence is expected
under either projection and does not by itself demonstrate precision loss.

On Apple M5 Max, Apple clang 21.0.0, SDK 27.2, depth32, near 0.115 and far
32000, two runs produced identical output:

| Pair / projection | Order-dependent pixels, sum across frames | Peak per frame | Changed between frames, sum | Changed and order-dependent, sum |
| --- | ---: | ---: | ---: | ---: |
| 0.2 m / forward | 60,702 | 6,770 | 40,957 | 20,196 |
| 0.2 m / reverse | 0 | 0 | 622 | 0 |
| Coplanar / forward | 141,600 | 6,770 | 679 | 57 |
| Coplanar / reverse | 141,600 | 6,770 | 679 | 57 |

Frame-zero water and island coverage was 25,088 and 16,196 pixels respectively;
at least 316 ship pixels appeared in every frame. Counts are pixel occurrences,
not distinct locations. Paired-order differences separate depth-order sensitivity
from motion alone; the 622 changed pixels in the reverse run are not proven all
to be raster edge movement. This reproduces an order-dependent defect in a
controlled close-depth scene, not the reported gameplay flicker or a complete
reverse-Z fix. The drawn ship is separated from the far island, so it does not
test near-coplanar ship/island overlap. No aim/drape or real water path is tested.

Reproduce using an existing configured native build; the script derives compile
and link flags from `ninja -t commands` and writes all build outputs into an
empty scratch directory. It does not invoke a project build or the fleet runner.
The archive may be from a different source snapshot, and the copied backend is
compiled from current source, so record both identities before comparing runs.

```
scratch=$(mktemp -d /tmp/corsairs-offscreen.XXXXXXXX)
python3 docs/evidence/island-flicker-20261105/offscreen-synthetic-20261113/run.py "$scratch"
```

Source checkout HEAD `cf9a436aef92225023517cbe9a608067bad5c46d` with existing
dirty changes. SHA-256 backend source
`6a8bd71d59470ae88ac8bd9d765b4a31fe2ac41ab1e56a7a1104c42e9d6fdd95`;
linked existing `libstorm_metal.a`
`5ff3467901649fc476f72aac7b1e8131b71ac4aac4799c67b40dd7be9a8d56a4`;
scene source `d47e3e662f5616210e67cb980d73e78ffb7ca80ad4919e04eae10b66c2f2541d`.
