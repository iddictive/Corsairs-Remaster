# Sea manual-aim envelope

## Status

The October 7 cannon batch is installed, including the corrected shared event
header; the player reached the current saved sea battle. Action acceptance remains
open for the reported cliff, stern-on ally and sinking ship. A subsequent live
manual-aim profile reopened performance acceptance. Its correction at11571da passed
the canonical build and installed stage; the current signed engine is37598a37.
The player's subsequent ship-aim hold reproduced the performance defect on that
installed engine: 1185/1717 main-thread samples were inside the aiming camera and
the concurrent manual scene showed 50 FPS. Solid-cliff gaps/flicker also remain
open. Source/probe timings do not accept either runtime outcome.

## Contract

First-person deck camera controls aim. One subtle ballistic volume spans every
eligible broadside muzzle. Air density is weaker near the guns and strengthens
modestly downrange. A tiny centered `+` marks the shooting direction.

An invisible rectangular range probe matches the user's approximately 60×34-pixel
annotation at 2048×1285 at 1x, scaled with the viewport. It compensates the actual
renderer magnification so zoom cannot narrow its angular/world support. That
support grows linearly from 1x to 1.5x at maximum 5x zoom, with the same rectangle
aspect. Actual mast/hull/fort polygons
inside that view region supply depth. The target remains on the center camera
ray at that depth: the rectangle does not redirect aim toward an off-center ship.
Selection is instantaneous, with no retained target, tracking or dwell. A nearer
center obstruction wins. Empty or genuinely out-of-range space uses common legal
maximum reach; nearby mechanically unreachable targets stay unavailable.

The visible contact area is an approximate rounded dispersion-density region,
not a guaranteed boundary containing every shell. The same smooth field is
evaluated on current water, hull, mast, terrain and fort depth. Physical first-hit
samples gate where it can appear, including continuation through real openings.
Those visibility masks do not draw their own polygon outlines. A faint interior
and colored rim share one composite; air still uses the stopped ballistic volume.

## Owners and implementation

- `cannon-trajectory-aim.patch` owns the base controller, geometry and GPU bridge;
  `cannon-rake-spread.patch` layers cloth selection, live parameter queries,
  per-gun manual/automatic raking and curved-receiver support
- Canonical `PROGRAM/sea_ai/AIBalls.c` owns height warp and shot dispersion.
  `AICannon.c` supplies their query events and the crew-quality multiplier;
  preview and fire read the same helpers without guessed C++ fallbacks
- `BuildManualAimSolution` shares range selection and per-muzzle eligibility between
  preview and manual fire. `MODEL::Clip` finds real polygons inside the small view
  frustum; pure traces confirm visibility. `SAIL::Pick` also visits exact unrolled
  cloth triangles, excludes the shooter's ship and performs no damage or RNG.
  Eight bounded aperture rays supplement model polygon candidates for cloth.
  Bounding boxes only reject candidates
- `rangeApertureSlope` and renderer `GetCameraMagnification` bind range support to
  the active both-axis camera zoom while retaining ordinary FOV/aspect policy
- The plus uses the current range-source relation. Firing events do not inherit a
  farther center-hit character after a nearer aperture receiver changes the range
- `manual_aim_geometry.hpp` mirrors live projectile warp and supplies exact section
  geometry; `manual_aim_volume_bridge.hpp` defines checked 64/32/16/8-byte GPU records
- `aim_volume.hpp` and `backend.mm` draw one soft-density/contact composite from
  current post-water depth and color snapshots. Per-gun surface triangles are gone
- `ship_surrender.h` supplies live per-ship display state to the controller and
  battle interface. Surrendered aim highlighting and overhead markers use gold
  `0xE6C663`; the marker retains its authored alpha and ordinary depth test
- Contact color and character identity come from actual depth-writing model draws,
  using the existing `GetRelation` mapping. One accumulated depth/token/color map
  captures registered hull/upper-model, fort, intact mast and owned sail/rope/vant
  groups. Exact current depth selects the actual receiver; water stays neutral
