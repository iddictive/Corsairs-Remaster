# Phonk Mode

## Common material successor

The October16,20:52EDT installed successor routes laser, artillery and ram fields into
one current-material owner. Its checkpoint, stable local cells, bounded tail,
repair and SAVE/map consumers belong to
`naval-ammunition-and-damage.md`, "Common hull-material implementation contract".
The lifetime512-unit history below describes the11fa90d predecessor and old
SAVE import, not the ready common-material path. Gameplay/FPS acceptance remains
with the player; source architecture and compilation do not prove it.

## Real separation and cloth contact

The player's19:05/19:08 replay confirms persistent small laser holes and chips,
including some small-ship damage, but rejects reliable large bow/stern separation.
This supersedes the earlier report of no holes at all. The later19:44 profile
identifies fragment contact solving as the dominant main-thread cost; the new
source successor's CPU/FPS remains unmeasured.
These changes first shipped in `11fa90d`; the player owns gameplay replay.
The later all-ship grounding-force successor belongs to `ship-shore-contact.md`
and is included in the20:52EDT installed batch.

Large multi-node models previously reserved one fragment for every later node,
leaving the root only one slot. A removed bore plus its newly disconnected bow
needs at least two, so the final severing dose was rejected while earlier holes
remained. Work now debits actual produced fragments, respecting the same aggregate
request and eight-group per-GEOM limits. Untouched nodes accept an exhausted
budget; material removal cannot. SAVE reconstruction uses that same allocation.
Root connectivity already releases smaller sections of a newly split component;
pre-existing disconnected authored islands remain independent. No generic
cross-node attachment graph exists: node origins are transforms, not physical
sockets. Complete multi-node assembly separation remains unaccepted.

`ship_laser_cloth_bridge.hpp` connects the existing SAIL owner to the camera pick
and authoritative muzzle ray. The nearest intact cell competes with wood,
island and loose-fragment contacts; existing cloth holes transmit the beam.
Sail incarnation/node/group/cell identity is revalidated across the script event.
`ProcessLaserSailDamage` uses explicit delivered seconds and the shared persisted
`hc/hd/mhc/sp/dmg` settlement, never cannon scratch state. At the current rate,
0.2seconds opens one cell; excess dose cannot charge unrelated cells. Ordinary
cannon tracing/damage is unchanged. Successful cloth contact and exposed wood
feed the existing spatial fire heat/fuel/SAVE owner at2.0 and0.8heat per second,
respectively. Successfully removed root wood now charges partial hull HP above
water as well; mechanical impact HP remains single-owned by TOUCH/script.

Active Phonk now protects only the main player's living ship: hull HP, direct
ShipDead, new structural cuts and ingress are suppressed. Pending incoming cut
work is cancelled before publication. Existing cuts/water are retained; their
fatal reconciliation and flooding resume after mode exit, without time catch-up.
Flood draft is suspended during protection. This changes the previous finite
contact-damage policy; NPCs and mode-off damage retain their existing behavior.
It does not resurrect a dead hull or imply universal avatar/crew immortality.

The source-only shore motor and depth-overlap corrections belong to
`ship-shore-contact.md`; cannon bodies and cosmetic chips belong to
`naval-crew-damage.md`. Player frames and scenario limits are retained in
[the private replay evidence](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-shore-ram-laser-1905/README.md).

## Player stall, collision-detail crash and cut CPU allowance

The d03e571 player sample binds1479/1518 main-thread stacks to SetHullCuts and
1268 to RebuildLaserSurface after laser fire followed by a bomb salvo. Ordinary
cannon changes replayed every retained laser plane synchronously. Reconstruction
now uses immutable authored input and a combined desired cannon/laser snapshot
off the frame thread. At most two active/retired rebuilds exist; cancellation
never joins a running job on the frame or GEOM destruction path. Only a complete
current-generation/revision result uploads and replaces render, material,
canonical collision and support together. Replay does not append SAVE history,
fragments, HP, flooding or crew events. Same failed input/revision is remembered;
the prior geometry survives. Removed pending Blots bindings remain tracked until
their clear applies; Draw also drives already-requested work after owner teardown.
Renderer-reset payloads are frozen per node generation/history count, and live
laser doses wait for reconstruction rather than being discarded or overwriting it.

All live ship, fragment, world and combined-rebuild CPU jobs share
`geometry_work_budget.hpp`: an aggregate allowance of10% of logical CPU capacity,
raised from5% at the player's explicit request after the22:23 replay.
Active worker wall time conservatively bounds CPU time; cooperative checkpoints
repay the shared allowance through cancellable sleeps. The target is averaged,
not an instantaneous OS hard cap: opaque Clipper/Earcut calls and allocation/copy
operations can burst until the next checkpoint. Main-thread capture/upload is
not throttled, and initial synchronous SAVE restore is outside the worker budget.
Geometry quality and physical cutter dimensions are unchanged; less allowance
means a later complete result. Actual CPU utilization and FPS are unmeasured.

The same installed image later crashes in GetCollisionDetails+404 at a BSP byte
read using an uninitialized trace index. Ordinary model tracing now preserves
the exact winning descendant across sibling traversal, clears misses, initializes
its collision node/index and validates detail indices. Material queries keep
their separate detail state. Source repair does not prove why the player's laser
produced no holes. Raw sample/crash and exact signed binary are retained in
[the private player evidence](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-sample-183658/README.md).
The owner forbids agent gameplay tests and will replay personally; installed
delivery and that unresolved outcome remain owned by `runtime.md`.

## Numbered ordnance and world geometry

The October16 successor replaces the lifetime64 world-cut veto and full original
history replay with current retained volumetric material. Terrain uses lazy
solid prisms; stone reuses existing finite-solid Boolean ownership. A fixed
authored BVH selects affected cells, and current outer/cap geometry advances
incrementally. The pending queue stays bounded at8; only64 recent dose records
remain, while a bounded128MiB two-slot checkpoint owns persistent geometry.
Legacy world SAVE1/2 restores before new work; SAVE3 stores retained checkpoints.
Fort, flag, grass and ground-support queries consume the same retained state.
First-source capture and the changed receiver's current-mesh encoding/upload remain real costs;
neither this source change nor the queue bound proves player-visible drilling
or an FPS gain.

Untouched open stone components keep their authored render surface; only BVH-hit
regions activate volumetric shells. Binding/publication caches authored node
matrices for support queries, avoiding full MODEL updates for every blade/root.
Accepted failed fields stay in the existing queue behind revision/topology
fences; unchanged failures do not replay or flood the log. Phase-specific
rejections distinguish the first-hole pipeline. These fixes close identified
source regressions, not proof of the player's previously absent terrain hole.

The October17 player replay still rejects terrain excavation. The source route
previously transformed a terrain dose for every scene node before filtering its
bounds, so an unrelated nonuniform architectural node could veto a valid ground
hit. It also triangulated a full10,032m column section before clipping that
section to the finite bore, then rescanned the whole outer GM for every sample
and cutter face. The correction checks authored world bounds before local-field
conversion. Unchanged receivers reuse their published checkpoint and accepted
cap coverage; the complete identity/save manifest remains authoritative, while
only affected receivers prepare and upload geometry.

