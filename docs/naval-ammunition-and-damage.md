# Naval ammunition cargo contracts, damage pipeline and combat mechanics

## Native capture-to-publication latency

The October18 follow-up reopens the preceding component-only result. The actual
eight-Manowar CORE/SHIP/GEOM driver measures6.594s cold damage,1.400s hot damage
and19.737s contact publication before the repair. Timing separates an18.371s
contact queue wait from166ms of actual projection: both GEOM slots were occupied
by other ships' cold material preparation. The two-slot owner now prioritizes
committed contact and cooperatively retires one unpublished background rebuild.

Required material readiness finishes during `SHIP::Mount`, using the existing
bounded workers. Identical pristine hulls share immutable surfaces/cells/layout
under the existing authored fingerprint; weak entries expire with live surfaces.
Damage, repair, foreign state and nonempty checkpoint tails retain their own
preparation. Reusing the surface while substituting a new layout was rejected:
it rebuilt the entire physical graph despite a cache hit. The accepted candidate
retains both owners together. Eight pristine ships take about9s of scene loading;
this moves cold work out of live hits, rather than making that work free.

The complete native thermal driver publishes the first hit in290ms, a hot hit
in163ms and contact in245ms. Thirty moving attempts admit29 samples and publish
four successive geometry revisions; the stroke drains404ms after its last input.
Ordinary Mount restores its six saved thermal tail records and four native ray
results unchanged; a foreign model record stays intact without a loading loop.
This verifies engine publication, not player aiming, worst battle FPS, flooding
or literal zero latency. The player scene remains unresolved. Optional
`STORM_METAL_DAMAGE_TRACE=1` records queue/work phases; ordinary play does no IO.
[Frozen binaries, source, commands and raw native results](../.local-archives/metal-renderer-evidence/destruction-player-followup-20261018/receipt.json)
own reproduction. The prior measurements below are component evidence.

The actual300-knot `TOUCH::Execute/ResolveShips` broadside case is larger than
that direct contact probe: first visual publication initially takes2.434s.
Whole-piece deletion now filters/remaps the existing exact connectivity graph;
only real pending shape edits reconnect. Authored-owner publication removes
cross-owner backward arcs using the same fence as ordinary reconnect. Five
stock-model removal cases, including a prior shape edit, match the full Boolean
reconnect reference in edges, component membership/volume and support measures.

Cap sources and512-triangle chunks retain the old clipping arithmetic and output
order. They use the existing synchronous four-chunk OS dispatch; contact and
weapon callers share two fork-group slots, with busy callers staying serial in
their existing worker. Child scopes inherit cancellation and the same CPU
allowance. Three large ownership masks produce identical cap and per-role/object
surface fingerprints, changed-object sets, area/moments/UV. Cancellation and
exceptions join before unpublished scratch expires. Whole-support boxes and
separating-plane shortcuts are rejected because their output differs; do not
reintroduce them as a speed fix without resolving that geometric discrepancy.
Current actual TOUCH publication is1.075–1.218s in two runs, with194–209ms worst
main frames versus2.434s/268ms before. Large
breakup latency therefore remains open despite the faster narrow laser/contact
path. Surface partition can amplify the stock source into over1.6million faces
in the half-hull oracle; replacing that algorithm needs an independent material
coverage/attribute oracle, not a relaxed old-output expectation.

## Integrated producer, contact publication and support repair

The user supersedes the audit-only handoff with a root-owned implementation.
`LaserState::sampledRay` now survives visual-only frames; the destructive cursor
spans consecutive admitted samples and resets across target/cloth/fragment/miss
boundaries. Held time accrues under backpressure, the first hit has no fixed
sample warm-up, and a completed stroke flushes without the extra burn timer.
The existing bounded thermal queue still owns dose merging and material work.

Finite material owns actual holes and disconnected sections. A conservative
waterplane no longer rejects a valid multi-loop material cut. Publication fences
keep model/generation/material identity, while subsequent node animation cannot
erase the captured hit. Unhit dependent masts join the same support transaction;
support-only results reach the existing mast release consumer.

Synchronous contact changes now retain publication metadata and feed
`ApplyPhonkLaserCutNow` once the render result arrives. This publishes openings,
remaining material, structural failure, flood capacity/owner and SAVE metadata.
Already transferred physical bodies are bound to their original transfer-step
identities; they are removed from the consequence result to prevent duplication.
TOUCH remains the impact HP owner (`payPartialDamage=false`). A release-only
follow-up checks dependent joints without applying a second weapon dose.

The stock Flyingdutchman replay uses40,445 faces,20 objects,5379 material cells
and486 structural cells. Narrow strong contacts previously failed admission;
local positive-cell edits plus paid coarse-support ownership now complete in
.825/1.360ms at half-widths.16/1m. Two successive contact deltas combine two real
bodies and two fields without double transfer. Deferred fine connectivity,
immutable decoration components and sparse material replacements avoid repeated
whole-hull work. Render preparation drops from571–612ms to112–159ms; contact
projection drops from39–41ms to.214–.356ms. A full-seed capacity oracle differs
by at most4.71e-6. Cold material bootstrap still costs about4s.

The actual.035s thermal beam still removes.017832112226m³ of positive timber,
opens.169768079184m² and changes a center ray from a hit to a miss. Its geometry
digest remains16259268190190756983; pristine release-only produces no false
fragments. Preparation/projection take74.753/6.934ms. These are CPU component
measurements, **not** capture-to-pixel latency or game FPS. Multi-node support,
flooding, repair/SAVE and sustained fire still require the rejected player scene
to be replayed; source repair and staging cannot accept that scene.