- Alpha-tested holes remain holes. Sails/ropes are non-stopping rendered contacts,
  gated by finite airborne support; they never stop a projectile or alter damage.
  Own hull and separately drawn own rigging are excluded by actual ownership
- The contact perimeter adapts its brightness to the underlying scene, with a
  restrained dark outer keyline for midtone/daylight contrast. This does not raise
  nighttime air opacity. Friend/enemy/neutral colors are green/red/yellow; water and
  unknown terrain remain neutral. Identity is not blurred through receiver gaps

## Physics, geometry and bounded work

Camera selection is followed by actual muzzle reach, elevation and traverse checks.
Far intent intersects the camera bearing with every compatible muzzle's legal
range disk and uses an inward numerical margin at the maximum-range boundary.

World queries are pure: no `Cannon_Trace`, damage events or random draws. Model
AABBs reject unrelated receivers; island tracing covers the full `ISLAND_TRACE`
layer. `WaveXZ` refines first crossings, including initially submerged muzzles.

Every eligible gun supplies its nominal trajectory. Spatial support guns add all
eight independent yaw/elevation/speed dispersion corners. Additional contact
samples follow complete trajectories through missed hull/mast gaps to their first
physical receiver. They discover continuation supports, not visible surface tiles.
Contact projectors use stable ship-local longitudinal/up extrema and a middle
gun, with near-tie handling. Active raking also retains the actual first/last gun
targets along the target's longitudinal axis. Each of at most seven density fields contains
65 fixed axial sections derived from that gun's untruncated nominal trajectory and
independent dispersion corners. Each ellipse is inscribed in its sampled support
hull, preventing covariance overhang at close range. Muzzle-relative plane solves and explicit endpoint
insertion keep translation from dropping a field. A fourth-power mean combines
field densities once, retaining isolated regions without per-gun alpha blending.
Receiver discoveries add visibility support only, never a density field.

Each physical-support projector keeps a 4×4 nominal-speed grid and 2×2 grids at both
live speed limits. Twelve optional probes per cell bound refinement (355 maximum
marches per projector; uniform water 131, uniform solid 227). Same-first-water fans
form a retained 2048×2048 coverage atlas. Same-first-solid fans form bounded surface
prisms; their thickness uses all independent corner/center witnesses of the
local launch subcell plus 1.5 cm. Above 0.5 m, one barycenter refinement precedes
a real `MODEL::Clip` mesh query bounded to that fan and its witnessed relief.
Each query accepts at most 128 polygons and retains real normals rather than
clamping relief into a flat slab. Receiver
mismatches are classified in launch space, so an opposite mixed corner does not
erase an otherwise valid child triangle. Current-depth uncertainty is handled as a depth
bin, not arbitrary world expansion. Character prisms require exact receiver tokens.
Mixed receiver samples clip/refine visibility and never gain a line of their own.
Solid contour width uses the symmetric secant of accepted depth neighbors. If no
safe neighbor exists, a current-depth pixel tangent supplies only stroke width;
it does not borrow another surface or expand eligibility. No physical trajectory
is extended past its first stopping impact.

Common downrange sections preserve actual muzzle/contact endpoints, reversed or
turning paths, thin rolled batteries and empty axial gaps. Exact hull-edge planes
from both slab endpoints avoid fixed-direction narrowing/fattening. Internal
station caps do not become visible contact seams. Air alpha remains capped at .22.

Range picking visits at most 4096 clipped polygons and confirms 32 nearest
candidates. GPU input caps are 16 density fields (1040 sections), 8192 contact prisms,
49,152 water vertices, 1024 air sections and 65,536 air side planes. Actual model
registration is capped at 128 receivers. Registry metadata is rebuilt without
rebuilding world model bounds.

Ownership resets on frame/camera changes and preserves later occluders. A model
capture starts only at its first depth-writing draw, so culled, empty and purely
non-depth-writing groups do no capture GPU work. Each visible registered group
still adds one full-depth copy and one depth/token/color pass, in addition to own
ship/water capture, the eligibility masks and full-viewport composite. Native
profiling remains necessary with many ships and rigging groups.

