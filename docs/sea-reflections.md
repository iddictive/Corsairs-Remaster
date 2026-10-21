# Sea reflection motion

## Authored sky and distant haze

The shared directional fog field samples the actual current/next and storm sky
textures, including their rotation, elevation, mirrored lower sky and camera
offset. Distant islands therefore approach the panorama behind them rather than
one uniform weather color. The visible sky and this field share one coordinate
mapping in `authored_sky.hpp`; local-volume extinction and field resolution keep
their existing owners.

An actual Metal component replay accepts direction, elevation, rotation, both
blends, absent-source fallback and mirrored-camera cases.200,027 coordinate
comparisons with the previous sky mapping have maximum UV error5.96e-08.
Warmed32-dispatch batches measure0.012285→0.041363ms median per field dispatch;
this is not a complete scene timing. [Retained measurements and probe sources](evidence/sea-dawn-contact-20261021/manifest.json)
do not close the reported foreground horizon cut or the duplicated solar source.

## Authored solar direction and late glare

Native visual sunlight uses one coherent daylight source cached from the loaded
panorama; current/next/storm blends and rotation drive the disc, lighting and
road together. Existing day eligibility and physical elevation/radiance gates
remain. Dark or invalid sources suppress a replacement sun, while the moon
retains its previous visual vector.

The late halo uses existing depth testing at far viewport depth, preserving
foreground rigging and distant opaque geometry while retaining clear-sky glare.
[Source, native negatives and paired full-consumer measurements](evidence/sea-dawn-contact-20261021/solar/README.md)
retain timing tails and limits; installed player-scene acceptance remains separate.

## Sun-road occlusion

`SUNGLOW::DrawReflection` samples the authored sky's solar transmission for its
current reflection camera. It retains its own geometry trace and solar-elevation
fade, independently of the main camera's `fFadeout`: a near ship hiding the
visible sun must not extinguish the entire reflection. Cloud transmission,
moon gain and sun-road cube cadence retain their existing owners. An installed
scene replay is still required to accept the visible reflection and horizon.

## Ship interior water exclusion

**Disabled by the player's explicit October18 instruction.** Only exact
`STORM_METAL_SEA_HULL_CUTOUT=1` enables the old diagnostic exclusion; absence,
zero and malformed values leave the full sea intact. No replacement cutout or
new water API is shipped. The rejected exact-clipping candidate and raw fleet
comparison remain in the [follow-up evidence](../.local-archives/metal-renderer-evidence/destruction-player-followup-20261018/receipt.json).
The prior implementation and comparison below are historical, not the default.

The separately authorized ship-interior-water scope contributes one frozen
`sea.cpp` change to the shared installed batch. SEA obtains each retained hull
XZ contour through the existing SHIP collision owner, selects nearby sea blocks
and culls triangles whose centroid lies inside it after wave generation. Dead
fully submerged hulls stop supplying that contour. The original fragment shader
has no additional ship loop; `STORM_METAL_SEA_HULL_CUTOUT=0` is a diagnostic off
switch. This is mesh-resolution exclusion, not exact clipping at every hull edge.

A serial same-executable8-ship1920×1080 comparison (256 frames,96 warmup) gives
off/on CPU preparation medians10.189/10.035ms, completion cadence10.304/10.095ms
and GPU medians8.213/7.840ms;316 sea triangles are removed. No regression is
measured in this fixture. Both arms report zero complete shadow cubes and use
intact ships, so this cannot accept full-scene FPS or water through a real breach.
[Inputs, raw traces and summaries](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/sea/receipt.json)
retain the comparison. Root owns the integrated build/stage; player breach replay
and hull-edge appearance remain unresolved in runtime.md.

The player reports ship/shore reflections changing rapidly with a stationary
camera. Native `fTmp` advances one 64-slice normal-volume cycle per second;
authored physical wave phases advance 1–8 slices per second. The two rates are
independent. This establishes a temporal-rate mismatch, not the cause of every
visible reflection change.

## Owner and contract

