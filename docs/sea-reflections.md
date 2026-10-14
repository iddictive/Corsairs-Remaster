# Sea reflection motion

The player reports ship/shore reflections changing rapidly with a stationary
camera. Native `fTmp` advances one 64-slice normal-volume cycle per second;
authored physical wave phases advance 1–8 slices per second. The two rates are
independent. This establishes a temporal-rate mismatch, not the cause of every
visible reflection change.

## Owner and contract

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

The combined canonical native build passes. Final `run.sh --stage-only` refuses
while the player's app is running; the successor is not installed. The parent
graphics task owns that next stage after normal game exit. Player acceptance is
still the real ship/shore scene while moving the camera, then holding stationary,
with both small and sloping waves. No autonomous game launch/input/SAVE inspection
was performed.

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