## Manual-aim CPU work

The live October 7 profile of the installed cannon batch spent 1533/1951 main-thread
samples in the manual overlay (78.6%). The dominant leaf work was BSP clipping,
segment/model AABB tests and exact model traces; wave sampling was minor. These
inclusive and leaf counts are different measurements and must not be added.

`AimMarch` now rejects unrelated models once using a conservative full-parabola
bound, including interior coordinate extrema and float roundoff. Original model
IDs/order, per-segment AABB tests, exact `Trace`, water tests and all sampled paths
remain. There is no reuse across frames, so moving hulls, camera and waves cannot
leave stale first contacts. A one-off extracted-function comparison across 1000
seeded scenes retained identical sampled segments, first-hit fraction/point/surface
and end time, including submerged and generic unbounded receivers. Model AABB
checks fell from 5,243,152 to 367,765; this is CPU work, not a measured FPS gain.

`GEOM::Clip` owns a lazy local-space triangle AABB cache. BSP vertices and triangle
indices are populated only during geometry construction; render vertex buffers
and moving model matrices are separate. The cache dies with its `GEOM` owner and
requires no frame invalidation. Plane signs and conservative error thresholds are
prepared once per query. A wholly outside face is rejected only after the stock
first-occurrence dedup mark, before vertex copying/clipping. Sphere, BSP traversal,
callback order and polygon caps remain unchanged. Zero-plane queries retain stock
behavior without building this cache.

Rejected alternative: tightening the search sphere to the exact five-plane fan
preserves uncapped polygon sets but changes first-occurrence order for duplicated
BSP faces. On 640 queries each, it changed the capped 128-polygon set in 24 hull,
four Nevis and one PortoBello cases. This can reintroduce missing coverage and is
not used. The same-traversal reject instead preserved exact callback bytes/order
and capped results across those 1920 queries; 1183 had more than 128 uncapped
polygons. The final extracted function also preserves visited face/node counts;
cold/warm cache, zero-plane, test-only and fully rejected queries pass. Its aggregate
CPU timings improved by 7.4–15.8% across these geometries, separately from the
curve/model prefilter. Initial cache construction plus query took 0.372–0.996 ms.
Installed aiming FPS remains the final acceptance criterion.

The later installed37598 ship-aim hold confirms that this bounded optimization
did not close the performance outcome. The foreground manual scene showed the
target ship, centered crosshair and 50 FPS; 1185/1717 main-thread samples were in
the aiming camera (69.0%). Collapsed leaf counts were `GEOM::Clip`418,
`GEOM::Trace`261, segment/model boxes134 and `SEA::WaveXZ`107. An earlier sample
of the same ship view was almost entirely background sleep and is rejected.

## Reopened curved-surface coverage

Actual GM replay, using extracted `AimMarch`, `GEOM::Trace`, `GEOM::Clip` and the
current grid/refinement branch, reproduces loss before either polygon cap. Across
24 fans per geometry, removing both caps still missed 470/5838 actual first hits
on Flyingdutchman1,335/6729 on Nevis and54/6936 on PortoBello. Nevis triangle54375,
side+1 is the decisive local case: muzzle(-543.138,17.3739,-575.566),
aim(-636.343,17.3739,-539.334), all289 trajectories hit the model, but14 points lack
eligibility (eight beyond sampled relief, six beyond every flat fan side).
Only1197 output triangles are present, and cap128/uncapped outputs agree there.

Caps cause additional loss and temporal toggles. Translating a broad real-mesh
query by1cm changed capped eligibility at50 hull,two Nevis andfive PortoBello
surface points that remained in both uncapped outputs. Queries below128 polygons
are the nearest unaffected case and have zero such differences.