The October15 performance request explicitly permits coarse water reflections:
fine vessel details may be omitted while the hull, masts, sails and large-scale
silhouette remain recognizable. This does not authorize lowering main-view
geometry, changing collision/damage geometry, delaying capture faces or accepting
the unresolved reflection jitter. `metal-sea-benchmark.md` owns the measured
geometry population and the current diagnostic LOD candidate.

`renderer-rain-weather.patch` publishes existing physical phases
`fFrame1 / FRAMES` and `fFrame2 / FRAMES` in vertex constant 2.Y/Z, with W=1 as
the native marker. X retains original bump/foam animation; rain constant 244
retains its existing `fTmp`. `sea_shaders.hpp::modernSeaBumpPhases` consumes Y/Z
only for the marked normal volume. Modern open-sea body, additive SunRoad and
SimpleSea share that helper. Original-fidelity entries, preblended 2D fallback
and unmarked fixtures retain the original phase. No new clock, temporal history,
lower cubemap cadence or flattened ripple amplitude is introduced.

The dynamic ship/shore cube retains complete same-frame face updates;
`sea-reflection-budget.patch` only amortizes the separate distant sun-road cube.
Rolling dynamic faces and alternate-frame refresh remain rejected because they
introduce torn or lagging ship reflections (historical evidence in
`metal-frame-pacing.md` and `runtime.md`).

## Evidence and acceptance

### October14 — displaced reflections on water

The player specifies spatially detached reflections on water. An independent
mirrored-geometry oracle accepts flat modern Sea2 (six markers, three cameras,
maximum0.493px centroid error). Modern SimpleSea/Sea3 instead interpolates
already-divided per-vertex projection coordinates and carries an old vertical
bias. Its modern fragments now derive projection UV from fragment position and
the viewport; original-fidelity entries remain unchanged.

Default Sea2 additionally samples finite objects from one mirrored-camera cube
origin rather than the physical water point. Tilted-normal component scenes
reproduce3–9px displacement. A hit-only screen-space experiment was rejected:
it left the old displaced cube geometry as a second image and failed when the
source geometry was outside the main view.

The existing same-frame capture now retains background after SEA_REFLECTION
(sky/cloud/lightning) and radial depth after SEA_REFLECTION2 (finite geometry).
`CubeTexture` owns an optional R32F depth cube/array view and RGBA8 background
cube, about3MiB at256². No scene rerender, CPU readback or temporal history is
added. `modernSeaCubeReflection` traces the physical reflected water ray through
that depth, samples corrected finite hits and reveals captured background on a
miss. Modern Sea2 ordinary/aim/fog variants share this owner. Original water,
foam and SunRoad retain their prior functions.

The backend publishes only a matching complete face set with the same frame,
origin, projection, target, viewport and depth surface. Above-water face mask
omits negative-Y. Rebegin clears prior masks; Present, Reset and graphics changes
expire old captures. Invalid/missing captures retain the original cube path.
A single depth layer cannot recover geometry hidden from the capture origin.

Current-source Metal component evidence: eight tilted markers, translated/yawed
cameras and a steep downward view have maximum0.154px centroid error. The actual
exported native bridge converts depth104.885582 versus independent104.885544,
preserves background RGB4/8/12 beneath finite red geometry and moves the corrected
reflection toY72.066 (oracle72). Present expiry and actual Device Reset restore
the legacyY67, proving stale captures are rejected. Seeded complete→incomplete
and waterline face sets, wrong target, missing depth and sky-only capture pass.
Existing rebuilt original/modern sea and depth-water probes also pass.
This proves source/component behavior, not installed player placement or FPS.

The initial final-stage attempt refused while the player's app was running.
The subsequent direct-source batch passes canonical `run.sh --stage-only` and
installs signed engine `987efaf34220f0d0b25bb94bd5e5709e359fa457ca7997fe96619ef0cb7fd74e`.
Player acceptance remains the real ship/shore scene while moving the camera,
then holding stationary, with both small and sloping waves. This graphics task
performs no autonomous game launch/input/SAVE inspection.