Terrain's first section now uses `prismSection` with all finite cutter limits
before planar union. Already-cut cells use the shared finite-limits-first
`solidSectionCaps` kernel. Internal cell faces stay eligible; the current visible
outer/cavity masks remove genuinely existing coplanar walls. A cached outer BVH
selects source faces, their clipped descendants stay beneath those same bounds
through one admitted batch, and assembly/indexing happens after the batch.
Only intersecting cavity patches are clipped or considered as masks. CMW2 and
legacy SAVE imports retain their wire contracts; the indexes and bounds are
derived. This adapts the existing convex local-carving owner described in
[Cracking destruction](https://blog.voxagon.se/2014/05/13/cracking-destruction.html),
without replacing volumetric land with a hollow shell. Source checks and the
canonical build cannot accept this change: a player must observe progressive
ground removal at the actual contact, matching collision and preserved adjacent
terrain after save/reload.

While Phonk is active, native `SHIP::ExecutePhonkOrdnance` reads the mapped
`Phonk_Laser`/`Phonk_Nuclear` controls and publishes the selection in
`Sea.Phonk.Weapon` (6/7). Script initialization and mode entry register these
controls for the three sea groups. Ordinary load restores settings first, then
reconciles these names; battle-interface initialization repeats the idempotent
reconciliation, preserving user remaps. The existing Ammo/Powder rows show the
selected6/7 weapon and native `LaserCharge`/`NuclearReload`; their -6/-7 icon
sentinels select laser/nuclear cells41/42 in `LIST_ICON2` while suppressing powder;
ordinary negative icons, sails and wind retain their behavior. The generated
RGBA source `src/assets/ui/phonk-weapons.png` and prepared atlas are hash-bound
by the UI manifest. Entry defaults to6; active testing laser charge remains full
and no longer ends the stroke after6.25s. Nuclear targeting uses actual water, ship and world
receiver traces within6000m of the firing ship, and the draped footprint uses
that same unfiltered target and220m radius. Missiles travel on a bounded arc,
hit the first intervening receiver and reload in4 game-seconds. Paused frames
advance no launch, flight or damage; launched missiles continue after mode exit.

Laser camera mode follows the active native `COMMON_CAMERA`, not the derived
`Crosshair.OutsideCamera` flag, which can also be true for a first-person telescope
or restored camera-controller state. First-person laser fire keeps the exact
crosshair pick, including local ship details and ground; third-person laser fire
uses living hostile ships and active fort guns ordered by distance without a
camera cone. Existing relation/surrender policy and intact firing-bort muzzles
remain authoritative. Automatic laser fire requires a real muzzle ray reaching
that receiver. If the bounding-box point misses, a fixed nearest set of actual
retained root-hull triangle points supplies fallback picks; full occlusion and
receiver checks still apply. Nuclear targeting retains its existing camera flag.

Finite dose dimensions are admitted in physical metres by the shared
`ship_hull_damage_bridge.hpp` helper, including SAVE decode. After conversion to
a receiver's local coordinates the kernel requires finite positive dimensions;
metre limits cannot be reapplied to scaled local values. Sample and geometry
budgets remain unchanged. This distinction prevents a legal2m crush pocket from
being rejected after a rotating yard produces a local radius of2.00000024.
Frozen native baseline inputs, commands and raw observations are retained in
[the private regression evidence](../.local-archives/metal-renderer-evidence/phonk-live-regression-20261016/README.md).
Current coherent native CORE replay and its exact fixture inputs/library hashes
are retained in
[the successor evidence](../.local-archives/metal-renderer-evidence/phonk-current-20261016/README.md).

Automatic nuclear fire leads through `AIShip::GetPrediction`, checks its arc and
rejects an impact inside the firing ship's predicted blast envelope. Preview
selection refreshes at10Hz and is solved again at the trigger edge. Ordinary
cannon inventory/reload and Phonk trigger/pause/exit-flight semantics stay intact.
Source-body sanitizer and native syntax checks pass; actual moving ship/fort,
camera-switch, obstruction and self-blast replay remain pending. Private evidence
owner: `phonk-auto-scene-20261016`.

`ISLAND` binds persistent `Islands[].PhonkWorldCuts` to transient static models.
Legacy halfspaces, finite beam doses and deterministic26-plane crater volumes rebuild one retained mesh
for drawing, tracing and clipping. Active PN terrain is the input when present;
material/UV data and unaffected components remain intact. A world-only material
classifier covers shipped ground families independently from PN subdivision
admission. Terrain is a union of downward triangle columns; open architecture
uses bounded inward prisms and closed components retain their authored volume.
Local cutter-face sections are normalized by pinned Clipper2 2.0.1 and the
existing triangulator; global projected basement contours are not a valid
solidification for the self-overlapping shores. Fully removed receivers retain
an authoritative empty draw/collision owner. Validated finite cuts commit only
after all receivers accept; rejection
restores the prior projection. Counts and geometry budgets are bounded; a
rejected cut must leave the previous solid intact. Live world cuts now capture
immutable authored geometry and prepare their retained mesh, scene cap coverage
and collision BVH in one value-only worker. Ordered requests retain delivered
doses; across requests only successfully committed predecessors advance a repeated bore. The
main thread validates every receiver's identity/transform, stages all GPU buffers
and publishes geometry, collision and SAVE together. Paused frames cannot publish.
Load/rebind cancels stale work and reconstructs the existing saved schema through
the same pure geometry path. Fort cannon reconciliation
uses the existing cannon damage and assault owners. Nuclear ship impacts reuse
hull cuts, flooding and script-owned death/HP, avoiding a second partial-HP cost.
The private geometry dependency is unmodified upstream Clipper2 at commit
`21ebba05db8894f0c7217ad35ea518080f324946`, licensed BSL-1.0; its engine/triangulator,
transitive headers and license live under `geometry/third_party/clipper2`.

New cavity faces have separate draw batches. Their material copies reuse base
texture0, source-derived UV density and vertex multiplier; stage1 island masks
never tile across a cavity wall. Outer surfaces keep their complete authored
material and UV channels. Collision retains the exact union of outer/cap meshes;
both uploads must succeed before either projection replaces the previous state.
This fixes a native frame whose old `.12` UVs, tiled mask and full-white vertices
produced a dense pale grid despite correct physical cutting.

Cap coverage is also a scene responsibility: overlapping shore/seabed receivers
must not draw the same cutter face twice. Immutable raw patches are keyed by
persistent cut index and canonical face. ISLAND resolves their union in authored
world coordinates, preferring shore then uncovered seabed/fort areas, only during
cut/rebind. Reflection models have an independent coverage group because they
render in a separate pass. Derived sphere axes preserve canonical face identity
through rotated/uniformly scaled nodes; neither axes nor coverage add SAVE fields.
Coverage replacement and collision rebuild must both succeed before commit.
Difference polygons use the existing earcut path first. Its invalid skinny-strip
diagonal on Antigua requires the pinned integer triangulator as a bounded
fallback, with Delaunay flips disabled. Its boundary check honors the kernel's
source-defined one-unit repair on the declared million-units-per-metre grid;
ordinary cap validation stays exact. Area, holes and secondary-only coverage
remain mandatory. Sixteen complete/PN roles and transformed coverage pass
ASan/UBSan; the scene frame and installed delivery have separate acceptance.

The first delivery is rejected by the player's6/7 and unchanged-island replay.
Actual CORE/MODELR/ISLAND reproduction confirms Antigua's seabed and fort reject
one220m blast and roll back the complete transaction. The original component
tests missed whole-model/child-node topology and cannot accept that delivery.
The corrected local-field path passes16 complete Antigua/Cumana role cases,
including actual PN input, shell apertures, unchanged triangles and strict cap
coverage/area checks under ASan/UBSan. Native VM verifies the new load/bootstrap
and HUD against old/remapped controls and ordinary/OFF cases. The actual Metal
explosion helper separately passes shader/pipeline and foreground-depth checks.
Integrated correction delivery and accepting player replay remain governed by
`runtime.md`; component success alone does not close those gates.

## Finite laser locality and nuclear cloud

The live laser records actual muzzle-contact samples, never an automatically
oriented cut plane. Each sample subtracts an octagonal bore0.32m across flats;
depth is4m per delivered second. Receiver-local coordinates follow a moving
ship. A0.20s batch retains the union of its samples; disjoint aim positions and
receivers are not bridged by a chord or bounding box. Membership and body-envelope
queries share the geometry field without per-query allocation. The installed
predecessor's ship history has
512 cost units: a finite record costs its actual sample count, and a legacy plane
costs16. Thus legacy splits remain capped at32 while single-sample doses can use
up to512 ordered records. Each record still holds at most16 samples; the separate
pending-job limit remains32 and active fragment buffers remain256. That predecessor
also capped world history at64; the current retained world owner is described
above. More small ship operations can increase cumulative
preparation/replay cost; this change is not a measured FPS improvement.

Completed worker snapshots validate unchanged node identity/generation/revision,
but an animated yard outside the cut no longer invalidates a changed hull.
Changed nodes still require their captured local pose. Camera/contact updates
precede dose backpressure, so a full queue cannot freeze manual aim. On a receiver
switch, an unadmitted old dose is retried once then retired; it cannot block every
later receiver. Accepted jobs retain their existing ordered publication contract.

Repeated contact follows the center ray through the actual octagonal bore,
including small aim/pose drift. It advances to the existing far cap only when
the complete ray segment stays inside that bore; nearby independent strokes,
side exits and reverse rays get no free depth. Earlier doses in one atomic batch
are included before publication, and are rechained when a committed predecessor
moves that batch. Radius and delivered material depth remain unchanged.

Ship records without `kind` retain their original plane semantics. Finite records
carry the shared sample codec; world state accepts version1 and upgrades to2 on
the next committed cut. Ordered geometry reconstruction restores the apertures,
collision and support state without additional save-owned flood fields. Ordinary
ship rams use finite local crush pockets at every admitted energy; nuclear ship
cuts retain their plane semantics. World nuclear craters retain their bounded
volume. A finite bore preserves the coarse outer contact
waterplane; exact retained triangles and the finite field own its cavity. New
flood aperture comes only from removed authored skin: synthetic caps, deeper
drilling and overlapping doses cannot count the same entrance again, and finite
openings never grow with seam age. Legacy separating-seam ingress is unchanged.
Crew cut reactions use the same saved finite volume. A partial mast notch retains
support; loss of an actual spanning section invokes the existing whole-mast
`MastFall`, including its rigging ownership. Partial-stump simulation is not added.

Repeated ship bores subtract the union of recorded cutters from immutable
authored material; they do not repeatedly partition unaffected triangles.
Wood uses the inner vertices of its classified physical shell, closed authored
components retain their own solids and unclassified open sheets stay open.
Laser camera and muzzle rays use the opt-in `TraceLaserMaterial` hierarchy query:
its retained outer/inner timber and finite-wall index is prepared with the mesh
and ray scratch, then published atomically. Ordinary cannon topology updates
reuse their clipped CPU triangles to refresh this projection; unchanged cut
signatures do no topology work. Ordinary `Trace`, `Clip`, hit details and canonical
structural accounting keep their existing surfaces. Inner timber removal still
publishes real geometry without adding interior area to canonical damage;
later finite-wall removal follows the existing canonical formula. Flooding
apertures still come only from removed authored outer faces.
The installed stock inventory covers all57 classified root GMs; each has zero
validated canonical closed volume. The profile owner excludes mast, yard and
shatter files. A synthetic closed solid with an added inner shell still rejects
an interior-only dose; this extension remains unsupported and closed-volume
conservation guards stay intact. The exact inventory, commands and source are in
the private [material-query receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/integrated-sea-laser/held-fire-diagnostic/implementation/receipt.json).
An enclosed disconnected piece leaves the retained hull as its own fragment;
ordered SAVE reconstruction repeats that extraction. A hollow hold and gunport
are not filled to create a cutting surface. Solid/hollow ring checks and actual
Flyingdutchman shallow/progressive cuts pass CPU checks, but the complete ship
transaction and accepting native scene remain pending. Its measured20–90ms CPU
preparation reached100ms during integration and must run outside the interactive
frame before delivery.

Live ship cuts use immutable GEOM inputs and a value-only worker for the complete
mesh, fragment properties and collision BVH. SHIP validates model/node identity,
local transform and geometry revision before staging every node and committing
the batch. World movement does not invalidate receiver-local work. Rejected work
leaves prior geometry, collision and saved records intact; a valid later dose can
still commit. Load/Mount cancel stale work, and pause cannot publish it.
Mechanical work is reserved once and refunded only to the same unexpired contact
on rejection; its impulse belongs only to the resulting newborn pieces. Existing
nuclear HP admission remains once per blast.

Support compares each locator's original authored-face witness with retained
material after released chunks leave. Crew above a deck and locators inside a
real hold do not fail a point-in-solid test. The same SHIP query drives crew,
rigging and cannon availability; gun checks cache the geometry generation and
cut revision. The transaction passes source-body queue/rollback checks; real
Metal upload cost, ordinary saved-game reconstruction and player actions remain
separate acceptance gates.

CPU sanitizer evidence covers actual Antigua/Cumana/Flyingdutchman meshes,
old-plus-finite cuts, support notches/severing, exact aperture unions, codec
rejection and rotated queries. It is not native input or player SAVE acceptance.
The finite candidate still needs the integrated CORE firing/rebind/rollback and
installed-player replay recorded in `runtime.md`.

The nuclear renderer reuses its depth-stopped Metal volume and9s lifetime. One
immutable256KiB noise lattice supplies coarse/mid/fine density; toroidal advection
rolls the cap while the hot core cools into a lobed smoke plume. Two optical-depth
taps provide internal light attenuation. The wide ground ring is analytic, so
the64-sample cloud march avoids empty ring-space work. This adapts the maintained
[three.js volume-fire optical model](https://github.com/mrdoob/three.js/blob/master/examples/webgpu_volume_fire.html) without its fluid-grid simulation lifecycle.
Actual helper evidence contains217 consecutive1920x1200 frames plus exact zero,
foreground-occlusion and expiry checks. Late smoke remains soft at its300px-class
fixture footprint. The fixture backdrop proves neither terrain/rigging integration
nor continuously viewed motion. Private evidence owners are `bounded-laser-20261016`
and `laser-locality-nuclear-detail-20261016`; delivery remains separate.

## October16 — airborne coast contact and beached support

Airborne grounding previously replaced the ship's actual Y with wave height;
TOUCH independently swept its coastline contour atY0. Both enforced a sea-level
barrier even above low land. Airborne ships now skip only that wet-depth branch:
the retained hull contour sweeps actual island geometry at three heights.
Ordinary wet grounding and its complete TOUCH depth branch are unchanged.

The existing island trace and five retained keel points supply terrain support.
A descending hull stops at the solid terrain plus its draft, keeps its supported
pose after Phonk exit, and reconstructs support from the saved elevated pose.
No SAVE fields are added. Removing terrain or raising water above support clears
the transient state; death clears it too. Beached hulls report no water contact.
The existing wet depth raster is only a broad phase for terrain queries.

Actual Move/TouchMove/LandSupport/PhonkMotionAllowed/CheckShip2Strand methods pass
bounded ASan/UBSan adapter checks for low-coast traversal, higher-rock contact,
landing, OFF/reconstructed pose, prediction restoration and support invalidation.
Probe sources and limits belong to private `shore-airborne-contact` evidence.
These are sampled hull checks: thin obstacles between rays, initial overlap and
isolated bumps between keel points can be missed. Native delivery belongs to
`runtime.md`; actual tsunami/Shift/coastline, tall-rock, ordinary wet grounding
and saved beached-pose replay remain player-owned and unresolved.

## Beam visibility and fracture hitch follow-up

The scene helper now owns one unlayered `SHIP_HULL_CHIP_ASSETS` entity, keeping
the existing three immutable chip models warm between bursts. One loader in
`locator/blast.cpp` owns the service-bound weak cache; the helper and active
bursts hold strong references. The anchor consumes zero burst leases, creates
no particles and releases with the helper. Active bursts retain their meshes
after anchor removal; the last owner releases them. Failed preload preserves
the existing burst retry path. Ordinary BLAST ownership and the eight-burst cap
remain unchanged.

The causal same-binary RAM151 comparison warms only those three models, with
geometry prewarm disabled: first-action time falls57.573 to28.795ms and all151
state rows match. Startup asset loading costs16.105ms. Prewarming hull geometry
separately removes its cold preparation but does not address this asset-load
gap; eager fleet geometry preparation is not integrated. The exact loader,
anchor and BLAST ownership bodies pass the reused ASan/UBSan oracle, including
partial failure, service switch, eight bursts and final release. Native installed
delivery is recorded separately in `runtime.md`. Fresh normal RAM151/laser151
runs against all50 staged canonical libraries retain every151-row state value,
including first cuts101/134 and retained supports17 to10/9. RAM action falls
45.499 to30.194ms; laser cut-frame CORE falls43.936 to34.066ms. The normal Mount
owns preload: there is no diagnostic warmup entity or geometry adapter. Whole
frames retain the visible hull-stopping beam and fragment assemblies; battery
drain/recharge remain. This is not120FPS or removal of the synchronous split,
and does not accept ordinary TOUCH, prior SAVE, sinking or gameplay death.

The invisible-over-water beam is a render-order defect. Actual native submission
passes SetFVF, the PhonkLaser technique and the compact Metal draw, with valid
diffuse vertices, depth testing and no depth writes. An exposed muzzle and an
air-only ray reject the submerged-gun hypothesis. SHIP draws at priority31;
Sea follows at65530 and composites over the transparent beam. With the identical
diagnostic binary, changing only SHIP's priority to65532 makes the white/violet
beam clearly visible over water. Retained input plans and full frames live in
`.local-archives/metal-renderer-evidence/destruction-scene/`; `dispatch-trace-87b4acb`
binds the dispatch probe and `late-order-trace-87b4acb` the causal ordering probe.

The source candidate reuses SHIP_CUT_FRAGMENTS for a65532 Realize callback in
the resolved ship layer. Mount attaches it before the first shot and also covers
Load. Only transparent beam, contact and hot-edge vertices move late; opaque
fragment geometry remains in SHIP::Realize. Actual layer membership excludes
removed ships, and reflection layers never invoke the helper. Contact-light
publication stays early in PublishPhonkLights; moving it late would miss the
opaque receivers before the next frame clears the light catalog. Both consumers
resolve a moving target through PhonkLaserEndpoint. The contact light retains
the original nonzero-delta/reflection exclusion. Canonical stage-only exits0
and installs signed6a6be8eb, preserving all526 protected player files. Fresh
current-header native RAM401/laser401/water151 runs193610/193617/193622 exit0,
bound to50 canonical libraries and the frozen six-source map. Whole-frame
inspection accepts the beam over water, its stop at the target hull and the
short air segment/surface contact under downward aim. Cut, retained-geometry
and charge/recharge invariants remain. Player/save/death/quest acceptance and
the independent renderer-cache crash remain open.

Inclusive native stage timing locates the first fracture's synchronous cost:
ApplyHullImpact/ApplyPhonkLaserCut take60.3/59.4ms; chip creation takes23.1/25.4ms
and cold closedVolume takes10.7/10.5ms. Nested stages overlap and are not additive
frame totals. The hitch candidate shares the three immutable chip meshes through
weak service-bound cache entries and per-burst strong leases, preserving each
burst's2/4/2 mesh assignment and ordinary BLAST ownership. It reuses the existing
quantized-edge hash for unordered closed-volume counts, preserving the closure
predicate and volume accumulation. ASan/UBSan ownership, failed-init, service
change, last-release/recreate and actual hull-volume probes pass. The integrated
native action still hitches: RAM108.295ms includes42.668ms action plus65.567ms
CORE; laser43.9756ms includes its43.936ms CORE action. No image readback occurs
in those frames. The cold RAM CORE excess is not yet causally attributed;
these measurements reject full hitch acceptance and are not an FPS claim.

## October15 — timber/contact/flight source successor

The16:04 ghost report has a source-bound native draw reproduction: rough-cut
refinement/side partitioning dropped explicit-empty object entries. `GEOM::Draw`
then restored their original GM draw while collision retained zero triangles.
On actual Flyingdutchman first rough and flat→rough cuts,18 removed objects
reappeared. A four-line successor preserves the object domain in refinement
and both split halves: exact native draw invocations fall18→0. ASan/UBSan
rebuild, saved-plane replay and failed-upload ownership checks pass. This is a
source repair installed by canonical87b4acb staging as signed61af9725; exact
delivery evidence belongs to `runtime.md`.

Coherent engine-linked native runs20261015-165616 (RAM),165708 (laser) and
165809 (water aim) complete401/401/151 frames without a cache crash. The private
fixture first required an enrollment-order correction: SEAFOAM must start after
both native ship models mount, rather than dereferencing their still-empty models
in Main. No production source or installed app changes implement that correction.
Binary497292c3 binds87b4acb headers and50 unchanged linked inputs; all runs use
two Flyingdutchman1 ships and1920×1200 captures.

RAM removes34.49% atframe101, with retained probes17→10. Laser cuts at680ms,
retained probes17→9 and structural failure published; charge falls1→0.8624 and
recovers to1 by3s. The detached left hull/mast is absent from the retained ship
in the6s captures, without the earlier original-half resurrection. The oblique,
partly submerged cut faces do not accept section appearance or continuous
fragment contact. The fixture directly calls ApplyHullImpact and acknowledges
no death events: its unchanged6999HP/alive state proves neither gameplay death
nor its failure. Ordinary TOUCH, prior-save, sinking and quest replay remain open.

Hitch acceptance is rejected by the full native action: RAM frame101 costs
132.417ms versus8–9ms neighbors; laser frame134 costs54.825ms versus7–9ms.
Neither action frame performs image readback. Allocations rise8→128/110 and
uploads roughly6.5→12.5/12.7MB; GPU waits are7.064/0ms. These include native
preparation, uploads, collision, child nodes, chips and the remaining frame,
which the earlier warmed header timing excludes. The causal timing and pending
optimization are described above. At200ms these baseline laser/water frames
show no readily distinguishable bright beam or water-contact ring; visible
heat/faint violet marks do not accept the requested beam visibility. These
negative results remain open independently of the renderer-cache crash.

The15:20:32 player replay rejects installed7433ec35: layered sections, breakup
hitches, dragged pieces, water-aim beam visibility and nonphysical wrecks.
Canonical5e9af7f reached the installed app (exact receipt in `runtime.md`), but the
player's16:02/16:04 replay still rejects pass-through and detached stern ghost
geometry. Its87b4acb successor corrects the reproduced draw/geometry divergence
above; complete player acceptance remains open, including the native rejects.

Timber interiors precede
fracture in one cached tagged surface: outer triangles are clipped once while
canonical collision caps and render-only timber caps stay separate. Pinned
unmodified [Mapbox earcut.hpp v3.2.3](https://github.com/mapbox/earcut.hpp/releases/tag/v3.2.3)
triangulates nested structural-wood contours with holes. Hollow-box/gunport
centres remain empty; ambiguous open contours receive no invented disk.
Mechanical pieces retain their complete spatial assembly, with at most one
additional split of an oversized assembly. Grain refinement visits the fracture
slab and shared neighboring edges. Actual Flyingdutchman first rough cut CPU
split/group/assembly falls38.430→8.187ms; render triangles72,247→52,979. Initial
timber preparation costs4.862ms once. GPU upload/BVH/fleet cost are excluded.
Canonical half/upper/severe-horizontal fractions and saved-plane reconstruction
remain unchanged; failed uploads preserve both caches and old fragment ownership.

Fragment tangential impulse is bounded by0.18 times normal impulse, with angular
effective mass. A0.001m/s grazing approach beside a1000-knot receiver transfers
0.0002016m/s rather than92.6m/s; separating contact transfers none. Laser and
ordinary aim share bounded wave-contact refinement. The visible air ray reaches
the wave with a surface contact ring; its physical ray still cuts submerged
hulls. An immersed middle gun prefers the nearest intact exposed gun on the
same bort. Depth testing and solid hull/land occlusion remain enabled.

Flight removes the once-per-tsunami-event launch latch; existing airborne,
settling and contact gates admit a later crest departure after landing. Complete
finite frame time is consumed with at most64 integration substeps, avoiding the
reproduced200ms false support acceleration. Actual-header replay at
16/50/100/200ms gives two departures/two landings on two same-event approaches,
one per single crossing and none for flat/falling/parallel support. World
trajectory governs departure, allowing sideways drift; Shift provides thrust,
not an unconditional jump. Native pixels, trajectory, complete frame cost and
prior-save acceptance remain unresolved.

## Historical hollow-hull and loose-fragment successor

The player rejects the staged50daab4 destruction: open paper-like hull sections,
pieces passing through ships, and severe material loss leaving a live remnant.
Successor `88b3ebe` passes canonical build and installed-app delivery in frozen
`4ee15fd`; `runtime.md` owns the exact delivery receipt. Native scene motion,
prior-save restore and real death/quest handling remain unverified, so player
destruction acceptance remains open.

Actual installed Flyingdutchman geometry reproduces a missing fatal case:
`y<=0.01` removes84.48% of authored root surface while retaining the original
waterplane and all five keel samples. Cumulative retained surface now also marks
critical failure at35% or less. The ordinary `y<=8` upper trim removes18.60%
and remains nonfatal; half-length, half-width and bottom-support cases keep their
prior fatal result. Reconstructing the same sequential planes reproduces the
remaining fraction. These are surface/predicate checks, not measured wood mass
or acceptance of native SAVE/death/quest handling.

The existing split/cap and native model collision facilities remain the owners.
Disconnected authored surfaces are grouped into bounded bodies without dropping
triangles/materials/UVs; bodies use measured surface centroids and actual vertex
supports. Mechanical display pieces may split again unequally without changing
retained support or saved cuts. Whole-node pivots were rejected: the actual
detached half's centre differs by roughly12m from the original model centre.
`ship_fragment_contact.hpp` supplies a bounded convex proxy and continuous
triangle separation queries. Translation is exact for that proxy; rotation uses
conservative angular bounds and can stop early. It is not full mesh-mesh physics.
The ordinary ram path shares these owners outside Phonk.

`ship_wood_surface_profiles.hpp` explicitly classifies112 root-model/base-texture
pairs across57 of58 installed root models. This is partial surface-role coverage;
Lugger_w1 and ambiguous mixed atlases remain unclassified. Render uploads alone
add inward wood faces and boundary thickness; original outer UVs, ports, hollow
holds and unclassified cloth/alpha/metal surfaces remain. The canonical mesh,
collision, damage fraction and saved cuts do not acquire a fabricated solid
volume. Actual Flyingdutchman z=0 retained/detached render triangles increase
25104→31553 and15941→19822. A new Blast/PhysX asset/world integration was rejected
for this candidate because it needs authored chunks/bonds and a new runtime
adapter; local split, collision BVH and existing wooden-chip entities are reused.

Actual native translation-unit syntax and bounded ASan/UBSan checks pass material
preservation, port apertures, upload rollback, body ownership, broad-face contact,
moving receivers in both ship execution orders, pure spin, separating seams,
birth/pause/layer lifecycle and all32 eight-body cuts without retirement. A CPU
smoke pass visits316 BVH candidates for eight bodies against25104 retained
triangles in1112us, including proxy construction; it excludes native Clip/GPU
cost and establishes no fleet FPS result. Native pixels, complete motion,
gameplay death and prior-save replay remain unresolved.

## October15 — integrated fracture lifecycle

Source `50daab4` passes canonical native build/staging with signed engine
`d660336fae0108c01f6ba5e2a6ebb523822e0686030ec65c9a995f44b8ac7d18`.
Fresh protected player hashes remain unchanged; player acceptance is unresolved.
The integration owner records complete delivery evidence in `runtime.md`.
The engine source owner is now `experiments/native-metal/engine`; historical
patch references below describe prior receipts, not the current edit path.
The player rejected the preceding4c14ef fracture behavior: half-hulls remain alive,
crew walk over removed decks and loose rigging remains above absent structure.
The successor binds geometry, retained support and structural death for both
laser and mechanical cuts; source evidence below is not player acceptance.

The next source-verified contact correction applies the existing accepted HP
budget to new physical contact work before geometry cuts. The collision event
returns that fraction; TOUCH uses the inverse damage curve, so even a saturated
1000-knot impact cannot bypass protection merely through its large raw energy.
Zero accepted fraction adds no mechanical cut. Already critical structure and
laser cuts retain their ordinary death reconciliation. This successor also
requires the heavier hull to supply inward acceleration for sustained crushing;
passive resistance to a small pusher no longer admits that work.

Mechanical breakup now adds optional grain/roughness/spacing/phase to the same
saved local cut owner; old cuts default to exact planes and lasers stay planar.
One diagonal piecewise-linear signed field drives visible geometry, support,
waterplane/keel, openings and geometry rebind. The existing mesh split/cap runs
through a determinant-one shear with bounded knot/triangle/seam budgets; an
explicit incomplete result preserves the previous geometry. Multiple retained
waterplane components reject before mutation. No second fracture system or
dependency is introduced. Actual closed-box volume and installed Flyingdutchman
mesh cases pass ASan/UBSan; the latter has40445 input triangles/20 objects,
35838 kept/22749 detached triangles,228 seam edges and field deviation below
0.0003m. A repeated overlapping micro-sliver cut rejects while preserving the
prior watertight mesh. Shared native support/contour/keel/mast probes retain a
17-point waterplane, nonfatal upper trim and fatal keel/half loss. These checks
do not accept a rendered fracture or prior-save replay. The coherent native+
AIShip successor is canonically staged in50daab4; `runtime.md` owns the fresh
518-file protection and signature/content receipts. The separate indexed-span
renderer corruption remains unresolved and is not fixed by this gameplay batch.

The current request keeps ordinary Phonk at500 knots and adds held left/right
Shift, using the existing `ILeftShift`/`IRightShift` controls, to accelerate
continuously toward1000 without a timer or consumable. Brake wins; release sheds
excess speed continuously. Supported thrust raises the bow through the existing
damped attitude response; airborne motion and water-entry compression keep their
existing owners. Ordinary/NPC movement and the binary save layout remain.

`PROGRAM/scripts/sea_phonk.c` owns2% maximum hull regeneration per game second
and the main-player protection policy described above. Pause, reload, death and
mode disposal retain their existing guards. The former5%/25% contact-episode
budget and active-player structural death have been superseded in source.

The existing mapped `Ship_Fire` input owns the Phonk laser; native camera fire
does not spend cannon ammunition or alter its reload during the mode. The aimed
existing broadside supplies one intact cannon near its middle, emitting a
single beam with muzzle-to-target occlusion. One battery drains16% per game second during
fire and restores10% per second on release; zero charge stops the beam. Existing
navigator charge arcs show it only in Phonk, retaining the ordinary charge and
damage paths outside the mode. Holding
fire visibly advances the beam and burns one target for0.65game seconds before
the final material bridge parts; the next cut has0.20seconds recovery. Moving
the aim over the same target supplies the cut plane, allowing vertical bisection
or a horizontal upper-hull trim. A stationary aim defaults to a vertical plane.
Camera picking and the selected muzzle's ray respect intervening ships/islands. The
shared bound includes transformed geometry centres and every model child.

`ship_phonk_laser.cpp/.hpp` own input, transient beam/heat, separated-piece motion,
native wave sampling and persistent plane/flood attributes. Geometry owns actual
half-space splits, original materials/UVs, closed-contour caps, persistent0.08m
char bands and retained-mesh collision. It reuses the hull BVH; immutable bodies
have child draw/release tokens and a compatible aggregate token. Shared limits
allow32 cuts and up to eight groups per node/cut, hence256 body buffers per GEOM;
previous debris cannot exhaust admission before all supported cuts. SHIP shares
at least eight new groups across eligible nodes, debiting actual outputs only.
Fragments inherit point velocity, gravity, wave support and gradual waterlogging,
collide through native MODEL::Clip, and retire only after full submergence.
The existing globally bounded wooden-chip entity accompanies actual separation.
Actual intersection edges glow orange and cool over5seconds.

One `SHIP_CUT_FRAGMENTS` entity executes last in the active sea Execute layer,
after all ships publish their poses. A transient per-ship clock and body birth
clock prevent order-dependent receiver snapshots and double advancement of new
pieces. Full elapsed time is subdivided; pause and render passes add none.
Layer rebind and empty-fleet deletion own the helper lifecycle. Generation-bound
tokens are never drawn or released against replacement GEOM instances.

Damage follows the actually removed geometry. Closed meshes use validated
volume; the authored open Flyingdutchman mesh falls back to surface area, which
must not be described as measured mass. Successful laser removal of root wood
charges partial hull HP for dry and wet cuts; decorative child-only cuts do not.
Small wet cuts feed progressive flooding. Retaining at most35% of the original
root material measure, losing35% of the original waterplane, or35% of the draft
at three of five original keel stations, marks critical
structural failure through the existing death/quest-guard path. Dry top trims
retain waterplane/keel support and are not fatal. Up to16 actual seam points in
height bands per cut query current `WaveXZ` at4Hz; lengths are grouped within
those bands, conservatively sampling their lowest actual opening. Mixing upper
and lower seams into one averaged point was rejected: it could move an entire
wet opening above water. Submerged openings admit water with a
simplified head/area flow; capacity derives from the original waterline contour
and draft. Flooding continues with Phonk OFF and restores after load. A separate
global sea event applies its progressive HP ceiling, with ordinary immortality
and quest guards. This is a simplified ingress model, not a fluid simulation.

`Ship.PhonkLaserCuts` stores model-bound local planes; `Ship.PhonkFlood` stores
ingress and attribution. Normal startup/load reconstructs cuts, chars, breach
samples and the contact contour without replaying damage or spawning old pieces.
TOUCH invalidates its existing cached contours through one default-zero hull
revision; ordinary ships retain their prior path. Old saves without these
attributes enter unchanged. Native pause advances neither burn nor ingress.
Renderer resets rebind saved planes to each new geometry generation without
duplicating damage, ingress or fragments. Source-node techniques and lighting
are scoped around independent fragment draws.

Every successful cut publishes `SeaPhonk_HullCut` (`alfll`) after its geometry,
owner attributes and attack consequences. The sea-lifetime handler owns both
laser partial damage and structural failure; mechanical partial cuts never pay
HP twice. A structural hit uses the existing death/quest path independently of
Phonk and combat multipliers. Restored critical cuts request fatal-only
reconciliation on usable frames until acknowledged; pause/reload cannot lose
the request, and partial HP damage is not replayed.

`SHIP_BASE::IsHullPositionRetained` owns model-local support. The common-material
successor queries current geometry against the immutable authored support
domain; the installed predecessor uses saved cut half-spaces. Intact ships keep
their existing support.
Sailor graph points, routes, work targets and offsets consume it; removed support
enters the existing fall/swim lifecycle. Mast/yard subtrees stay intact under
their native attachment owner rather than being cut beneath unchanged cloth.
The lowest unsupported/intersected authored mast falls through `MastFall` even
on restoration. Pending mounting precedes either Execute or Realize exactly
once, and deleted mast references are cleared. Individual hull-root rope/shroud
anchors also consume support, cached by revision and model/root generation.

Rope storage destruction synchronously sends the existing model/rope-scoped
`MSG_SAIL_ROPE_UNTIE` before freeing endpoint positions. SAIL records signed tie
IDs and clears both endpoint signs plus matching rotating attachments; a retied
corner, unrelated rope and another model with the same rope number survive.
All individual/group/destructor removal paths share that owner. Actual core
message dispatch and rigging methods pass ASan/UBSan across six lifecycle modes;
private rope-sail-lifetime retains the prior endpoint write-after-free and fixed
evidence. This does not establish the cause of the separate renderer-cache crash.

Mechanical impacts enter `SHIP_BASE::ApplyHullImpact`. TOUCH owns collision work
and ordinary HP damage; the material hook never charges HP a second time. Small
work doses retain their existing local-contact accumulation and expiry. The
common-material successor replaces the old artificial whole-hull split plane
with a finite crush field bounded by the participating root hulls' actual
overlap. Work limits removable material inside that contact; it cannot invent a
cut across remote wood. Exact shape/work/publication ownership belongs to
`naval-ammunition-and-damage.md`.
Active-player Phonk protection now rejects new physical damage as well as HP loss;
existing damage resumes after mode exit. The separate contact-motion adapter and
slow mass-dependent yielding policy belong to the collision solver.
Environmental contact uses the existing owner index-1, with the same ingress path.

Historical predecessor source evidence:6000 baseline motion comparisons, held boost/release/brake/air
and complete launch/landing checks; native VM contact/regen/laser/flood/OFF/pause
and saved-attribute reload cases; actual Flyingdutchman cut geometry with512
BVH-vs-linear ray comparisons (max51 candidate triangles, mean6.88). These
include frame-independent energy, dry/wet/paused ingress and plain-versus-charred
cut checks. Mechanical work-dose/expiry/contact-change and catastrophic transverse
plane checks pass at16/50/100ms; the native VM also verifies shared impact/ingress
HP budgets, environmental owner admission and saved-attribute reload. Actual
Flyingdutchman transverse cuts at the centre and±15% hull length separate
28–43% of authored surface into fore/aft chunks, with valid seams and retained
collision; this open mesh result is surface proportion, not measured mass. Native
translation-unit syntax checks pass. These
checks do not prove appearance, game frame cost or actual player acceptance.

The reopened lifecycle passes source-derived ASan/UBSan checks combining the
actual native cut/event body with real mesh splitting: ownership and attack
ordering, waterplane/keel failure, dry/small-wet negatives, mast exclusion and
load/pause/ack retry. The actual script VM passes own-Phonk structural death,
OFF, duplicate settlement, partial damage, environmental ownership, quest and
immortality guards and saved-attribute reconciliation. Crew fall/swim and
route/offset negatives, both mast stage orders and attachment cache invalidation
pass component checks; none proves the repaired battle frame.

Required replay remains held Shift/ordinary return, tsunami launch/landing,
contact survival/regen, aim hold/sweep/cut/fragment/char lifecycle, dry top trim,
wave-driven flooding, bisection, pause/OFF/save/load and an ordinary cannon shot
with Phonk OFF.

The black-splash source repair is separate: `splash`/`splash2` already bake
ambient into vertex RGB and now explicitly disable a second lighting multiply.
`splash.fx` joins canonical technique delivery. Black tsunami `SEAFOAM_PS`
particles are not reproduced by this finding: their ordinary technique already
disables lighting and their installed texture contains no visible black texels.
The player clarified that the unresolved effect is surface wake/splashes while
sailing, visible at particular camera angles. That producer remains under
investigation; do not close it from the ordinary splash correction.
The latest tentative location is ahead of the bow. `SEAFOAM::RealizeShipFoam_Particles`
uses `frontEmitter[0/1]` (`seafoam`/`seafoam2`, `water1`) for ordinary bow spray;
`frontEmitter[2]` (`seafoam_front`, `Sparcle2`) also serves tsunami and Phonk
water entry. Both use unlit particles with culling disabled; installed `water1` has no black
visible texels in any of its five mip levels (minimum visible luma80.06/255).
The hull-side carcass starts at zero alpha at the nose. Missing normals and
local particle lighting do not explain the camera dependence. The next decisive
evidence is the same splash in correct/black camera views, or its texture label,
shader entry and fog/local-volume inputs; source arithmetic alone cannot bind
the reported black patch to one of these producers.

## Status

Installed October 13 through the canonical Metal stage-only path. The exact
native patch, controller/menu and Faster input passed component, full PROGRAM
compile and installed-byte/signature checks; installation preserved SAVE/settings.
The player's sea replay found insufficient speed, barely visible illumination
and braking that changes the sliding direction. The installed successor adds
adjustable throttle and tsunami flight. A subsequent screenshot confirms flight
activation but rejects its jump/landing motion and water foam under an airborne
hull. The current correction replaces the crest kick and instantaneous landing
with hull-support departure and damped water entry, and gates both water effects
through one native contact query. Component acceptance and delivery are recorded
in `runtime.md`. The player now rejects that revision too: frozen/weightless
flight, weak landing without a deep hull dip or impact splash, and a water trace
still visible beneath the airborne ship. Retained track/particle draws and the
water-projected glow were not covered by the earlier insertion/emission checks.
The successor passes complete movement/model-matrix, particle/render, sound and
lifecycle component checks. Canonical delivery and the exact installed engine
belong to `runtime.md`; player assessment of this successor remains unresolved.
Music disposal before death/scene replacement is already installed in `802fbde`.
The October14 music successor is local source only: the player reports silence
across combat-state changes. The reproduced fallback victory path stops the loop
before its routed replacement is suppressed; the repair and its player-replay
boundary are recorded in `runtime.md`.
No agent game input or launch was performed. The current engine and chronological
installation receipt belong to `runtime.md`.

## Contract and owners

At local sea, **Esc → Phonk Mode** sits directly below **Цунами**. The same
button turns it off and resumes play. Map, land and boarding retain their menus.
W accelerates, releasing W coasts, S brakes, and A/D turn the hull through the
existing remappable ship controls. Speed is controllable below a hard **500-knot**
effective horizontal ceiling, including tsunami/whirlpool transport. There is no
minimum cruising speed or automatic acceleration to the maximum. Toggle-on
preserves current velocity. The independent motor accelerates at 120 knots/second;
the brake removes 240 knots/second along the current world vector. The hull turns
while momentum keeps carrying it sideways; countersteering and recovering
traction straighten the course. Ordinary/NPC propulsion, steering and scripted
impulses retain their existing response. The reopened high-speed contact repair
bounds collision-induced turning for all ships. Mode exit caps its own residual
motion to ordinary propulsion.
Grounding, collision, death, stopped/fixed/surrendered ships retain their guards.

Climbing a tsunami increases acceleration toward the same ceiling. The existing
6×6 hull-averaged `ShipRocking` surface owns buoyancy height and inclination.
Departure occurs when a rising hull would require a negative support reaction
to follow that surface; the moving wave and ship motion determine its actual
vertical velocity. There is no fixed upward kick or minimum launch speed.
Phonk alone uses a cinematic vertical scale: gravity grows smoothly with the
cube of horizontal speed from native gravity to8× at500. Native support remains
authoritative on the ramp, and measured launch velocity is conserved. This
shortens the high-speed flight; it is an arcade choice, not ordinary gravity.
Flight retains horizontal inertia without motor/braking/traction/currents, while
continuous angular response follows momentum in the hull's forward/side axes.

Contact requires a closing boundary and uses hull draft, including the root
geometry's bottom. It retains incoming position/velocity. The finite compression
then follows the current hull-supported water surface using latched normal
impact velocity; its response is solved for a minimum up to92% of root-hull
freeboard. No position/velocity hard clamp is used. A strong landing therefore
submerges most of the hull before recovery; a low-energy crest skim settles
without a fake slam. The response returns to ordinary supported heave when it
decays. Controls resume on contact. A later wave serial can launch again; the
same serial cannot launch twice. Ordinary waves alone do not activate flight.
Flight and momentum reset on load/death/mode exit.
Airborne shore contact uses actual-height island traces and retained keel support
as described above. Ordinary wet grounding remains unchanged. The inherited
TOUCH hull-admission solver can miss thin ships on extreme long frames; no
all-frame no-tunnelling guarantee is introduced.

`SHIP_BASE::HasWaterContact` defaults to the existing wet behavior; the Phonk
ship overrides it from transient flight state. ShipTracks stops new wake inserts
AND retained-row draws in air while rows age normally. Landing begins a separate
strip, so rendered triangles cannot bridge the flight. SEAFOAM hides hull foam
and stops particle emission/draws in air, using the existing particle processor
to age retained particles. The water-projected halo is hidden in air; purple
ship-local lights remain. Wet contact restores normal emission rules.

One default-zero native `GetWaterImpact` snapshot supplies strength, generation
and the interpolated contact position to the existing seafoam_front particle
system and ship_bow sound. The pulse is emitted at that fixedXZ, reprojected onto
current WaveXZ; sound starts once per new impact. Existing particle lifetime
ends the extra impact draw. Only Phonk caps the stock speed-based bow offset to
hull length. No parallel script flag or SAVE layout is introduced. TOUCH restores
hull samples and all transient motion after speculative movement.

The navigator's numeric speed uses raw native knots for the live player in
Phonk mode. Ordinary arcade scaling previously displayed250 as100; the physical
ceiling was already250. Ordinary, dead and NPC readouts retain their existing
scale, as does the spyglass. The saturated dial angle does not clamp the number.
The player subsequently rejects250 as too slow; the source ceiling and matching
motor/brake rates are doubled. This changes the speed range while preserving
approximately4.2seconds of held W from rest to maximum. Canonical stage-only
delivery installs this speed packet; the compiler receipt binds its exact patch
and built header. Player assessment of the new speed range remains unresolved.

The lilac halo peaks in the exposed area around the hull rather than underneath
it. Four local light sources sit outside the port/starboard surfaces so outward
hull normals receive light. The existing scene source catalogue and additive
water-surface rendering own these effects; all changed render states are restored.

`PROGRAM/scripts/sea_phonk.c` owns transient `Sea.Phonk.Active` and one looped
music voice, using the existing music-volume bus. `PROGRAM/sound.c` routes
ordinary playlist requests through that owner while active and retains the latest
requested ordinary track for restoration. Sea teardown/initialization, boarding
and death dispose the mode. No separate setting, permanent perk or save layout
is introduced. Native motion/light are owned by `ship-phonk.patch`, appended
to the existing ordered native patch stack.

Combat entry/exit retains the same live Phonk voice. `AIShip.c`'s
`Ship_PlayVictory` asks that owner before stopping music; its ordinary victory
sound and timer remain unchanged when the mode is off. `SeaPhonk_Music` checks
the existing native `MSG_SOUND_IS_PLAYING` query, rather than trusting the track
name alone, so a previously stopped voice can restart on the next music request.
This does not add a per-frame playback poll or alter scene/death disposal.
The Great Recipe composer pins the complete normalized `AIShip.c` input in
`tools/patch_great_recipe.py`; its reviewed input fingerprint must include this
victory-path correction. The frozen pre-receipt delivery bootstrap is unchanged.

The strongest existing facilities are the native transient drift/collision path,
ship point-light rendering, and script music bus. Reuse them; no replacement
physics engine, renderer, playlist scheduler or UI shell is required.

## Reopened tight-circle movement and saved mode exit

The player reports ordinary ships following tight arcs after high-speed contact,
and speed remaining after Phonk exit. The native contact producer converts power
to yaw using a size-independent `.02` multiplier; repeated contacts accumulate.
Full consumed movement with Flyingdutchman mass/inertia reproduces excessive yaw.
No-input/no-contact high-speed motion stays straight, and initialized live mode
exit already restores ordinary velocity. A separate full Save/Load reproduction
shows that serializing500 while clearing transient mode state on load bypasses
that exit projection. These are distinct source mechanisms, not proof of which
one produced an individual player observation.

The source successor identifies contact origin through the default-compatible
`SHIP_BASE::AddCollisionStrength` adapter and transient per-slot tags in `SHIP`.
Only the contact yaw component is admitted within hull lever/linear-kick and turn
capacity budgets; its remainder follows the exact native yaw damping. Linear
recoil, script impulses, helm and perks remain separate. Slot reuse/delete,
prediction, death, load and live mode exit own transient cleanup. No new saved
`STRENGTH` field or binary layout is introduced. Blanket impulse clamping was
rejected because it erased a pure scripted rotation.

An active initialized Phonk save writes the same ordinary exit projection to a
fixed-size state snapshot, retaining live momentum. Airborne saves normalize
both existing position records to the water surface; ordinary/NPC serialized
bytes remain unchanged. Full serializer/loader, mixed and queued forces,
prediction and prior jump lifecycle checks pass. Already-written unmarked
high-speed/yaw saves cannot be classified as mode motion versus scripted motion;
they remain unchanged and are an explicit compatibility limitation. Persistent
forced-contact stress is synthetic; it does not accept actual pair detection or
remove the inherited long-frame thin-hull admission limitation. Exact canonical
delivery and actual player movement replay remain owned by `runtime.md`.

A bounded October13 upstream scan finds the same geometry-independent angular
producer in [develop](https://github.com/storm-devs/storm-engine/blob/develop/src/libs/touch/src/touch.cpp).
The [release list](https://github.com/storm-devs/storm-engine/releases) and indexed
open/closed issue/PR search supply no applicable collision-yaw correction for the
pinned native source. The local change reuses its hull geometry, contact queue
and damping instead of replacing the collision solver.

## Music input

The Esc menu adds an adjacent Phonk-music toggle. `pchar.PhonkMusicMuted`
uses normal character-attribute SAVE persistence, defaults to music enabled and
survives sea teardown. Muting stops only the owned Phonk voice immediately and
returns to the remembered ordinary track; the central music router prevents
Phonk restart while muted. Unmuting resumes that same owner. Effects, ordinary
music and Phonk gameplay powers remain active. The menu and scene transitions
still require the player's replay after delivery.

The player selected any popular phonk song. This implementation uses
[RAIZHELL — Faster](https://ncs.io/Faster), an official
NCS drift-phonk release. `src/assets/audio/manifest.json` binds its direct
official download, exact bytes and music consumer. `tools/prepare_phonk_music.py`
reuses verified local bytes; canonical staging fetches the input when absent.
The downloaded audio is ignored, while the reproducible recipe is versioned.
The installed app retains the user's downloaded input; the source repository
does not redistribute the recording.

NCS creator-video terms are not a game redistribution licence. A public game
archive containing this recording requires the label's game licence; this
personal installation does not establish that licence. See the
[official usage policy](https://ncs.io/usage-policy). No paid licence is purchased.

## Acceptance

Compile the complete PROGRAM/menu chain against the native script VM, check
on/off plus land/boarding/dead rejection and restored music. Native drift must
show lateral movement after a turn and retain exact ordinary-off behavior.
Canonical `run.sh --stage-only` must consume the final patch and music input.
Installed menu, real ship movement and lilac appearance/music require an actual
sea replay; source/probe/build/staging results remain separate evidence.

The reopened jump contract requires the complete ramp/air/entry/recovery sequence
at the current500 ceiling: continuously changing hull attitude, preserved impact
inertia, a visible hull dip bounded by real hull geometry, a waterline-anchored
impact splash, and a continuous return to buoyancy. No wake, retained contact
particles or water-projected halo may draw beneath the detached player hull;
purple ship lights remain. Ordinary/NPC wet motion and effects, predictor
restoration, OFF/death/load cleanup and shoreline guards remain required.

Current attempt disposition and installed receipt belong to `runtime.md`.