Rejected repair: replacing the expensive clipped-mesh fallback with its same
witnessed five-plane prism removes enumeration/caps but retains virtually all
pre-cap gaps. Increasing prism thickness also cannot restore points outside its
flat sides. The measured fallback half-thickness can exceed4m; merely raising
the backend limit is therefore insufficient. These probes prove the mechanism,
not the number of visible holes on the player's particular save.

The remaining owner is continuous launch-space support at the actual rendered
depth point plus its first stopping obstruction from the muzzle. Camera depth
and receiver token supply visible geometry but cannot alone rule out an earlier
occluder, including another portion of that same model. A disposable analytic
inverse of the existing projectile warp recovered feasible launch support for
200,000 actual `manual_aim::trajectory` points with no misses, maximum endpoint
error1.007mm and a rejected out-of-support case. Native visibility throughput
and installed-scene acceptance are still pending; no replacement is delivered.

The disposable continuous-curve GPU proof now passes 400,005 trajectories plus
six tangent/coplanar/two-root/occluder cases against an independent binary64
oracle, with zero hit/miss or nearest-time errors (maximum 5.96e-8 seconds).
Plain FP32 was rejected: a triangle-edge sign flipped and skipped a Nevis cliff
face by about110m. Shared CPU/MSL compensated coefficients, corrected roots and
strict projected edge predicates eliminate that counterexample without geometric
dilation. Coincident-face IDs can differ; one extra Flyingdutchman GPU face tie
still needs classification, despite identical first-contact time.

Actual Apple M3 Max GPU times for100k curves are2.593ms Flyingdutchman1,
10.364ms Nevis and5.859ms PortoBello; this is an isolated query workload, not FPS
or an integrated projection. The accepting layer still needs a bounded scene
budget, current model/node transforms and trace flags, water-first stopping,
model removal/mast detachment invalidation and actual visible receiver ownership.
The current renderer remains the sampled implementation described below.

## Surrender marker shading, tracers and demasted automatic fire

The former surrendered marker used `SELECTARG1/TFACTOR`, replacing the authored
neutral atlas RGB with a flat gold fill. The corrected native patch multiplies
the existing texture by gold instead, preserving its rim, highlight, shadows,
alpha and depth test. It restores the technique's previous color operation and
second argument before drawing ordinary ships. An offscreen actual-atlas GPU
oracle checks254 opaque texels within one byte and unchanged transparent
exterior; the actual `ShipInfoImages::Draw` driver passes interleaved surrender,
hidden-ship indexing, ordinary batching and state restoration. Installed replay
is still required.

The initial all-ammunition `Bomb_Smoke.xps` assignment made each shot luminous,
but the player's installed replay rejected its identical smoke/trace appearance.
Canonical `AIBalls.c::CreateBallsEnvironment` now selects three independent
non-bomb profiles and preserves the original bomb effect. They reuse only its
authored luminous Particle2, removing the bomb smoke component. The original
atlas, color, transparency and material remain; no shader or renderer changes.

| Ammunition | Sprite size | Trace width vs bomb | Trace life vs bomb | Emission rate vs bomb luminous stream |
| --- | --- | --- | --- | --- |
| Grapes | .055 | .15 | .23 | .55 |
| Balls | .20 | .40 | .50 | .75 |
| Knippels | .24 | .65 | .72 | 1 |
| Bombs | .30 | 1 | 1 | 1, plus original smoke |

`src/gameplay/RESOURCE/Particles/{Grapes,Balls,Knippels}_Tracer.xps` are native
PSYSv3.5 assets with exact SHA-256 bindings in the canonical gameplay manifest.
They derive from authored Bomb_Smoke hash
`a115d87af80129b77b7941adfe4556e02eb68c3c91d6e04804080f4f68f12cfa`;
only Particle2 Size/Life time/Emission rate values change, and Particle1 is
removed. Binary XPS delivery uses the existing source/receipt transaction,
without text recoding; source hash drift and unknown installed bytes fail closed.
Existing UTF8/CP1251 text contracts remain. A supported prior runtime accepts
the three new assets and updated script atomically; repeat delivery is idle.