[Frozen sources, commands and raw results](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/receipt.json)
own reproduction. [Runtime](runtime.md#current-verdict) owns installed delivery.
The following audit describes the pre-repair counterexamples.

## Reopened player destruction: producer and publication audit

The22:47 player report rejects the405d6ec installed candidate: sweeping the laser
does not visibly cut reliably;200–400-knot rams mainly throw/spin ships; a delayed
bow hole can retain unsupported forward structure and no observed sinking.
This audit inspects the actual SHIP/GEOM/TOUCH/script chain at405d6ec. No gameplay
repair or installed acceptance is implied by the following source findings.

1. **Continuous laser motion loses intermediate aim positions.**
   `ship_phonk_laser.cpp::UpdatePhonkLaser` accumulates `sampleClock` until.035s,
   but refreshes `rays` before the `sampleTime == 0` return. The next stroke starts
   at the preceding *frame*, not the preceding destructive sample. In a constant
   20m/s,100FPS arithmetic replay,25 strokes cover only5m of a20m centerline;
   intervening centerline gaps reach.6m against a.32m beam diameter.60/120/240FPS
   also skip intervals. This isolates producer coverage, not tracing, visible
   pixels, material execution or game FPS. A stationary aim has no spatial gap.
   The destructive sampling cursor must survive visual-only aim updates and
   preserve target/muzzle discontinuities rather than bridge another receiver.
2. **Contact failure immediately chooses the complete rigid response.**
   `ship_contact_material_impl.hpp::CommitHullContactBreak` refuses work while
   `phonkLaserWork.active`; GEOM also refuses pending physical/render work.
   `ship_hull_material.hpp::tryContactEdit` can return incomplete at its finite
   candidate/piece/tetrahedron budgets. In `ship_contact_frame.hpp::integrateFrame`,
   only an accepted fracture replaces the old material and limits the impulse.
   Otherwise the original rigid impulse is applied; `touch.cpp` queues remaining
   `ApplyHullImpact` afterward. A later hole therefore cannot undo that earlier
   bounce. Increasing damage constants does not repair this ordering contract.
   A current-source stock Flyingdutchman check with valid horizontal contact,
   specificWork5000, half-height4m and half-widths.16/1/4/12m returns incomplete
   for the first two and one released body for each wider patch. The calls take
   .004–.044ms: this check proves selective admission, not the reported stall.
3. **The synchronous contact publication bypasses ship consequences.**
   `PollPhonkLaserWork` consumes `TakeHullContactResult` by binding fragment render
   tokens and setting `materialSavePending`. It discards that result's openings
   and never reconciles remaining hull, structural failure, flood capacity/owner
   or mast support through `ApplyPhonkLaserCutNow`. `SaveHullMaterial` serializes
   the existing metadata and advances both saved/binding revisions, so ordinary
   `EnsureHullMaterial` does not subsequently rebuild ingress. `SeaPhonk_Flood`
   runs only from known native breaches; `SeaPhonk_HullCut` requires publication.
   Thus newly removed contact material can have no corresponding water ingress
   or structural-failure event. An above-water hole alone is not proof a ship
   must sink; the missing contact-to-consumer transition is the proven defect.
4. **Removing a support does not necessarily schedule its attached mast.**
   `StartPhonkLaserWork` selects beam-intersecting nodes and then adds ancestors
   only for selected mast nodes. It does not include a mast merely because its
   supporting hull node is being removed. The worker checks joints only for
   captured `hasSupport` targets, while `DetachUnsupportedCutMasts` explicitly
   skips every finite beam. A hull-only removal can therefore leave a mast whose
   known contacts vanished. The separate contact path above runs no such joint
   reconciliation at all. The screenshot cannot identify the exact forward
   node; do not claim its particular attachment was reproduced by this audit.

The retained [contact source, commands and raw output](../.local-archives/metal-renderer-evidence/destruction-owner-audit-20261017/contact-receipt.json)
and [sampling conditions/results](../.local-archives/metal-renderer-evidence/destruction-owner-audit-20261017/sweep-receipt.json)
are counterexamples and diagnosis evidence, not acceptance tests. The initial
contact input used a nonhorizontal normal with a vertical tangent and is excluded;
the corrected run asserts field validity. Broad contacts succeeding also reject
the claim that all mechanical cuts are disabled.

The correction boundary is one accepted material transition feeding collision,
geometry, physical bodies, dependent supports, ingress, structural state and SAVE;
the beam producer additionally needs its own persistent sampling cursor. Existing
failure fences can retain requests until another revision/capacity change; the
current player's exact laser rejection or GPU/main-thread stall remains unbound
without a current execution trace. The older PID97749 profiles belong to the
preceding engine. Do not label the complete FPS cause proven by these probes.

## Material worker work admission and authored connectivity

The22:15 player profiles reopen laser latency and sustained contact FPS.
One worker reaches `fractureCells → clipCell → solidSectionCaps`; another
spends396/599 samples preparing convex physical contact, including171 in
`buildSource`. These are worker-stack counts, not request latency or frame time.

A CPU adapter using the installed Flyingdutchman GM and current GEOM bootstrap
exposes5379 material cells/486 graph cells. A radius.16/depth.84 beam takes
13.666s in material preparation. Even a zero-weapon release-only job emits8
groups:26 pristine connected components were collapsed into8 coarse source-prism
owners, so18 pre-existing islands were mistaken for newly severed material.
The expensive boundary partition/release work dominates the actual local cut.

`ship_hull_convex_material.hpp` now derives immutable ownership from its existing
positive-piece connectivity traversal on pristine material. Clipping propagates
that identity; merges require the same owner. Fresh-source reconstruction uses
the same graph, including after CHM3 decode/rebase, without a new SAVE field.
The old per-prism union is removed. A real new split still releases material;
zero body allowance retains it until capacity becomes available.
`clipCell` also admits strict whole-cell containment as empty and a separating
plane as unchanged before invoking closed-boundary Booleans. Touching/partial
cuts retain the established path.

On the same stock model and beam, material preparation becomes116.125ms plus
41.180ms contact projection; fragments8→0 and canonical faces105522→40562.
The center ray changes from a hit at.25 to a miss, with.017832112226m³ positive
timber removed and.169768m² opening area. Pristine release-only becomes a.029ms
no-op with identical positive volume. Cold bootstrap still takes4.626s; total
CPU worker time is4.783s and observed peak287MB versus about1.94GB baseline.
This is component evidence, not player request latency or game FPS.

Volume conservation uses positive convex pieces, not `closedCutVolume` on
assembled cell boundaries. A same-measure baseline/current comparison gives
581.236161864486m³ and identical37133-piece geometry digest; the boundary-only
diagnostic gives548.115378467255 in both versions because some assembled cell
boundaries fail its manifold check. The ownership repair changes no pristine
wood geometry. Actual CHM3 save/reconstruction retains a deferred split, then
releases it once capacity returns (positive-volume error4.62e−10).

The stock adapter also exposed a valid≈2.3e−6m³ piece rejected by planar contact
preparation; the [contact owner](ship-wreck-contact.md#positive-material-projection)
records its numerical correction. [Final commands, frozen inputs and results](../.local-archives/metal-renderer-evidence/contact-followup-20261017/laser/final_receipt.json)
and the [volume comparison](../.local-archives/metal-renderer-evidence/contact-followup-20261017/laser/volume_receipt.json)
retain the evidence. The adapter does not recreate player saved damage,
child-node poses, request queues, GPU publication or measured game FPS.

## October17 cleanup of deferred material ownership

`ship_hull_material.hpp::releaseSections` must return every separated section
beyond the fragment allowance to retained material; truncating its output alone
deletes wood without any physical owner. After fracture, the existing `seed`
rebuilds contact and physical cell/bond capacities from the actual kept surface,
while paid graph damage remains in the existing state. Replacing this with
`refreshContacts` left released cells alive in the deferred-capacity projection.
The producer also retains rejected exact requests behind its existing
revision/capacity fence; a fixed retry count must not discard paid work.
[Focused CPU evidence](../.local-archives/metal-renderer-evidence/gemini-cleanup-20261017/ship/)
covers zero allowance/material conservation, fracture and a subsequent
release-only no-op. Immediate aim/cut feedback and worker polling are preserved.
The inherited75% cooperative worker allowance and50ms quantum remain tuning
choices without a new whole-game latency/FPS measurement.

## October17 local destruction revision

The player rejects59a56f1 in the01:09/01:11/01:18 replay: the contact star
sticks, cuts arrive late without useful section release, terrain never opens,
fire causes little observed hull damage and resists suppression, and some
impacts stall the game. The requested outcome is continuous real local material
removal with connected-section release and bounded frame work. Source/build
evidence must not be represented as an accepted player replay.

Adopt the existing material cells and support graph, adapting the local convex
carving/connectivity mechanism described by
[Smash Hit's developer](https://blog.voxagon.se/2014/05/13/cracking-destruction.html)
and the [Blast support model](https://docs.omniverse.nvidia.com/kit/docs/blast-sdk/latest/docs/api/introduction.html).
Prefractured-only Chaos/Blast cannot provide arbitrary laser cuts; replacing the
entire asset/render pipeline with Teardown-style voxels would expand this fix
into an engine conversion. Preserve authored hollow hulls and volumetric land.

Order: establish current hit independently of retained destructive work; admit
local material edits against actual receiver geometry; realize connected pieces
and synchronize collision/contact response; then build/stage the combined batch.
Fire simulation/suppression and ground's local receiver path are independent
source lanes. Root owns shared APIs, ship producer, Git/build/app delivery;
material, ground, fire and contact lanes return source-only deltas.

Acceptance requires the current beam/star to follow every camera pose even under
queue pressure, progressive real wounds and eventual section separation, ground
excavation at the actual hit, recoverable finite fires, and impacts that spend
work on fracture while retaining motion. Unchanged material, sails, ordinary
weapons, saved damage, map transfers and player files remain valid. Existing
cooperative10% worker allowance was retained in this original batch; later
tuning is described above. Avoid whole-assembly work for an unrelated
local receiver and avoid main-thread joins. Invalid requests must retain an
explicit recovery owner without freezing new aim or every later request.
No gameplay tests, probes, agent launch or player-process control are authorized;
the canonical build/stage and source review precede player-owned acceptance.

The producer now traces the current aim before draining retained work. A bounded
round-robin dose queue keeps a busy receiver from owning later targets. Finite
beam bounds select affected authored nodes before material capture; unchanged
mast ancestors are captured only as support receivers and never projected as
damaged nodes. Ordinary hull fire ignition follows committed material removal,
instead of each speculative laser sample.

Same-step released material is reserved as native fragment bodies before the
physical hull snapshot changes. These bodies borrow immutable positive convex
parts, retain event pose and point velocity, and enter the existing fleet
fragment gravity/buoyancy/contact solver immediately. The paid worker projection
binds its ordered GPU handles to those existing bodies, without respawning them
or debiting HP again. Pending fragments use exact borrowed-volume ray queries;
one pending physical projection per receiver bounds the handoff lifecycle.

Read-only asset cardinality explains why the previous synchronous limits could
fall back before a broad ram changed anything. Run the existing
`experiments/native-metal/island_geometry.py:load_static_gm(Path)` reader against
installed `RESOURCE/MODELS/Ships/{Manowar1,LineShip1}/{name}.gm`; sum draw
`triangle_count` for the exact case-folded wood texture basenames admitted by
`ship_wood_surface_profiles.hpp`. Manowar1 (`victory2.tga`, `deck.tga`) has6234
wood triangles; LineShip1 (`linehullag.tga`, `linedeckag.tga`) has11404. The
existing three-tetrahedron prism representation would start at18702/34212
tetrahedra before live geometry rejection. These are source cardinalities,
not measured live cache or frame costs. Input SHA256 is
`7e33b7ae163d1e5ca2faab7c016cba9935623166b1279c497279ecfd5b41669d`
for Manowar1 and
`5c24c956d098f4053231b34ae880ac59ead1edd7e2d7d27066e2b2922d33cacc`
for LineShip1. The new broad-contact path transfers prepartitioned support
ownership instead of raising the per-frame plank/tetrahedron scan limit.

## October16,23:13 successor under player review

The player confirms mast release and visible scars in the22:50 image, while
section detachment, powerful-ram fragmentation and sustained FPS remain rejected.
The testing laser no longer drains its6.25s charge. Sampling retains admitted
elapsed time, subdivides consecutive muzzle/aim poses into bounded contact
quanta with fresh occlusion traces, and backpressures instead of dropping
accepted time. Compatible thermal samples share a value job without widening
their physical fields. Thin remnants retain actual material capacity and graph
connectivity; proximity alone does not delete wood between holes.

An operation that exceeds the available body allowance no longer rejects every
otherwise valid body. Newly emitted groups transfer actual geometry; the
remainder stays present in hull/collision with failed strength and is released
by bounded ownership-only jobs as capacity becomes available. Those jobs add
no second weapon energy, HP debit, crew hit or cosmetic burst. CHM3 derives
deferred ownership from retained failed material without a new SAVE schema.
Preparation/publication failures retain their exact accepted request and a
revision/capacity fence, report node/phase, and retry after relevant change;
they do not spin each frame or silently discard thermal/ram work. Legacy
bootstrap preserves material cells/structure across separation.

The23:16/23:19 profiles bind installed UUID0710C216. Dominant reflection calls
come from `worldDraws`/`worldCapDraws`; installed draw offsets show repeated
first/second UV copies. Identity triangle indices exclude those meshes from
the expanded-vertex cache. The direct compact GPU path now decodes all four
ordinary2D UV sets, preserving their authored mappings; lit/generated-coordinate
exclusions remain. Exact samples, screenshot and signed executable are retained
in the [profile receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-aa-deck-laser-2319/evidence.json).
This identifies removed redundant work, not measured post-fix FPS or accepted
separation. The aggregate cooperative geometry-worker allowance remains10%.

## Ram work and progressive mechanical damage

The October17 source calculation uses shipped `PROGRAM/ships/ships_init.c`
entries `SHIP_MANOWAR` and `SHIP_LINESHIP`, not an observed player's SAVE.
Their default Weight values are `Tonnes2CWT(4000)` and `Tonnes2CWT(2500)`;
Capacity is6600 and4850, respectively, and Lineship HP is6100.
`PROGRAM/utils.c::Tonnes2CWT` computes integer `tonnes*10000/508`, then
rounds upward to100CWT. `SHIP_BASE::GetHullContactMass` adds published
`Ship.Cargo.Load` to `State.fWeight`; `RecalculateCargoLoad` includes guns,
crew and goods. Setting Load equal to Capacity gives85400 and54150CWT.
These are game mass proxies: Weight already describes the authored ship's
displacement convention, and adding Load is the current contact rule, not a
claim that the resulting tonnage is a calibrated real vessel displacement.

For a central normal impact on a stationary, fully loaded Lineship, with no
initial yaw or tangential velocity, the exact source formulas give:

- `KNOTS2METERS(500)=500*1853/3600=257.361111m/s`.
- `mu=mA*mB/(mA+mB)`, restitution `e=.08`, normal impulse
  `J=(1+e)*mu*v`, dissipated work `D=.5*mu*v*v*(1-e*e)`.
- The target receives `D*mA/(mA+mB)` of dissipated work, then specific
  work `E=D*mA/(mA+mB)/mB-.25=12322.949879m²/s²`.
  Target impulse velocity is170.096238m/s, about330.4622kn.
- Using50.8kg per conventional CWT only to express those game units in SI,
  incident kinetic energy is143.673752GJ and dissipated work55.393351GJ.
  This conversion does not calibrate timber fracture or the game's mass proxy.
- Side-contact strength75 gives `f=E/(E+75)=.9939506`, or6063.098738 of
  6100 base HP before script immunity/admission. Native `hp*.1` and the
  collision handler's `fPower*10` cancel; they are not a tenfold energy loss.
  Bow strength can reach131.25; the same case gives6035.714457 HP.

The formula owners are `touch/src/ship_contact.hpp::solve`,
`touch/src/ship_contact_frame.hpp::integrateFrame`,
`touch/src/touch.cpp::ResolveShips`, `ship/include/ship_base.h`, and
`PROGRAM/sea_ai/AIShip.c::Ship_Ship2ShipCollision`.
Off-centre lever arms, actual normal closing speed, yaw, cargo, generated ship
stats and script admission change this representative case. A speed control
reading alone does not prove that closing speed reached the solver.

`SHIP::ApplyHullImpact` accumulates contact work for at most the live contact
episode, admits it above an HP-dependent4..40 strength-scaled threshold, and
caps its accumulator at1e6. Neither bound suppresses the central500kn case.
Its finite footprint uses overlapping root-hull contact bands and full root
support depth; empty space still consumes no graph capacity. Successful queue
admission clears the episode accumulator. Rejected preparation/publication
retains the exact request; deferred bodies retain real hull geometry and failed
strength. The graph budget is
`E/strength * (wholeStructuralArea/layout.area) * layout.volume`; when the two
area domains match, the side-hit example supplies164.305998 whole timber
volumes of capacity work. Damage is still confined to actual contacts inside
the finite footprint. `DamageResult.remainingWork` is currently not returned
to a later contact or enlarged footprint after successful publication. It is
unused local work, not evidence that the solver delivered it elsewhere.

Previously a subcritical mechanical hit updated graph health only: no skin
change occurred before a cell/link failure. Live mechanical preparation now
realizes only its already-paid crushing work, capped by actual contacted timber
volume, using a finite beam/cell intersection and a conservative16-step
signed-volume depth fit. Fully failed/released cells remain real fragment
material rather than ablation. Sparse physical capacity refresh caps paid
health by remaining section without charging the work again; overload and
island release remain graph-owned. Strong hits may still split instantly.
Actual mechanical geometry folds the existing CHM3 checkpoint; historical
health-only tails retain their old semantics through a transient live-capture
flag. Canonical compilation/staging passed on October17; the installed identity
and exact delivery receipt belong to `runtime.md`. Actual progressive damage,
high-speed fragmentation and FPS remain for player replay.

## Status

The common hull-material baseline was installed on October16,20:52EDT;
the player's later replay rejects reliable laser/ram separation. The graph-based
successor installed on October16,21:40EDT is also rejected by the21:50 player
replay. The local-ablation/contact successor below is installed on October16,
22:13EDT. The next player replay confirms some delayed damage, but rejects
section detachment, disappearing ram material and mast separation; it also
supplies the22:23 crash below. The correction is installed22:50EDT; actual
separation, direct mast contact and FPS remain for player replay.
`runtime.md` owns delivery and replay evidence.
The ammunition baseline below was
documented on October6. It captures the naval combat ammunition
pipeline across Storm 2.8 and Native Metal, cargo vs character inventory boundaries,
damage reduction math, and technical integration specifications for custom shot types.

## Common hull-material implementation contract

October16 successor in source: the player's repeated failure to obtain a real
laser/ram separation reopens this implementation. The installed22:13 batch
is a partially working, crash-rejected baseline. A finite thermal dose removes actual local timber
immediately; a coarse graph of actual timber sections and shared load paths
decides structural strength and additional separation. Local mechanical work is
spent in travel order; surviving connections
carry the remaining section load and can overload. Geometry realizes failed
cells/bonds and graph islands, including a remaining thin visual strip; exact
mesh connectivity no longer decides whether that strip supports the whole hull.
Empty hull space is never filled with a bounding-box solid. Bow/deck/keel strength
comes from present material and connection area, not a separate HP multiplier.

Ready-solution comparison: NVIDIA Blast's
[support/bond model](https://docs.omniverse.nvidia.com/kit/docs/blast-sdk/latest/docs/api/introduction.html)
and [stress solver](https://docs.omniverse.nvidia.com/kit/docs/blast-sdk/latest/docs/api/extensions/ext_stress.html)
provide the relevant graph/health/island mechanism. This runtime already owns its
GM meshes, collision bodies, worker allowance and SAVE transport; adapt that
bounded graph mechanism in the existing material owner rather than add a second
physics/authoring stack. The structural approximation is game-specific, not a
calibrated timber stress simulation. Health-only updates must not copy/rebuild
the whole boundary or upload GPU buffers. Thermal ablation and actual fracture
change geometry; a thermal notch must not wait for a whole coarse cell to fail.
State persists in the same versioned checkpoint/bounded tail and follows existing
repair, reset, encounter and map consumers. Sustained pressure receives
no invented damage work: TOUCH retains only actual dissipated closing energy.
Compile-only evidence and the remaining player replay must stay distinct. This
corrected successor is delivered through the same previously requested app
installation; no agent gameplay replay is performed.

The player uses the laser to expose defects in the destruction system, not as
an isolated weapon feature. A continuous finite laser stroke, concentrated
round-shot/bomb/knippel impacts and a physical ram must remove material from the
same hull and weaken the same connections. A completed cut through a load path
must release its real section; unrelated damage elsewhere must not fabricate a
whole cut plane. Weapon-specific footprints and heat remain distinct. Script HP,
crew casualties, immunity, quests and TOUCH collision work retain their current
owners; publishing a mechanical hole cannot debit the same HP twice.

Source discovery found two independently maintained material projections:
accepted artillery hits entered `Blots::AcceptedHullHit`/`GEOM::SetHullCuts`, while
laser/ram fields entered `SHIP::ApplyPhonkLaserCut`. The rejected predecessor combined them
by replaying authored geometry, every cannon mark and every laser operation.
The finite ship history also stops after512 sample-cost units; increasing that
limit increases replay cost, and deleting old operations would restore wood.
These mechanisms do not satisfy the requested continuing destruction contract.

The shared owner is GEOM's retained material, published by SHIP's existing
atomic value-job queue. The successor reuses material-role meshes, broadphase,
finite local material clipping, fragment physics and the aggregate10%
geometry-worker allowance. Its checkpoint preserves outer skin, canonical caps,
timber interior and finite walls separately, including object IDs, UVs, normals
and colours. A rebased clipping boundary must not replace the immutable authored
support domain or grow timber again. Current-material queries, visible/collision
geometry, support loss, ingress and the saved checkpoint publish together.

`Ship.HullMaterial` is an additive versioned attribute checkpoint, bound to the
model's authored content and a monotonic material revision. Ordinary attribute
SAVE serialization remains the only persistence transport; binary SAVE layouts
do not change. A complete checkpoint is authoritative. Older `Ship.Blots` and
`Ship.PhonkLaserCuts` reconstruct once through their established order, without
HP, crew, chips or historical fragment replay. Renderer reset restores current
material, and stale/cancelled/malformed projections retain the last valid state.
The CHM3 node payload preserves graph health/alive values bound to a deterministic
authored-layout fingerprint and distinguishes physical thermal tails from the
old CHM2 health-only tails. CHM1/P tails retain their old exact semantics. Old
formats import/fold once, reconciling already disconnected sections without
replaying historical fragments or converting latent damage into new holes.
Health-only publication has its own
material revision, so it cannot invalidate collision/convex-contour caches.
Only bounded pending work and current retained geometry remain live; an ever
growing operation log must not be required to continue damaging a ship.

`SHIP::ApplyHullProjectileImpact` owns the common artillery producer, including
the near-hull radial-burst caller in AIBalls. Its native profile identifies live
goods through the projectile registry, not copied goods IDs. Accepted script HP
loss is inverted through TOUCH's existing work curve E/(E+strength); strength
remains `SHIP_BASE::GetHullContactStrength`. Wrecks use the existing shot mass
and trajectory derivative without another HP event. This is a whole-hull
specific-work domain and an area-density material approximation, not calibrated
timber engineering data. The finite local contact footprint admits weapon work;
failed cells and overloaded connections determine the resulting fracture. The
coarse lattice uses about five cells on its shortest material axis, aspect-aware
longer axes, and a bounded sparse occupancy. Actual timber volume and shared
section determine capacity; empty holds are not structural cells. A four-pass
reserve check uses20% of intact incident capacity as the section's declared game
load, scaled by remaining material. This is an explicit5x design reserve, not a
gravity or calibrated stress solver. Direct laser depth retains its existing
delivered-time input and subtracts a finite local volume from current timber.
The graph reconciles actual lost volume and connection section once; it does
not spend that dose again as hidden damage. A local closed-material connectivity
comparison detects separation within one coarse cell; only a new split invokes
whole-section ancestry/ownership reconciliation. The local oracle reads one
shared neighborhood of actual closed source cells before/after; it does not
construct artificial six-plane box intersections/caps for every grid cell.
Mechanical subcritical hits can still change strength without fracture. Failed
mechanical cells transfer real boundary roles into bounded connected-region
bodies, using surviving physical contact between cells, rather than deleting
their wood. Failed support and physical vaporization are different operations.

The outer envelope of a hollow hull is allowed to open into its hold. Its filled
closed volume is not the timber's volume, and cannot veto an actual breach.
Closed MaterialCells own real wood; the generic solid/fragment cutter retains
its separate closure/conservation checks. A ram uses its finite projected contact
patch, not a single centre ray which can pass through an open gunport and discard
the entire impact. TOUCH's impulse and dissipated specific work are unchanged.

The shared kernel/checkpoint, GEOM/SHIP publication, legacy Blots import and
immediate crew-field consumption have one native owner. Finite ablation/crush
plugs produce bounded cosmetic chips; real bodies come from retained sections
that actually lose support. Already detached bodies retain their contact and
lifetimes. Legacy separating planes retain their detached section semantics.
Existing
shore, fire-anchor and fragment-query source fixes remain in the same successor.
The player owns gameplay testing. The integration owner may compile the engine
to catch C++ integration errors; compilation does not accept gameplay or FPS.
Canonical delivery uses `run.sh --stage-only`; this correction is installed22:50EDT.
Game control and gameplay probes remain declined.
Source review must distinguish local
notches from complete cuts, preserve old damage across another weapon/reset,
reject stale publication, and preserve undamaged/free-water behavior. Gameplay
acceptance remains open until the player observes these outcomes. Generic
cross-node attachments require real model support data; a transform origin is
not an attachment socket and cannot justify deleting neighboring assemblies.

Performance is a core acceptance criterion. Merely moving an ever growing
whole-hull reconstruction behind a worker does not close it. The shared material
implementation retains stable authored cell IDs/bounds, sparse current overrides
and a bounded32-record tail before folding checkpoints. Health-only damage shares
the immutable geometry/contact BVH and copies only bounded graph state. Thermal
Boolean work visits nearby faces/cells and contact-capacity reconciliation visits
only changed authored source solids, with sparse BVH replacements. Whole role
copies, role-BVH builds and collision/render flattening still occur in the worker.
Workers pack checkpoints; frame publication
reuses unchanged packed bases and writes a bounded tail. A fold still serializes
the changed node's full current boundary, including untouched authored faces,
alongside sparse changed cell solids. Actual fracture still traverses the node's
role meshes and rebuilds its material/contact/collision indices and GPU upload.
Ordinary local thermal publication uploads only changed authored objects; an
empty replacement suppresses that object's original draw instead of regrowing it.
These operations have a real cost; sustained cutting/FPS
and memory remain unmeasured until player replay. This is not fully sparse mesh
persistence or an accepted frame-time result.

Full repair, ship replacement and quest/traffic restoration share
`ShipsUtilites.c::Ship_ResetHullDamage`. Partial paid repair writes cumulative
`Ship.HullMaterialRepair.percent`; native material acknowledges only committed
work and restores actual authored material under area quota. Encounter handoff
copies checkpoint/repair/legacy/flood state, while map visuals use the same
versioned node decoder as SHIP. The ordinary attribute SAVE codec is unchanged.
Partial material addition and checkpoint folding are implemented in the installed batch;
compilation and source review cannot establish SAVE/repair gameplay acceptance.
Mast self-support uses the authored bottom-contact vertex set and base-to-tip
path. When the old tip is gone, the check follows the highest actual surviving
material with the same authored ancestry; removing the foot can still release
that survivor. Cross-node mast-foot support is a separate worker contract below.

### Mast support across authored nodes

`ship_mast_support.hpp` owns finite triangle-contact regions between an authored
mast and its authored ancestors, including a supporting parent mast. Bounds only
select candidate triangles; transform origins, a single nearest point and bbox
overlap do not establish support. Any surviving finite part preserves the mast;
complete loss of all known contacts publishes existing `supportSevered` and uses
native `MastFall`. Missing authored contact is unresolved and preserves geometry.

The existing per-cut value worker derives the regions and checks current retained
receivers. `LaserBinding` keeps a transient cache keyed by both immutable authored
sources, geometry generations and composed local relative transforms. Publication
fences every participating generation/collision/material revision and hierarchy
transform. Load/repair/reset reuse the existing binding lifecycle; SAVE has no new
layout. Native `pMasts` authorizes the mast's authored outer boundary and parent
mast receivers. A hull receiver still needs admitted timber. Root-only
`ship_wood_surface_profiles.hpp` deliberately excludes mast/yard/shatter files;
an empty mast wood map must not disable native mast support or collision.
No per-frame whole-hull attachment traversal is introduced.

### Direct ship-to-mast contact

The early TOUCH hull solver remains unchanged. Its owned late `TOUCH_MASTS`
coordinator captures all mounted ship frames after movement, including wrecks
whose hull no longer intersects the waterline. It queries every finite swept
native mast axis against current receiver geometry through MODEL::Clip's normal
sphere/hierarchy/geometry broadphase, then publishes all events after queries.
Previous/current receiver-local endpoints define two swept triangles; axial
degeneracy uses the actual collinear interval. This is linear within-frame
relative motion, not an exact curved rotational sweep or a flexing mast solver.

The old ship-contact0.5s snapshot consumer is removed; island tracing retains its
cadence and ball damage is unchanged. Closing work is
`0.5 * normalSpeed^2 * (otherMass / totalMass)^2 * (1 - restitution^2)` in the
existing specific-work domain. Capacity is the declared game scale75 times
actual authored mast surface area / admitted hull timber area, with node-scale
area conversion. GEOM caches authored area during its existing source decode;
no per-frame triangle measurement or guessed bbox capacity is added. Pending
bootstrap has no invented capacity and becomes eligible when that owner is ready.

The native `SHIP_MAST_TOUCH_SHIP` event appends a bounded damage increment
(`llffffaaf`); AIShip consumes it instead of fixed0.2. Strong work can sever one
mast immediately. Contact-entry state and old-axis overlap suppress repeated
damage while resting against it; mount/load/teleport seeds continuity without
swept damage. Native generation/revision, Phonk and script immortality guards
remain. This paired engine/script contract requires canonical joint delivery.
The live contact pass is outside the10% background geometry-worker allowance;
fleet pair/candidate costs and real mast/FPS behavior remain player-unverified.
This collision correction does not invent laser shell thickness for unclassified
mast/yard material.

### Clipper stack-guard crash in the local split oracle

The22:23 report binds installeddecf7a02/UUID71AC6C6A. Its worker stack repeats
`ClipperBase::CheckSplitOwner`4731 times through `localMaterial`/`newlySplit`.
Raw report and matching signed engine are retained in
[the crash receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-cut-crash-2223/receipt.json).
Removing artificial local partition caps avoids that unnecessary caller.

The vendored Clipper2 version is2.0.1. Its existing fixes for
[#987](https://github.com/AngusJohnson/Clipper2/issues/987) and
[#1029](https://github.com/AngusJohnson/Clipper2/issues/1029) do not guard the
empty-split recursion before `GetRealOutRec`. The current upstream function
still has this branch; [open issue1084](https://github.com/AngusJohnson/Clipper2/issues/1084)
describes the same stack-overflow family. The later
[FixSelfIntersects change](https://github.com/AngusJohnson/Clipper2/commit/4da1564fb1ae69ad14b334a677c0e34a88aa8e7a)
changes another operation, not this traversal guard. The bounded local correction
marks empty split records with the existing per-target visit tag before descent,
as the live-record branch already does. Polygon ownership/triangulation and
generic solid geometry are preserved; no library-wide replacement is introduced.

## Contract

All naval ammunition (balls, grapes, knippels, bombs, powder) is ship cargo (`cargo.goods`),
measured in hundredweights and units, stored in the ship hold, and transferred only via
ship-to-ship transfer (`transfer_main.c`) or port stores. Ammunition never exists in personal
character inventory (`pchar.items`), personal chests, or cabin wardrobe containers.

## Scope

In scope:
- Cargo vs inventory ownership boundary and goods definitions.
- Gun deck firing pipeline and ammunition reload cycles.
- Ballistic flight simulation, particle attachments, and billboard atlas rendering.
- Collision dispatch, damage formulas (hull, sails, masts, crew, guns, fire, panic, surrender).
- Why round shot and bombs currently fail to kill crew (mathematical proof).
- Technical specification for integrating new ammunition types into Storm scripts and Native Metal.

Out of scope:
- Native Metal volumetric collision math (owned by `docs/sea-cannon-ballistics.md`).
- Manual trajectory overlay UI (owned by `docs/sea-aim-overlay.md`).

## Architecture and Owners

### 1. Cargo vs Character Inventory boundary

| Domain | Data Owner | Script / Engine Files | Notes |
| --- | --- | --- | --- |
| Ammunition & Powder | `rCharacter.Ship.Cargo.Goods` | `PROGRAM/store/goods.h`, `initGoods.c`, `storeutilite.c` | Registered as `TRADE_TYPE_AMMUNITION`. Measured in units and weight (cwt). Transferred via `transfer_main.c`. |
| Personal Items | `rCharacter.items.<itemID>` | `PROGRAM/items/items.h`, `items_utilite.c` | Sabres, pistols, spyglasses, potions, amulets. Stored in personal pockets or cabin chests. Never holds cannon shot. |
| Cannon Batteries | `rCharacter.Ship.Cannons` | `PROGRAM/cannons/cannons_init.c`, `cannons.c` | Gun calibers (lbs), types (cannon vs culverine), max range, reload time, damage multiplier. |

### 2. Gun Deck and Firing Pipeline

1. **Charge Selection**: Selected via `BattleInterface.c` (`hk_charge1` to `hk_charge4`, command `BI_Charge`). Sets `pchar.Ship.Cannons.Charge.Type` (`GOOD_BALLS`, `GOOD_GRAPES`, `GOOD_KNIPPELS`, `GOOD_BOMBS`).
2. **Reload Calculation**: `AICannon.c` (`GetCannonReloadTime`):
   `fReload = fBaseReload * (1.0 + (1.0 - fCrewExp) * 3.0) * (1.0 + (1.0 - fMorale/MORALE_NORMAL) * 0.2)`.
   If crew is below optimal or injured, reload time is tripled. Powder must be present in cargo hold; without powder, cannons cannot fire.
3. **Muzzle Origin and Dispatch**: `AICannon::RealFire` in `src/libs/sea_ai/src/ai_cannon.cpp` emits `CANNON_FIRE`, calling `Ball_AddBall` in `PROGRAM/sea_ai/AIBalls.c`.
4. **Trajectory and Dispersion**:
   - `SpeedV0 = Cannon.SpeedV0 * Goods.SpeedV0` (Balls 1.0, Grapes 0.6, Knippels 0.9, Bombs 0.8).
   - Angle and direction dispersion calculated from cannon skill (`Accuracy`).

### 3. Visuals, Particles and In-Flight Rendering

- **Render Technique**: Billboard quads rendered via `balls.fx` using `AIHelper::pRS->DrawRects` in `ai_balls.cpp`.
- **Atlas**: `RESOURCE/Textures/AllBalls.tga` with `SubTexX = 2`, `SubTexY = 2`:
  - Index 0: Bombs (`Particle = "bomb_smoke"`, `Size = 0.3`).
  - Index 1: Grapes (`Size = 0.2`).
  - Index 2: Balls (`Particle = "ball_smoke_low"`, `Size = 0.2`).
  - Index 3: Knippels (`Size = 0.2`).
- **Water & Obstacle Hits**:
  - `BALL_WATER_HIT` spawns particle `splash` or `splash_big` and plays `ball_splash`.
  - `BALL_FLY_NEAR_CAMERA` plays `fly_ball`.
  - Floating cargo in water: `Goods[...].Swim.Model` (e.g. `roll_of_materials`, `box_of_bottles`).

### 4. Collision and Damage Dispatch

Collision sweep checks sails (`CheckSailSquar`), masts (`SHIP::Cannon_Trace`), hull, fort, island, and water.
On hull impact, `SHIP_BALL_DAMAGE` dispatches to `Ship_ApplyBallDamage` in `PROGRAM/sea_ai/AIShip.c`.

#### Why Balls and Bombs Fail to Kill Crew (The Math)

The crew damage formula in `AIShip.c:3295` is:
`fCrewDamage = stf(rBall.DamageCrew) * fCannonDamageMultiply * AIShip_isPerksUse(...) * GetCrewDamageReduction(rOurCharacter)`

Followed by `Ship_ApplyCrewHitpoints` in `AIShip.c:2708`:
`fDamage = fCrewDamage * (1.0 - 0.75 * stf(rOurCharacter.TmpSkill.Defence))`
`if (Doctor2) fDamage *= 0.8; else if (Doctor1) fDamage *= 0.9;`

Base parameters in `initGoods.c`:
- `Goods[GOOD_BALLS].DamageCrew = 0.2`, `DamageHull = 11.5`
- `Goods[GOOD_BOMBS].DamageCrew = 0.5`, `DamageHull = 19.5`
- `Goods[GOOD_GRAPES].DamageCrew = 2.4`, `DamageHull = 1.0`
- `Goods[GOOD_KNIPPELS].DamageCrew = 0.3`, `DamageHull = 1.5`

Mathematical degradation on a healthy target (100% hull):
1. `GetCrewDamageReduction` returns 0.35 to 0.55 for Class 1-3 ships (65% to 45% reduction).
2. Captain Defence skill (0..1) reduces remaining damage by up to 75% (`1 - 0.75 * 1.0 = 0.25`).
3. Doctor perk reduces another 20% (`0.25 * 0.8 = 0.20`).
4. Result for Balls: `0.2 * 0.35 * 0.20 = 0.014` crew lost per ball hit.
   A 40-gun broadside hitting cleanly kills 0.5 to 1.0 sailors while dealing 2,000+ hull damage.
5. Result for Bombs: `0.5 * 0.35 * 0.20 = 0.035` crew lost per bomb hit.
   The target ship sinks long before losing even 5% of its crew.
6. The 25% immortal floor (`MinCrew * 0.25`) prevents crew death from falling below 25% of minimal crew via artillery.

#### Existing Secondary Damage Mechanisms in Scripts

1. **Gun Destruction**:
   `SendMessage(&AISea, "laffff", AI_MESSAGE_CANNONS_BOOM_CHECK, rOurCharacter, fTmpCannonDamage, x, y, z)`
   Evaluates gun battery explosions at coordinate `(x, y, z)` via `AIShipCannonController::CheckCannonsBoom`.
2. **Fire and Explosion**:
   `bInflame = true` triggers `PostEvent(SHIP_ACTIVATE_FIRE_PLACE, ...)` and `PostEvent(SHIP_FIRE_DAMAGE, ...)`.
   Critical boom (`bSeriousBoom`) triggers `Ship_Serious_Boom(x, y, z)` with 8..12x hull damage.
3. **Surrender at Sea**:
   `Ship_CheckSeaSurrender` evaluates whether the enemy captain yields based on remaining fleet threat.
   Sets `rCharacter.SeaSurrender`, `rCharacter.Surrendered = true`, and raises the white flag (`FLAG_WHT`).

### 5. Boarding and Prize Cargo Lifecycle

1. Boarding triggers `LAi_boarding.c`: deck combat phases, then captain cabin duel or cabin surrender.
2. After victory, `transfer_main.c` launches the sea transfer interface:
   - Cargo holds are opened between player flagship and captured prize.
   - Ammunition, food, rum, and trading goods are transferred via cargo slots.
   - Prize ship can be captured with an officer captain or scuttled / sunk.

## Specification for New Ammunition Types

### Heated-shot perk

October 13 source candidate. `HeatedShot` is a passive ship perk learned by the
hero or inherited from the assigned cannoner, requiring `CannonProfessional`
and `ImmediateReload` through the existing perk tree. Old saves rebuild the
registry through `PerkLoad` without granting or clearing learned perks.

`AIBalls.c` rolls `rand(4) == 0` once at launch, only for `GOOD_BALLS` with the
effective perk. The existing saved projectile event string carries
`@heated-shot`; cargo, velocity, direct damage and the binary SAVE layout remain
unchanged. `native-metal/heated-shot.patch` consumes that tag as data (never as
a script callback), tints the ordinary sprite warm red and resets
`CurrentBallHeated` before each projectile collision. Ordinary shots and other
ammunition must not inherit a preceding heated shot's flag.

`AIShip.c` admits local hull/cloth ignition with 20% probability for a heated
hit through the existing fire owner. A true hull critical retains its 50%
chance and combines the independent 20% hot-shot contribution into one 60%
roll and one fire site. A noncritical round-shot hit therefore has a 4% overall
hot-shot ignition probability before suppression, not a guaranteed fifth shot
or fifth hit. Other ammunition retains its existing fire probabilities.
The initial cannon-professional icon alias was rejected as incomplete identity.
The corrected source uses `PERK_HEATEDSHOT_ENABLE` / `PERK_HEATEDSHOT_DISABLE`
and a distinct hot iron ball held by furnace tongs, matching the existing rope
frame and golden perk inset. `src/assets/ui/manifest.json` owns the retained
generation/cleanup sources and the checksum-bound 256x128 TX sheet: learned
128px tile on the left, sepia unlearned tile on the right. Character menus and
prerequisite links both use `GetPerkPictureGroup`; the two old generic aliases
are removed. `prepare_perk.py` reproduces this format and the existing raking
sheet without changing that prior asset. It adds no cargo goods or fire timer.
The read-only installed delivery plan admits the exact new texture and rejects
an unrelated texture path. Decoded TX pixels equal the resized canonical source,
both full-bleed 128px states were reviewed beside the existing perk, and the
real script VM passes `HeatedShotArtCases=0`, `script_errors=0` for learned /
unlearned group selection and unchanged raking / ordinary perk groups.
This accepts source, format and consumer selection; the installed menu remains
unreplayed while game control is stopped.

Random NPC acquisition was a separate limitation in the original perk batch:
`PROGRAM/Loc_ai/LAi_equip.c:LAi_NPC_EquipPerk` granted a fixed eight-tier cannon
list, rather than the dynamic registry. It included neither `HeatedShot` nor
`RakingFire`; player learning/assigned-officer inheritance does not imply random
NPC acquisition. The original source/art batch did not change that selector.
The subsequent registry-driven NPC replacement and its delivery state belong
to `npc-generation.md`; its isolated VM proves HeatedShot reachability with
actual prerequisites, while real encounter replay remains pending.

Acceptance requires learning from both the hero and assigned cannoner,
red shots mixed with ordinary shots, local ignition on a vulnerable target,
and unchanged non-perk/other-ammunition behavior in the installed Metal game.
The canonical native build consumed the new last-stack patch successfully.
The refreshed `script-vm-probe` compiled the full installed PROGRAM chain with
the current read-only delivery projection and passed `HeatedShotCases=0`,
`script_errors=0`: registry/prerequisites, preservation of a prior learned perk,
hero/officer inheritance and removal, 20%/60% chances, and unchanged other ammo.
Only the fixture's `Main` is renamed to prevent real game startup. Calling
the dynamically unloaded perk-init segment twice within one uninterrupted probe
run produces an existing extern-call error; the accepted fixture uses the
normal single initialization and makes no claim about complete SAVE reload.
Source/native VM checks cannot accept the battle replay. Installation and
game input remain outside this source batch while the prior game-control stop
is in force.

To add custom ammunition cleanly without breaking native engine stability:

1. **Registry**: Extend `PROGRAM/store/goods.h` (`GOODS_QUANTITY` and `GOOD_*` defines) and `initGoods.c`.
2. **Visual Atlas**: Expand `RESOURCE/Textures/AllBalls.tga` from 2x2 to 4x2 sub-textures, updating `SubTexX = 4` in `PROGRAM/sea_ai/AIBalls.c`.
3. **Particles**: Add trail and detonation particle effects in `RESOURCE/Particles/`.
4. **Hit Dispatch**: Add custom logic in `AIShip.c:Ship_ApplyBallDamage` for special status effects (gun dismounting via `AI_MESSAGE_CANNONS_BOOM_CHECK`, steering damage, panic fire timers, moral shock).
5. **HUD Selector**: Expand `BI_ChargeState` in `PROGRAM/battle_interface/BattleInterface.c` with extended hotkeys or shift-combos.
6. **Cargo Distribution**:
   - Fort arsenals via port authority dialogues (`Common_Portman.c`).
   - Bermuda blacksmith craft order (`Pirates_Shipyard.c`).
   - Contraband beach merchants (`canbecontraband = 1`).
   - Plundered enemy warship holds in `transfer_main.c`.

