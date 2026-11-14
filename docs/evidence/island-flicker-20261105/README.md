# Island flicker investigation

Additional independent [synthetic headless renderer replay](offscreen-synthetic-20261113/README.md)
compares moving-camera draw order with reverse-Z and a coplanar control; it does
not exercise the game scene or actual sea, ship and aim renderers.

## Zoomed distant terrain replay — 2026-11-13

The existing Metal terrain replay was repeated with its actual Nevis mesh and
textures, changing only `material-replay.cpp.txt` in a disposable copy:
`proj._11=proj._22=1` to `6`, and `q.z+=2600` to `8000`. At 256×256 the
standard depth projection produced 243 pixels differing when triangle order
was reversed, with 8858 terrain pixels visible. An isolated candidate using
the same terrain, reverse projection coefficients `.115/(.115-32000)` and
`.115*32000/(32000-.115)`, depth clear 0 and greater-equal comparison reduced
that difference to zero. The front red occluder remained visible in both runs.
The two runs changed 189 and 182 pixels, respectively, after a 0.01-unit
camera translation; those counts include ordinary raster motion and are not a
temporal acceptance metric. The red occluder filled this zoomed viewport, so
it does not test partial silhouette boundaries. Run the existing `replay.py`
against disposable copies with these substitutions and the bundled Python/Pillow
runtime. This isolates a depth-order defect on actual terrain; it does not prove
that all reported game flicker, aim projection, occlusion or full-scene behavior
is fixed. Scene-wide consumers and actual gameplay remain unverified.

The player reports polygon flicker on islands even without explosions. The cause
is unresolved; this replay does not establish that the game is fixed or that
its full scene is stable.

`replay.py` uses the real `CreateGeometry` and `GEOM::Draw` with a capture
service, then submits the captured Nevis terrain triangles, normals, vertex
colours, UVs and material groups to the actual Metal fixed-function renderer.
The four diffuse TX textures use all eight authored mip levels, linear filtering
and wrap; Nevis's actual rockK2 alias is rockA2 and enables the existing
stochastic sampling. The 256x256 fixture uses near 0.115, far 32000, camera
position (0, 200, -2600) and an independently drawn nearer red occluder.
Repeated fixed-camera output is identical; reversing triangle order changes
three pixels, and translating the camera by 0.01 changes 158. These observations
cannot reproduce or rule out the reported flicker.

The capture service substitutes for the normal geometry service and only the
single-texture stage is reproduced. The replay omits NODER/ISLAND traversal,
seabed, weather, dynamic lighting and shadows, water, sky, postprocessing,
scene MSAA, real player camera and installed game. Its elapsed time includes
CPU copies, readback and synchronisation and is not a performance baseline.
A single isolated view cannot generalise to all islands or camera distances.

Run from the repository root with an empty scratch directory, the configured
native build/runtime, and Python with Pillow:

```
python3 docs/evidence/island-flicker-20261105/replay.py <empty-scratch-directory>
```

The exact output is in [result.txt](result.txt). At source revision
`2529d304972e12adda20e8fff1e59030f9e7a9ee` the renderer archive SHA-256 was
`9b6629c8dce5a536100580fa91d4a8ef0541fe3eef5a476a66290b09766f3baf`.
Runtime asset hashes, relative to `experiments/native-metal/.cache/runtime/`:

```
90456f323bead05e8675eb206637c15ddf8a40ea1c4e33dfa28c9b73e87c1b95  RESOURCE/MODELS/Islands/Nevis/Nevis.gm
c2f815a7d39561fd7fec9cc5d778332dc6d82b40f4c1d3ec2fa91cc97b34d4bd  RESOURCE/Textures/jungle_gray.tga.tx
3363912678fa6fd37c4ebc8da654218c8cdb4de91a40e73c0cb22730113437cd  RESOURCE/Textures/rockA2.tga.tx
b34bcdda13bc6a8eeb514f3f33ff24998799ce7ef2e1cc1bd6a3240ca6092d8c  RESOURCE/Textures/rockK4.tga.tx
1f37a991166d28646cbb7a7ed1b2f59999d2532c940627554ccada665f5e10a9  RESOURCE/Textures/beach1.tga.tx
```