Installed `initGoods.c` and `Cannon_RecalculateParameters` retain distinct
launch-speed multipliers: balls1, grapes.6, knippels.9, bombs.8. Actual VM checks
produce100/60/90/80 from the same100-speed cannon, including LongRangeShoot92
for bombs and the no-cannon negative. Four codec rounds restore the new profile
names and sizes from the prior uniform state, preserving atlas indices and
global SpeedMultiply2. No ballistic, damage, charge or timing values change.
The native rigging radii of grapes and knippels are explicit type constants,
so their sprite resizing does not alter sail/mast collision; balls/bombs stay
the same size. Perceived speed and broadside/night visibility remain player
replay requirements until the installed action is observed.

The disposable native loader links current compiled DataSource/FieldList and
their field classes: all three profiles load as one emitter/one Particle2,
and original Bomb_Smoke as one emitter/two particle components. Native graph
evaluation returns initial emission224.765/306.498/408.663 per second. This
accepts binary loading; the existing synthetic-texture particle GPU fixture
accepts only its original shader/alpha cases. Neither proves the new profiles'
actual atlas geometry, night visibility or full ParticleManager/BBProcessor
launch-to-impact rendering. Particle counts are an analytic source budget,
not game FPS measurements.
The existing particle factory, emitter update, impact/clear/destructor cleanup and
flight/save layout remain unchanged. Already airborne legacy balls with no saved particle pointer acquire
no retroactive emitter; new shots use the updated environment. Actual broadside
visibility and frame cost remain replay requirements.

`AICannon.c::Cannon_GetFireHeight` now reads the native persisted `Ship.Masts`
damage record for automatic chainshot. After every known vertical mast is broken
it uses the same target hull band as round shot; native horizontal `mast1` and
its101..199 family do not preserve an imaginary sail-height target. Missing or
empty legacy records keep existing behavior until the native ship initializes
them. Surviving vertical/top masts, repaired masts, forts and other ammunition
retain their targeting. The real VM baseline aimed15.168m above an entirely
demasted target; the corrected source passes demasted/bowsprit-only, surviving
main/top mast, fort, other-ammo, missing state, repair and two save/load rounds.
This selects a valid fallback band; a player volley still owns hit acceptance.

## Preserved gameplay and limits

Projectile flight integration and save layout are unchanged. Manual and automatic
ship fire now share per-gun longitudinal target distribution. It ramps from zero
at effective `TmpSkill.Cannons <= .30` to full skill at `.85`, then multiplies by
`.45` without `RakingFire` or `1` with it. Crew quality contributes
`.1 + .9 * clamp(GetCrewExp("Cannoners") / GetCrewExpRate(), 0, 1)`.
Accuracy controls the independent shot spread, not this longitudinal factor.
Moving-target lead and common automatic broadside jitter remain in the target
offset; each gun retains its traverse/elevation/reach limits.
Reload timing/ammunition mechanics remain intact; invalid manual attempts do not
clear a broadside charge when no gun fires. The published manual broadside-level
random-offset suppression remains unchanged, while live per-ball jitter stays active.

The canonical ammunition script applies catalogue `HeightMultiply*.70` to ordinary
shot and `*.85` to knippels. Preview queries that live helper, including the actual
accuracy/perk/arcade modifiers. Missing query events suppress the unsupported
preview instead of supplying a second physics formula.

The display is a finite sampled approximation. Mesh and sample caps, receiver
boundaries and sub-sample openings can still lose eligibility; the source-level
curved-mesh probe does not prove arbitrary cliff continuity. Blended cloth with depth writes disabled, mixed-owner
flag draws and unregistered detached models remain unsupported for ownership.
Actual storm occlusion, motion stability and colored day/night readability need
native replay. There is no temporal target retention or fake collision plane.

## Focused verification

After applying the ordered source-patch stack:

```sh
cd experiments/native-metal
c++ -std=c++20 -O2 \
  -I .cache/storm/src/libs/math/include \
  -I .cache/storm/src/libs/shared_headers/include \
  cannon_aim_geometry_probe.cpp -o /tmp/cannon-aim-geometry-probe
/tmp/cannon-aim-geometry-probe
```

Also trial-apply the replacement patch on the pinned pre-aim source and validate
all subsequent ordered patches. Probe and syntax checks are not Metal execution.

Native replay must cover day/night water and shore; the contact perimeter and
relation-colored plus; mast inside/outside the invisible rectangle; camera roll;
real volleys through gaps; both broadsides; storm crests and own deck occlusion;
reload/fire; a heavily armed fort and multiple ships. The new path logs
the current aim renderer revision on successful initialization.

Zoom regression: place the plus between a target ship's masts and sweep 1x–5x.
Real mast/hull range support must remain eligible as it grows modestly; the plus
and physical trajectory stay centered, distant objects beyond the aperture are
released, and a nearer center obstruction still wins. The first Tab from the
outside camera now enters first-person manual aim; the second enters third-person
deck walking and the third returns outside. Saved camera mode and telescope/dead
camera gates are preserved by `tools/metal_deck_camera.py`.

## Raking fire along the captured hull

Manual fire at a locked ship distributes guns along the target longitudinal axis instead of one aim point.
`AimRakeFactor` ramps normalized gunner skill 0.30 to 0.85 and multiplies by 1.0 with the RakingFire capstone, 0.45 without;
at zero the legacy single-point volley runs untouched.

Raking geometry follows the ship selected by the existing range aperture, while
hostility attribution keeps the exact center-ray receiver. Hull stations use
the root hull geometry projected directly onto the ship axis; projecting a
world-axis box inflated a rotated hull and included rigging. The station center
moves toward the hull center with skill, so aiming at the bow or stern does not
collapse half the guns onto one endpoint. Elevation/traverse limits and the
fallback toward the original aim point remain; forts and water
retain their existing targeting. A disposable driver of the actual C++ method
checks a rotated 100 m hull, four distinct stern-aim stations, zero skill,
reachability fallback, non-ship targets and unchanged aim height.

Saved games serialize `ChrPerksList`, so startup registration alone does not add a
new ability to an old save. `PROGRAM/interface/perks/perks.c` rebuilds the current
definitions through `InitPerks()` inside the existing `PerkLoad()` callback after
restore. Character-owned learned perks, ability points and cooldowns remain
unchanged. The isolated native script VM rejects the old missing-registry path
and passes a save/restore round with the updated registry and preserved learned
perks/cooldown. The installed old-save ability menu now visibly shows
“Продольный залп” and its two prerequisite descriptors; the observed character
has zero free ship ability points and the new perk is unlearned. Manual volley
replay remains required.

## Firing eye floor

First-person firing view never drops below design eye height: a low ship camera locator or a saved crouch no longer buries manual aim under the rails.

## Continuous contact replacement — CPU/GPU proof candidate, performance rejected

The cannon owner reports an analytic launch-space support with at most two
feasible intervals, then adaptive subtraction of proven complete occlusion
intervals instead of finite candidate guessing. Exact Nevis289 endpoints pass;
13 of14 former prism misses are restored, while the remaining chord point has
a genuine first obstruction2.65m earlier at same-GEOM19345. A narrow real triangle
hole rejects nominal and both edge candidates but yields a free interval after
three adaptive steps. Duplicate61143/61156 incidence is handled by exact depth
bins, retaining outside-bin, same-triangle earlier-root and different-token
negatives. These are reported CPU fixture results, not rendered acceptance.

CPU raster execution costs roughly65–263ms/100k in those workloads. The later
shared CPU/MSL continuous family passes11 named geometry/boundary cases and182
seeded equal-record endpoints, with zero GPU parity mismatches across193
families. Flyingdutchman has one explicitly unsupported zero-time sample. A
synthetic binary64 Porto curve51 endpoint changes by1.46e-11m during transfer
and flips the original-vs-transferred oracle result; both records remain
separate and this serialization disagreement remains unresolved.