### Reopened camera-stop report

The player rejects `cad4858` as a jitter fix: ship/shore reflections still
jitter while moving the camera and after stopping it, at ordinary 1x speed
with Phonk off. The installed engine was independently rechecked as `cfe07b6e`.
The phase change's component evidence remains valid; player acceptance is OPEN.

A disposable GPU fixture uses the current `modernSeaCubeNormal`, normals from
all 64 shipped 128-by-128 height frames, a high-contrast 256-face cube, and 90
frames at 120 Hz: 15 camera-moving frames followed by a stationary camera while
wave phases continue. Single-level sampling has maximum channel step 17;
Metal's built-in mip filtering reduces it to 6. Neither arm shows an abrupt
over-24 step in this fixture. This does not reproduce the player's discontinuity
and does not justify a mip-filter implementation or a blur-based acceptance.
The native API probe generates cube and 3D mipmaps successfully; no resource,
shader or engine input was changed by this experiment. Metal's supported route
is [generateMipmaps(for:)](https://developer.apple.com/documentation/metal/mtlblitcommandencoder/generatemipmaps%28for%3A%29).

Checked source preserves world-space UV before camera-relative subtraction and
restores the reflected view/projection. However geometric normals animate the
UV displacement by `normalXZ * 5.3`; the phase repair does not remove that
independent spatial modulation. The dynamic cube has one level and omits its
negative-Y face above water, while tilted modern normals can reflect a grazing
eye into that face. Neither observation alone proves the actual scene's cause.

A separate concrete classification defect remains in `SEA::Realize`: ordinary
water calls `WaveXZ(vCamPos.x, vCamPos.z && bStarted)`, so the second coordinate
is 0 or 1 rather than the camera's Z. Its event branch uses actual Z. This can
change underwater classification and sun-road reprime while waves continue
after camera stop, but only if camera height overlaps the wave surface; camera
above the maximum height is the nearest unaffected case. Actual scene camera
height and mode transitions are still missing, so this is a conditional cause,
not an accepted jitter repair.

The next decisive evidence is the installed ship/shore scene during camera
movement followed by a stationary hold, distinguishing pixel aliasing from
underwater mode changes or changed cube/background coverage. No second native
repair is accepted by these source/component observations.

### Phase-rate component

The exact ordered stack reverse/replay and canonical native build pass.
Explicitly rebuilt `sea-probe` passes original and modern texture/Fresnel/night/
foam/cube-direction/unsupported cases. A disposable real-Metal kernel compiles
the full current sea MSL and accepts marked phases and both fallbacks. A 64-slice
sinusoidal volume sampled over 120 frames at 120 Hz has total variation 0.223287
at the physical 6-slice/s cadence versus 1.578761 at the old 64-slice/s cadence.
This is a temporal component fixture, not a scene or FPS result.

Rejected loop hypothesis: 64 shipped height frames sampled on an 8-by-8 spatial
grid have mean adjacent step 1.85 and wrap step 1.73, with peaks 10 and 6;
there is no exceptional wrap jump in those samples. The first early native
patch insertion conflicted with later rain-patch context; publication belongs
in the existing rain/sea uniform hunk and passes the complete ordered stack.

The accepting replay is the player's ship/shore reflection with stationary
camera, then ordinary camera movement with moving ships. Exact installed batch
and outstanding player replay are recorded separately in `runtime.md`.

The Phonk integration root consumed source hashes `4d43fa4a` / `b4250832`
through the canonical `run.sh --stage-only` combined batch. Installed signed
engine SHA256 is `cfe07b6eac1216c3c4adea8ee693c65ccc8cc5603921ff09a68e753410a4172b`;
unsigned combined engine is `f4eeaf4cb1e90f22d489ce7df39ea754f425259ce1ab3a14d986b8cb8a064e98`.
The canonical transaction verified the app signature and preserved 502 protected
SAVE/archive/settings hashes. Player scene acceptance remains unresolved.