Next evidence: capture consecutive full-scene frames during observed flicker,
with camera/view/projection, resident regions, island and seabed transforms,
immersion and distance blend, technique and material/texture draw identities,
depth state, and weather/lighting/shadow state. Compare terrain before
postprocessing with the final frame and isolate the changing input. The user
owns gameplay operation; a source or offscreen replay is not live acceptance.

## Moving-camera depth isolation

`depth-motion-replay.mm` is an independent Metal replay, not the engine or a
game process. It renders two full-screen opaque triangles into a 1x1 RGBA8 and
Depth32Float target, with the renderer's 0.115/32000 perspective range. The
front red triangle is geometrically closer; the back blue one is drawn last.
For 101 camera positions spaced 0.01m apart it counts incorrect blue results,
changes over time, and disagreement when draw order is reversed. It also checks
the nearby 5m/1cm pair and a mixed-projection occlusion negative. It uses GPU
projection arithmetic, a real Metal depth attachment and readback, but does not
replay island meshes, explosions, MSAA, shading, engine shaders or the game.

Compile the standalone source with Apple clang and the Foundation and Metal
frameworks, then run the resulting disposable executable. This is not a project
build or install:

```
scratch=$(mktemp -d "${TMPDIR:-/tmp}/corsairs-depth.XXXXXXXX")
xcrun clang++ -std=c++17 -fobjc-arc -framework Foundation -framework Metal \
  docs/evidence/island-flicker-20261105/depth-motion-replay.mm -o "$scratch/depth-motion-replay"
"$scratch/depth-motion-replay"
/Users/md/.codex/bin/codex-delete-temp "$scratch"
```

On Apple M5 Max, Apple clang 21.0.0, the 2026-11-12 run produced:

```
mode=standard front=5.00 separation=0.01 incorrect=0 order_disagreements=0 temporal_flips=0
mode=reverse front=5.00 separation=0.01 incorrect=0 order_disagreements=0 temporal_flips=0
mode=standard front=2600.00 separation=0.20 incorrect=71 order_disagreements=46 temporal_flips=23
mode=reverse front=2600.00 separation=0.20 incorrect=0 order_disagreements=0 temporal_flips=0
mode=standard front=5000.00 separation=1.00 incorrect=92 order_disagreements=53 temporal_flips=18
mode=reverse front=5000.00 separation=1.00 incorrect=0 order_disagreements=0 temporal_flips=0
mixed_near front=1000 island=2600 front_visible=no
```

The mixed negative uses the normal near plane for the front surface, 10m for
the back island, and one shared depth buffer: the island incorrectly covers
the nearer surface. Changing only island projection is therefore not safe.
Reverse-Z resolves this synthetic failure, but does not establish the cause of
the player report or near-camera compatibility across the game.