The first serialized GPU batch fails the performance target:55/63/64 seeded
families cost12.027/8.981/7.690ms warm GPU time for Flyingdutchman/Nevis/Porto;
the Nevis duplicate61143/61156 query alone costs7.068ms. These are isolated GPU
query timings, not game FPS. Correctness parity does not admit this variant for
installation. A second bounded revision replaces only BVH node broadphase with
outward FP32 bounds, preserving compensated leaf/sweep arithmetic.
Evidence is retained in the cannon owner's `continuous-family.kfAeKRPV` fixture,
particularly gpu-results.txt, final-cpu-results.txt and the separate Porto inputs.

That revision also retains all193 equal-record cases with zero parity errors,
but its warm GPU times are12.444/9.394/7.830ms for55/63/64 seeded families and
7.320ms for the Nevis duplicate. The one-triangle coplanar fixture still costs
1.142ms. Differences may be run noise; no speed improvement or dominant node
arithmetic cause is established. float-node-cpu-results.txt and
float-node-gpu-results.txt retain this changed-input comparison against the
frozen baseline. After two failed performance attempts, GPU probing stops;
the cannon owner diagnoses renderer/resource cost before another algorithm
variant. Both performance variants are rejected and no product delta is kept.

The subsequent native Apple MTLCaptureManager/gpudebug diagnosis captures only
the frozen55-family workload, one command buffer/encoder/dispatch per capture;
both runs retain all55 outputs and semantic work counters. The compiled kernel
has61806 instructions,96 temporary plus40 uniform registers,11552 spilled bytes
and zero static threadgroup bytes. Baseline L1 reads are0.39GiB/s,0.37 stack;
writes are0.48GiB/s, all stack. This proves substantial stack state/traffic,
not stack bandwidth dominance or a cause of the installed player's FPS loss.

Binding maximum threads to32 preserves arithmetic, inputs and parity but changes
none of those compiled fields; no structural resource improvement is accepted.
Profiler dynamic instruction estimates disagree across sessions, so they prove
no speedup. The cannon owner retains gpu-resource-report.md and capture/counter
files, closes profiler sessions13/20, reports empty fresh session/process
inventories and releases the GPU actor. Its read-only source specialist binds
ordinary-segment versus coplanar Polygon/Family data lifetimes and by-value
copies before the next source-only revision.

The retained original-scale GM corpus now binds render and BSP anatomy directly:
all40445/80025/104038 triangle vertex triples match for Flyingdutchman1/Nevis/
PortoBello, with39997/78575/104037 unique BSP triangles and no collidable render
face absent from BSP. The reader normalizes signed zero only; source DrawBuffer
proves RDF_OBJECT.svertex is BaseVertexIndex. This excludes source mesh anatomy
mismatch for those three files only. Live transforms, instances, node lifetime,
alpha/depth, deformation, camera unprojection and water remain separate contracts.
gm-binding-report.md/results and gm-binding.cpp retain the read-only evidence.

The subsequent temporary scalar Segment state revision retains generic coplanar
handling and uses immutable Family/Bin references. cpu-final-results.txt records
193 complete equal-record Result shadows,14164573 ordered per-triangle checks
and8730 ordered ranges without mismatch; strict MSL compilation passes with
fastMath disabled and zero queues/dispatches. The Porto original-versus-transfer
oracle disagreement and unsupported zero-time case remain unresolved. This is
CPU/compiler acceptance only.

The subsequent serial14-workload GPU falsifier uses193 families and56 command
buffers, retaining all complete Result/Pair bits and semantic work counters.
Three-warm medians improve to7.789/6.022/4.851ms for Flying55/Nevis63/Porto64,
and5.195ms for the Nevis duplicate; coplanar1 regresses1.142 to1.494ms. This
measured corpus supports smaller ordinary state, but tiny batches still cost
several milliseconds. Product/full-image performance remains rejected.