The owner-level candidate is a shared scene-wide reverse-Z contract, not an
island-only patch. For the existing row-vector perspective matrix in
`s_device.cpp`, retain `_11`, `_22`, `_34 = 1`, and near/far clipping; replace
`_33 = far/(far-near)` with `near/(near-far)` and `_43 = -_33*near` with
`near*far/(far-near)`. The depth clear must become 0 and less/less-equal tests
must become greater/greater-equal for that same shared scene. The reverse
coefficients must be formed directly, rather than subtracting an already rounded
standard depth from 1. This is a candidate for the renderer owner, not an
applicable two-line production patch: depth clears and comparisons, depth
snapshots, inverse-projection consumers, sky/background sentinels, shadow and
reflection passes and effects must retain mutually consistent conventions.
For example `sea_shaders.hpp` checks depth `< 1` for solar occlusion,
`underwater_medium.hpp` clamps to `.99999`, and `ship_fire_shaders.hpp` uses
direct depth comparisons; `cluster_impact_render.hpp` also reads scene depth.
The integration owner must inventory and adapt actual consumers and verify near
camera, sea, sky, shadow, effects, and depth occlusion in their real paths.
Until then there is no safe in-scope production patch or measured full-path
performance result. See also [NVIDIA's depth precision analysis](https://developer.nvidia.com/blog/visualizing-depth-precision/).

## Metal depth contract audit — 2026-11-12

Scope: current tracked and dirty working-tree sources under
`experiments/native-metal`, including Metal headers and engine consumers. Search
for `reverse.?z`, `depth2d`, depth reads, comparison/clear/resolve state,
projection writers, and clip-Z tests found no implemented main-scene reverse-Z
mode. This is a source inventory, not a runtime trace; a generated shader,
unsearched external asset, or changing concurrent work can add a consumer.

| Owner | Existing contract and candidate change |
| --- | --- |
| `engine/src/libs/renderer/src/s_device.cpp` | `SetPerspective` builds forward 0..1 depth; `SetCamera` rebases world translation. For a scene explicitly marked reverse-Z, build `_33=n/(n-f)`, `_43=n*f/(f-n)` directly, `_34=1`; preserve FOV, aspect, near, far, camera rebasing and XY. Do not globally rewrite `CMatrix::BuildProjectionMatrix`: `sea/src/env_map.cpp`, `shadow/src/shadow.cpp`, UI and world-map code build other projections. Their target/pass needs an explicit convention, with projection and consumer kept paired. |
| `backend.mm` | Main scene selection is `target==back && zbuffer==depth`, with `renderDepth()` selecting the current scene surface, possibly MetalFX-backed. Scope the convention to the actual depth attachment and its lifetime, not merely one draw: clears including partial clear map forward `z` to `1-z`, `less ↔ greater`, `lessEqual ↔ greaterEqual`, equal/notEqual/always/never stay unchanged, and depth/slope bias signs invert for reversed scene draws. Apply consistently in normal/legacy and auxiliary replay encoders, respecting render-state cache keys and viewport semantics. `sceneDepthReady` currently recognizes clear `z==1` at API entry; preserve logical versus physical clear meaning. Do not reverse unrelated depth surfaces or shadow prepasses. Current viewport special cases and pretransformed vertices require explicit compatibility decisions. |
| `scene_msaa.hpp`, `metalfx.hpp` | Scene MSAA resolves minimum today; reversed scene must resolve maximum so the closest sample still wins. The seed and MetalFX copies use `Always` and copy physical depth, so they should preserve physical values, not invert them again. Validate normal and scaled targets, continuation after resolve and rehydration. Apple documents [min and max resolve](https://developer.apple.com/documentation/metal/mtlmultisampledepthresolvefilter?changes=_7_9&language=objc); hardware and deployment availability still need a local compatibility probe. |
| `sea_shaders.hpp`, `sea/src/env_map.cpp` | Main-scene `modernSeaDepthComposite` uses clear 1 and compares foreground with `<`; reverse these only for reversed scene. The inverse projection must correspond to the captured pre-water depth. Solar visibility tests clear 1. Reflection capture is a separate projection/target and its `sea_reflection_depth` clear test must follow that capture's actual convention; radial distances and radial-depth tests are not reverse-Z values. Test refraction, silhouettes, underwater, sun and all six reflection faces. |
| `cluster_impact_render.hpp`, `phonk_ordnance_shaders.hpp`, `phonk_ordnance_render.hpp` | Impacts and nuclear effects read scene depth, test sky against 1, reconstruct with inverse VP and select clip endpoints; pressure comparisons and screen bounds assume near is `clip.z<=0`. With reverse-Z, distinguish sky at 0 and near crossing at `clip.z>=clip.w`, supply matching inverse VP, and reverse raw-depth ordering where used. Preserve offscreen, sky, behind-island and near-plane cases. `cluster_impact_offscreen.mm` is a fixture, not the live owner. |
| `ship_fire_shaders.hpp`, `ship_fire_render.hpp`, `missile_trail_vfx.hpp` | Fire volume treats clip depth 0 as near and 1 as sky; its first-contribution depth uses 1 as empty and compares `front>solid`, and embers directly compare depths. Reverse endpoint, sentinel and comparison together, including the intermediate front texture's clear. Fire bounds use `clip.z<0` for near crossing. Missile trails use a separate LessEqual depth state against scene depth and must select the shared scene convention. |
| `aim_volume.hpp` and `land_shadow.hpp` receiver outputs | First-person owner and identity capture assume a smaller post-draw depth means nearer (`b>=a` rejects), clear=1 means empty, and reconstruct near at 0/far at 1. Invert those assumptions only for the reversed scene; retain relation-depth equality and validate exact clear collisions and ULP-bin reconstruction. `AimContactField` near/far bounds are projector-domain values; inspect their producer before changing them. Ship, terrain, water, own-ship exclusion and airborne volume must remain aligned under zoom. |
| `ambient_occlusion.hpp`, `ambient_occlusion_composite.hpp`, `underwater_medium.hpp`, `volumetric_light_shafts.hpp`, `sky_fog.hpp` | AO tests `>=.999999` for sky and estimates reconstruction error from adjacent depth floats. Underwater clamps/labels near-1 depth as sky. Shafts reconstruct through inverse VP and keep a half-float depth history with a raw `.012` threshold; re-evaluate its temporal behavior. Sky fog explicitly uses 0 as near, 1 as far, and constructs ray direction from those endpoints; reverse both endpoints, clear handling and sky veil together to avoid reversed rays. |
| `land_shadow.hpp`, `point_shadow_registry.hpp`, engine shadow paths | Sun/point maps are independently projected depth maps with their own clear=1, less comparison and PCF bias. Preserve them forward unless each complete shadow producer/consumer is intentionally migrated. Main scene depth-equal receivers and shadow replay state need compatibility verification; do not blanket invert independent maps. |
| Engine techniques, `weather/src/sun_glow.cpp`, custom matrices | Explicit `ZFunc`, negative slope biases, `MinZ=MaxZ=1` for the sky, world-map/dialog projections and pretransformed vertices are convention-sensitive. Define whether these are logical forward API values translated by backend or already physical; avoid translating twice. Mirror depth viewport endpoints for reversed targets where appropriate and prove sky/depth-tested HUD behavior. |

The minimal viable integration is **one coherent scene-depth convention** at
the attachment boundary, plus its actual consumers. It is not a small isolated
patch: producers, consumers, cached pipelines, snapshots and depth transitions
must change atomically. The depth mapping applies to arbitrary projections only
when it is computed before destructive rounding; a post-projection `1-z` in a
vertex/fragment shader can retain the original far-depth cancellation. Preserve
independent forward shadow/reflection domains unless their entire pass migrates.
Non-default viewport ranges, `XYZRHW` and explicit projection setters need
bounded probes before selecting the precise translation boundary.

The following specifies the core mapping for that integration; it is not a
standalone patch and must not be applied without the consumers above:

```cpp
// Scene perspective, at its actual projection producer (row-vector matrix):
mtx._33 = near_plane / (near_plane - far_plane);
mtx._43 = near_plane * far_plane / (far_plane - near_plane);

// At a proven reversed scene attachment, for values received in the old API:
physicalClear = 1.f - logicalClear;
physicalCompare = logicalCompare == CompareLess         ? CompareGreater :
                  logicalCompare == CompareLessEqual    ? CompareGreaterEqual :
                  logicalCompare == CompareGreater      ? CompareLess :
                  logicalCompare == CompareGreaterEqual ? CompareLessEqual : logicalCompare;
physicalDepthBias = -logicalDepthBias;
physicalSlopeBias = -logicalSlopeBias;
// Resolve nearest covered sample with MTLMultisampleDepthResolveFilterMax.
```

The shader-side migration must use the physical convention of the particular
attachment and its matching matrix; do not translate clear, compare, bias or
projection twice. In particular, scene producers cannot flip to reverse-Z
while effects or aim consumers keep the forward convention. A migration flag
or target identity must also participate in cached state and temporal history.

Acceptance for the integration owner: run `depth-motion-replay.mm` plus real
engine offscreen pairs for (1) opaque island/terrain and a nearer ship at
several distances, both draw orders and moving/rotating/zooming camera; (2)
sea before/after water snapshot, shoreline, sun, underwater and reflection
cube; (3) impact/nuclear/fire in front of and behind island/ship, against sky,
including near-plane crossings; (4) first-person deck, rails, aim contacts,
water, zoom and depth-tested HUD; (5) MSAA off/on, resolve/rehydrate,
MetalFX on/off, empty sky, shadow maps, AO and location transition. Include
near clipping, far clipping, depth bias and alternate targets as negatives.
Compare the same representative full-path frame time before/after and retain
visual/depth outputs; the synthetic 1-pixel GPU result is no performance or
gameplay acceptance. User operates the real game; no game process was accessed.