One frozen55 native profile retains inputs/results/work counters: spill
reservation drops11552 to11040 bytes while96/40 registers stay unchanged and
compiled instructions grow61806 to134200. Stack bandwidth rates fall from
0.37/0.48 to0.18/0.08GiB/s read/write; rates do not prove total traffic or
bandwidth dominance. family-segment-gpu-report.md retains the result and limits.
The projection chat closes session9, reports fresh session and exact probe/game
inventories empty, and explicitly releases its GPU actor. Root's serialization
hold ends. No production source/cache/app/player state change or staging is
requested; the projection owner continues kernel/resource-boundary diagnosis,
preserving coplanarity, ordered ranges and the uncapped symbolic sweep.

The bounded ordinary/full masked specialization subsequently preserves all193
complete Results/Pair bits, work counters and CPU masks across56 command buffers
and112 ended encoders/dispatches, poisoning both outputs before every repeat.
Primary spills fall11040 to1152 bytes and instructions134200 to58638; full
masked remains11072/133699. Total warm Flying55/Nevis63/Porto64 costs7.6881/
5.9456/4.7401ms, only1.3–2.3% below scalar and potentially noise. Compiler
resource separation is accepted; product performance is rejected. Session9
closes and GPU actor releases; no larger dispatch or product integration follows.
family-specialized-gpu-report.md retains the evidence. The projection source
owner next performs read-only query/arithmetic diagnosis without a GPU actor.

The next temporary candidate caches exact raw UV values on ordinary segment
vertices while preserving original point construction, projection arithmetic,
Pair division order and coplanar Polygon storage. CPU193 Result shadows,
14164573 ordered leaf checks,8730 ranges and13 negatives pass; strict Metal
compile creates no queues or dispatches. Frozen55 UV calls fall3588 to1438 and
Pair divisions18505 to12055; non-UV divisions remain7741. Sequential matching
CPU runs cost0.997816 versus0.826971ms per55 (17.1% lower local CPU time).
This is not GPU or installed FPS evidence. Segment storage grows64 to96 bytes,
so the frozen resource/throughput gate remains required. family.hpp5a0a4c58,
unchanged family.metalfe1d2d53 and family-uv-memo/report.md bind the candidate.
The projection owner retains temporary evidence; GPU work waits for the current
player scene and ammunition replay actor, then explicit sole-GPU assignment.
The Porto binary64 serialization discrepancy remains unresolved. No prototype
is installed.

The single authorized UV-memo GPU resource capture and uncaptured14-workload
corpus subsequently pass all193 complete Results/Pair bits, work counters,
ordered ranges,13 negatives and poisoned output/mask freshness.56 command
buffers/112 ended encoders retain193 primary invocations and one coplanar rerun.
Primary spills1152→1120 bytes and96/44 registers stay unchanged, while compiled
instructions58638→94691 (+61.5%); full masked spills10912. Resource preservation
passes, but smaller CPU work does not imply a smaller shader.

Whole two-pass three-warm medians versus the retained specialization run are
Flying55 7.68808→6.51192ms, Nevis63 5.94563→5.06154ms and Porto64
4.74008→4.20117ms (11.4–15.3% lower). This historical-run comparison does not
control scene or hardware frequency and proves no installed FPS gain.4.2–6.5ms
for55–64 endpoints remains too costly: production/FPS adequacy is rejected.
No further variant or product integration follows this assignment.
family-uv-memo-gpu-report.md retains the exact source/capture/results; the
projection owner retains temporary evidence. Native session12 terminates
normally, fresh session/process inventories are empty and the one-shot corpus
supervisor reports no player interruption. Sole-GPU actor releases to root for
the independent ammunition/fire-mode batch.

Production boundary certification, live camera/render-mesh/BSP matching,
scene-budget GPU cost, live node/water lifecycle and installed FPS remain
unresolved. No contact replacement source batch is staged; the installed
last matched ship-aim profile showed50FPS and roughly69% main samples in aiming;
later independent ammunition deliveries do not establish a new FPS result.
