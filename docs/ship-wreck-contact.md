# Ship collision and contact with sinking ships

## Status

Player acceptance of the installed finite-cutter integration is reopened: intact
rammed hulls rotate without visible fracture. A source-bound native CORE contrast
separates admission from commit: a Flyingdutchman ram with work1000 reaches
`StartPhonkLaserWork` but rejects its snapshot, while held manual laser on the
same actual model commits two cuts. This rejects the hypothesis that the entire
asynchronous cut owner is disabled. The high-speed contact probe also supplies
work above the material threshold and adds yaw once; an arbitrary angular cap
would hide the intact-hull symptom without repairing the failed cut. The
native fixture identifies the rejection: a rotated node converts the admitted 2m
radius to 2.00000024 local units, which incorrectly exceeds a physical-metre
limit. Physical input/SAVE admission and local numerical validity now have
separate owners, and SHIP producer/restore admission uses the shared physical
limits before node conversion. The coherent native successor commits the same
work1000 impact at frame113; manual and automatic laser cases also commit.
Exact current-library inputs and raw frame/cut records are retained in
`phonk-current-20261016`. This accepts the cut transaction component; the actual
collision solver and player's visible battle remain separate replay criteria.

The October16 collision successor is under integration. Ordinary admitted rams
use finite, overlapping crush pockets in the existing retained-mesh cutter;
large work increases local penetration instead of triggering a halfspace split.
The actual retained
skin is traced before choosing the cut origin. This represents recessed/broken
planking, not elastic deformation of every board. The finite cutter preserves
genuine interior voids; it must not fill a hollow hold to manufacture a dent.

Contact damage height comes from the overlapping vertical bands at the selected
impact time, including a sinking or descending hull; it is no longer model-local
Y0. `SHIP_BASE::GetHullContactMass` owns bare hull plus published cargo load for
TOUCH and detached-body reactions. `GetHullContactStrength` keeps one bow/stern/
side reinforcement curve for HP admission and material thresholds. The
zero-duration `ApplyHullContactMotion`
branch applies only live velocity/yaw impulse after Execute, without predicting
another step or moving the already presented pose. Ordinary positive-duration
TOUCH integration is unchanged.

Saved ingress adds draft by water volume / original waterplane area. The actual
4Hz ingress/wave sampler publishes each breach's transient wet area; its centroid
supplies bounded, damped pitch and heel toward the breached
side. Dry cargo immersion remains its existing attribute; water volume remains
`Ship.PhonkFlood`, so repeated frames and loading cannot add draft twice. Death
captures the presented list through the existing pose owner. This is a bounded
hydrostatic approximation, not compartment-by-compartment liquid simulation.

Actual-method CPU sanitizer checks cover finite ordinary damage, remote and
opposite-side preservation, high-energy local/repeated/rejected cuts, prior ingress,
frame-rate/pause draft/list, live-to-dead list transfer and late ordinary/Phonk
impulse without pose rewind. The saved volume remains authoritative; wet-area
cache reconstruction takes the next ordinary ingress tick.
The contact frame comparison preserves every motion/energy value from4832b2e
while testing unequal/sinking/landing heights and airborne/separating negatives.
Native compiler checks pass ship.cpp, ship_hull_contact.cpp and touch.cpp.
These checks do not accept installed visual destruction or player-save replay.

Shore impact HP and physical-material admission share
`AIShip.c::Ship_ContactMaterialFraction`; the existing sandbank exemption returns
zero, and Phonk returns its actual accepted HP fraction. Corrected shore motion
has its own active owner; its final contact normal/work seam must be verified
after integration. Saved finite hull cuts already notify `Sailors::OnHullCut`;
the same volume drives gore and retained-support loss drops sailors through the
existing jump/swim lifecycle. No second crew-casualty counter is introduced.
Secondary fracture now has generation/token-bound exact Trace, immutable physical
boundaries and atomic GEOM replacement. Closed thin-timber overlapping recuts,
failed-upload isolation, repeated recuts and sibling/parent exclusion pass the
component probes. The scene owner retains orphan MODEL/material lifetime and
queues cannon, contact and blast damage without replacing body vectors inside
the solver. Detached laser selection, moving endpoints and finite damage are
wired through the same token identity. Scaled-piece Trace/Beam/Crush and blast
surface selection pass their preserved actual-method counterexamples; the blast
field reaches material instead of the conservative bounding sphere. Canonical
build and real scene acceptance remain pending; this is source integration
evidence, not installed gameplay acceptance.

Retained secondary-component reproductions live under
`.local-archives/metal-renderer-evidence/ship-collision-20261016/`:
`secondary-geometry/generate.py` exercises actual GEOM capture/replace and its
`--baseline` mode replays the thin-timber counterexample;
`secondary-projectiles/probe.py` and `kinematics.py` replay the cannon consumer;
`secondary-scene/generate.py` extracts the scene methods against the native math
types and CPU geometry kernel. Their adjacent generated inputs, raw logs,
SHA-256 receipts and native compiler commands distinguish each frozen run from
the changing checkout. Scaled-piece and blast first failures are retained in
`secondary-scene/scale-blast-first-failure-*`. These are private component
fixtures; `runtime.md` owns canonical build and actual player-scene verdicts.

`secondary-scene/native-hook/prepare.py` extends the retained current-ABI CORE
driver with an actual moved orphan token. Set `CODEX_SCRATCH_DIR` to a task-owned
temporary directory and run `<impact|laser|blast> --run` serially. The fixture
creates a real separating cut for setup, removes the original SHIP through its
AI owner, checks continued motion and an outgoing-hit negative, then exercises
the projectile/blast bridges or actual manual laser input. Original character
HP/cuts, hull callbacks and intact emitter geometry must remain unchanged while
the old material token retires asynchronously. Exact inputs, native library
hashes, model geometry and outputs are retained under `native-hook/runs` and
`native-hook/assets`. The current native run commits the seed and creates a real
token, then fails before weapon admission when its individual live-ship deletion
leaves registered ShipLights entries. Exact binary UUID, faulting virtual dispatch,
baseline6e2 sources and ordinary script cleanup ordering are retained in
`native-hook/runs/impact-run-a61da8401bb235ed/crash-classification/receipt.json`.
Ordinary sinking keeps the native SHIP; full sea exit deletes ShipLights first.
The bounded fixture successor preserves the other ship's registered light, then
exposes the same incomplete individual-deletion lifetime in SEAFOAM; its exact
binary and consumer closure are retained in
`native-hook/runs/impact-run-d453c214a86630d4/crash-classification/receipt.json`.
Sailors also retains the native SHIP while men remain. This stronger immediate
orphan-deletion case stays unresolved; these failures do not justify product
destructor or registry changes.

The distinct `DEBRIS_NATIVE_LIFETIME=attached` case preserves the original SHIP,
AI and normal scene consumers, matching ordinary sinking's entity lifetime.
The current native seed creates and traces token2. A logging-only replay binds
the later failed Position lookup to legitimate secondary contact damage: token2
moves at frames42–44, then becomes tokens3/4 at frame45 with the same model,
owner and GEOM generation, before any fixture weapon is armed. The parent stays
valid/mounted with unchanged HP/cuts/geometry, and the dry fragment's age0.08s
and fleet count4 exclude lifetime/cap cleanup. The late birth-frame solver can
queue damage before the fixture teleports the material; its camera then follows
a retired token. Exact native body rows, binary and source binding live in
`native-hook/runs/impact-attached-run-4f7bcd8a2743f1b3/classification.json`.
This accepts native motion and secondary contact replacement for that component.
The deterministic successor uses existing `ApplyPhonkLaserCutNow` only for its
separating prerequisite, selecting/moving the real piece in the same beforeFrame
call before any CORE/contact step. Actual weapon admission and asynchronous
secondary preparation/commit stay unchanged. All three native cases pass:
incoming impact retires token2 at frame59, manual deck-camera laser at58 and
blast at59, each armed at53 after native motion and the outgoing-hit negative.
Original parent HP/cuts/hull revision/callbacks and intact emitter HP/geometry
remain unchanged. Exact commands, inputs, native archive hashes, binaries and
rows are linked from `native-hook/attached-before-core-batch.json`; the serial
batch completes in28.725s with no product/build/app change. This accepts those
native weapon components, with `orphan=0` explicit in every result. It does not
exercise the full AIBalls ballistic callback chain, post-parent orphan lifetime
or accept player gameplay.

Ordinary balls hitting an intact SHIP have a separate remaining boundary:
`Cannon_Trace` sends admitted damage to `Blots::AcceptedHullHit`, which merges
finite holes/chips through `GEOM::SetHullCuts`. That path does not publish newly
detached whole-hull section bodies or call `UpdateCutStructuralFailure`; successful
projectile damage to an already detached token does not prove that cannon fire
alone can bisect an intact ship. The laser/ram cutter releases newly disconnected
surface components, keeping the largest supported component of each original
authored component. It does not solve stress in a surviving thin connection.
The separate ship-fatality heuristics are retained material<=35%, waterplane
area<=65% of baseline, or lost support at>=60% of original keel samples; these
do not create an arbitrary cross-ship separation plane.

The ghost report also reproduces in the native geometry draw owner: missing
empty overrides resurrect original visual objects after rough cuts, while the
retained collision mesh correctly excludes them. The four-line source repair
in `ship_laser_geometry.hpp` retains those empty object domains; actual
Flyingdutchman false original draws fall18→0. This explains a visible-but-absent
collision mechanism; complete native contact replay remains unresolved.

Canonical5e9af7f is installed, with exact protection/signature receipt in
`runtime.md`. The16:02/16:04 player replay still rejects pass-through and ghost
stern geometry on that engine. The source checks below do not accept contact;
the coherent native scene reproduction remains pending its serialized GPU slot.

The15:20:32 player replay rejects installed7433ec35 frame cost and exposed-wreck
pass-through. The source successor separates physical contact from the damaged
waterplane: native Clip supplies the current root XZ convex projection, cached
by GEOM identity/generation, collision-topology revision and Phonk cut revision.
Cannon-after-laser success invalidates it; failed uploads preserve the revision.
All58 installed root projections fit128 points (maximum51); actual cut cases
use19/30. Current transformed retained-bounds corners provide conservative
vertical band/exposure. The `y < -height/3` gate is removed: exposed wrecks
remain physical and repeat rams reach the shared fracture hook, while fully
submerged wrecks are excluded. Existing yaw-only convex XZ fidelity remains.
ASan/UBSan actual-adapter/frame probes pass repeated impacts, empty-waterplane
remnants, cache invalidation and submerged/airborne/no-contact negatives; native
syntax passes. Installed scene motion and complete frame cost remain unresolved.

The following installed predecessors are historical evidence.

The latest player frame reopens destruction acceptance again: hollow sections,
loose pieces crossing hulls, and a live remnant after most root material is gone.
The installed successor adds wood shell thickness, measured detached bodies,
native convex contact and severe cumulative-loss fatality in the shared owner
described in `sea-phonk.md`. Ordinary ramming and Phonk use that same fracture
path; Phonk retains its separate intact-contact HP allowance. Component checks
pass; `88b3ebe` also passes canonical build and installed-app delivery in frozen
`4ee15fd` (`runtime.md` owns the receipt). Player-motion and save/death acceptance
remain open; successful staging does not close the rejected scene quality.

The first expanded collision source is committed in `f4d3c58`; installation was
interrupted when the user clarified sustained crushing and catastrophic breakup.
That follow-up is source complete: sustained pressure, transient contact-work
accumulation, catastrophic midship fracture and contact-budget-safe flooding pass
their source/native component checks. Follow-up `1bc5325` passes canonical staging
with signed engine `4c14ef379c6a9ca248a6a0339222f830f0b33351b7ff2f5c23e4a7709e030467`;
the player subsequently rejects structural survival and floating crew/rigging
after ram/laser. A coherent structural-death/support repair is source verified
in the shared ship owner (`sea-phonk.md`) and canonically staged in `9288f64`
(signed engine498cc8dc...). Player collision/fracture replay remains pending.
Earlier component results do not close that reported failure.
The player then rejects a small NPC grinding against a passive heavy hull and
Phonk losing physical hull despite its contact budget. A source-verified
successor requires inward acceleration supplied by the heavier ship's native
step, applies accepted Phonk HP protection to material work, and gives mechanical
fractures one persisted jagged field. The coherent successor50daab4 passes
canonical staging with signed engined660336f... and fresh518-file protection;
player replay remains pending. The renderer cache crash is a separate unresolved
branch, not repaired by these changes. `runtime.md` owns the delivery receipt.
The previous wreck-only correction below is historical evidence, not acceptance
of ordinary ramming. Native integration remains with the shared build owner.

## Expanded contract and implementation boundary

All hulls use their geometry, mass, relative point velocity and contact normal;
there is no ship-name or class exception. A bow ram damages the rammer locally
as well as the struck hull; tangential scraping does not spend head-on impact
energy. Comparable hulls can push at low speed without repeatedly paying ram
damage. With more than a twofold mass mismatch, continuing drive can overload
the smaller hull and progressively damage both, with much less damage to the
heavier hull. The heavier hull's native step must increase XZ kinetic energy
and accelerate inward; passive drag against a small pusher supplies no crushing
drive. Measured normal impulse still caps the admitted force.
The elastic specific-energy allowance applies to impact work; sustained crushing
uses accumulated normal impulse divided by frame duration, not closing speed.
The script collision event returns accepted material fraction. For the player's
Phonk ship the native owner maps that protected damage back through the existing
inverse impact curve before applying physical cuts; scaling saturated energy
directly would still destroy an HP-protected hull. Ordinary ships return1.
Collision mass is bare `State.fWeight` plus canonical `Ship.Cargo.Load`, both
in cwt; the latter already includes goods, installed guns and crew, and is not
capped at nominal hold capacity. Existing cargo writers/invalidation and
`RecalculateCargoLoad` retain ownership of that published load.
Live hulls must block each other across a frame, including high-speed
crossings, initial overlap, containment and rotation. Separating contact corrects
overlap without fresh damage. Sinking hulls yield and can lose real geometry
through the same fracture/contour owner as other hull damage, without laser heat.
Shore grounding, ordinary sailing controls, existing damage protections and the
binary SAVE layout remain with their existing owners.

- `engine/src/libs/touch/src/ship_contact.hpp`: swept convex-hull queries,
  point-effective-mass impulses, signed torque, friction and dissipated energy.
- `ship_contact_frame.hpp`: transient earliest-contact integration of all hulls,
  mass-weighted overlap correction and one accumulated impact per pair/frame.
  That row also carries the summed normal impulse for continuous contact load.
  Rotating separating-plane bounds release sliding contacts without false
  stalls; a spent safety budget blocks only the unresolved contact component.
- `touch.cpp`: one pair owner, native world/local unit conversion, bounded
  contact solving, nonpenetration and one damage publication per pair/body.
- `ship/src/ship_hull_contact.cpp`: applies solved world velocity, torque and
  endpoint through native `State`/`TouchMove`, including Phonk world drift;
  no second persistent motion state or collision-strength slot is introduced.
- `SHIP_BASE::ApplyHullImpact`: shared physical damage seam; the ship/geometry
  owner retains mesh cuts, fragments, contour invalidation, flooding and saved
  damage. TOUCH must not copy that state or add a parallel fracture path.
- Mechanical contact work accumulates only for the same nearby contact and
  expires after one second without it. That accumulator is transient; resulting
  cuts/fragments use the existing saved damage owner. Catastrophic lateral hits
  near midship cut across the keel rather than applying a bow-only exception.
  Actual Flyingdutchman center/+/-0.15-length cuts detach42.04/43.01/28.26 percent
  of authored surface, with valid seams and retained/removed-side BVH checks;
  these are real-mesh component results, not rendered gameplay acceptance.
- Native State/TouchMove remain the movement and prediction owners. Existing
  script collision events retain HP, perks, crime and kill attribution.
- Mechanical fractures use a bounded diagonal piecewise-linear grain profile
  in the existing split/cap owner; laser cuts remain planar. Geometry, retained
  crew/rig support, waterplane contour, keel and save/rebind consume the same
  signed field. Invalid topology, multiple waterplane components or a budget
  failure preserve the previous mesh instead of substituting a planar cut.

The source-bound baseline reproduces three detector failures: two30m hulls at
28m center separation overlap but fail broadphase; perpendicular8×30m hulls
cross without an admitted contact; intersecting horizontal/vertical segments
return the sentinel(10000,10000) rather than(0,0). The old recursive path also
publishes damage per traversal instead of solving one equal/opposite pair.
The decisive checks are the actual TOUCH adapter plus swept/overlap cases,
linear/angular momentum and nonincreasing contact energy, both mass orders,
bow/side damage, three-ship contact, separating/scraping negatives and a prior
save through normal loading. Full installed ramming/fracture replay stays open.

The successor's actual adapter/frame-solver ASan/UBSan falsifier reproduces120
specific crush work for either driver role in the prior code. A small driver
against a passive heavy hull now produces0 HP events/0 material work over10s
at16/50/100ms while moving the heavy hull0.9105/0.9136/0.9182m. Real heavy drive
retains120 work; braking, equal/twofold mass, separation/scraping/no-contact
negatives pass. Native motion preserves endpoint within1.6e-6m and yaw within
6e-9rad; no second motion fix is added. An unprotected1000-knot ram has32869.6
work; accepted scale0.05 maps to5.40977 material work and zero scale makes no
cut. Actual VM contact-budget, reload and structural-death cases pass; native
syntax passes. These are source/component checks, not the player's battle.

Final source-bound ASan/UBSan checks use the actual TOUCH and ship-contact
method bodies with synthetic hull/native-motion adapters. A 20-knot equal-bare-
mass ram gives the target 1112.762/10000 raw HP damage; adding 3000 cwt of cargo
to the rammer raises that to 2448.936 and lowers its own damage from 895.670 to
139.434. Halving speed lowers target damage to 286.396. Two hundred frames of
gentle pushing accelerate the target from 5 to 15.004 knots with zero HP events.
Crossing, three-ship contact, initial overlap, wreck displacement/fracture
callbacks and unaffected no-contact motion pass. The real 8-by-30m Phonk envelope
(1000 knots, 0.7 rad/s) passes fast crossing and 120 frames of sliding contact
at 16/50/100ms with no exhausted frames; an unrelated third hull stays untouched.
Pair-level checks cover momentum, nonincreasing kinetic energy and energy-loss
accounting. Native compiler syntax checks pass. These do not prove full native
movement, mesh breakup in a rendered battle, prior-save loading or installation.

The follow-up source-bound falsifier reproduces zero damage while a fourfold-
heavier hull drives continuously into a lighter one. The corrected actual TOUCH
publisher, with real frame contact solving, gives 1667.04–1671.12/10000 raw HP
damage to the lighter hull over ten seconds at 16/50/100ms; the heavy hull pays
59.54–59.68. Equal and twofold masses retain zero pushing damage. With a
1000:1 ratio the heavy hull pays only 0.00266 raw HP. Opposing equal hulls at
5/6 knots each retain 226.415/330.664 raw HP damage; at 500 knots each the
raw collision is catastrophic (9960.272). Native script protection still owns
the player's Phonk HP budget. These material thresholds are game balance, not
a timber constitutive model: overload begins above 0.015 m/s² of mean normal
load per lighter-hull mass, contrast ramps continuously above 2:1, work rate is
bounded and reciprocal damage follows squared inverse mass and bow/side strength.
A single 500-knot broadside ram against a stationary equal-mass hull gives
9842.953/9909.650 raw HP damage and 8226.355 specific work per hull. A 1000:1
mass contact reduces the heavier ship's forward speed by only 0.107892%.
The final TOUCH syntax check and repeated supported Phonk CCD/sliding negative
pass after the normal-impulse field addition; no movement equation changed.

The bounded upstream search found the same heuristic in the maintained
[PiratesAhoy TOUCH](https://github.com/PiratesAhoy/storm-engine/blob/main/src/libs/touch/src/touch.cpp).
[Box2D3.1.1 collision queries](https://github.com/erincatto/box2d/blob/v3.1.1/include/box2d/collision.h)
are the strongest reusable alternative, but their8-vertex proxy cap requires
lossy simplification/decomposition of the current128-point hulls. Adapt the
[standard point-impulse equations](https://github.com/erincatto/box2d-lite/blob/master/src/Arbiter.cpp)
inside TOUCH instead of introducing a second motion world; a detector that still
misses swept/contained hulls rejects that choice regardless of the smaller diff.

## Historical wreck-only correction

October 12, 2026: original repeated-contact defect reproduced; final safe-contact,
mass/persistence probes and native CMake compiler syntax checks pass. Canonical
`run.sh --stage-only` succeeded after the final ordered integration; the signed
player app contains the correction. Original battle and real save-file replay
remain unresolved; no game was launched.

Installed engine SHA-256:
`1763605379170486f369e1cc36fbf80fffcccb08d37e3c3fd8ef9db0f5820b18`.
The consumed wreck patch is
`bf6f673f65b8914700058d405790db667ce61b9b0cac21dfaf55d4fbf0336ccd`.
The installed binary exports `SHIP::ApplyWreckContact`; all 249 snapshotted
player SAVE/userdata/config files were preserved by the delivery owner. The
canonical receipt and current combined batch belong to `docs/runtime.md`.

## Historical contract

Ramming an already dead, still visible hull causes mild contact resistance,
pushes a lighter wreck aside and accelerates its sinking without a recursive
damage/rotation cascade. Ordinary live-ship ramming and shore contact retain
their existing behavior.

## Owners

- `experiments/native-metal/wreck-collision.patch` owns the native correction.
- `src/libs/touch/src/touch.cpp`: pair admission, contact power, reciprocal
  impulses and overlap separation.
- `src/libs/ship/src/ship.cpp`: collision prediction and the existing saved
  `vCurDeadDir` sinking displacement; `ship_base.h` exposes contact absorption.
- `PROGRAM/sea_ai/AIShip.c`, `Ship_Ship2ShipCollision`: existing hull damage
  is `(1 - abs(slide)) * abs(power) * 10`; this script is unchanged.
- Canonical integration/build/staging stays with `run.sh --stage-only` and
  the batch integration owner. The installed app is the accepting runtime.

## Reproduction and mechanism

The source-bound disposable probe uses actual TOUCH geometry, recursive contact,
impulse, separation and SHIP prediction/strength methods. Its explicit adapters
provide rectangular hulls, a flat sea, fixed mass inertia/braking and the real
dead Execute pose rollback. These are component results, not a player replay.

An 8 x 12 x 30 hull at 12 knots, equal weights, wreck offset x=6 and yaw=1.57,
with a previously queued 3-knot/0.08-radian impulse, produces 33 player collision
events, 15 with negative power, and 1774.61 raw hull damage. The same live pair
produces 3 events, 143.142 raw damage and no negative events. In a second
30-knot oblique case, dead contact reaches 1.29712 radians/second player yaw.

`SHIP::TouchMove` predicts movement from the dead ship's retained velocity and
queued strengths. Dead `Execute` subsequently restores `vOldPos/vOldAng`,
discarding that movement and FakeTouch's separation while retaining velocity.
The next collision therefore sees a moving obstacle that did not move.
Separating overlaps also return negative power; the script's absolute value
turns it into fresh damage. Vertical sinking velocity is not the cause:
`BuildContour` transforms only X/Z.

## Correction and preserved state

Dead prediction returns the actual anchored pose and zero velocity/rotation.
Dead ships do not initiate or reciprocate contact impulses. A live hull contacting
a wreck projects contour movement onto the contact normal oriented toward the
wreck, then keeps only positive closing power; its recoil/damage power is scaled by
0.15, the wreck/live mass ratio capped at one, elapsed seconds capped at 0.1,
and the exposed fraction `clamp(1 + 3 * wreckY / hullHeight)`.
The remaining absorbed contact travel advances existing `vCurDeadDir.y`.
Thus stronger/longer contact sinks the wreck further, and contact fades as it
submerges. No new saved fields, extra death events or script flags are added.

FakeTouch splits horizontal separation by the existing continuous mass
coefficient: a live hull ten times heavier moves 5% of the separation and the
wreck moves 95%; equal masses split it equally. `ApplyWreckContact` retains
the wreck's shift in `vOldPos`, current `State.vPos` and output `vPos`, so its
next death-pose update cannot undo it. Save already stores State/vPos and
reconstructs the anchor through SetDead on loading. The finite solver budget,
live/live and island branches remain.

The existing `SetPos` cannot own this shift: it resets wave height/world offsets
and does not retain the dead pose anchor. Contact absorption and retained
translation therefore share the native displacement method, without new state.

The patch follows `sea-whirlpool.patch` in the canonical stack. Its TouchMove
insertion stops at `old_state`, leaving the subsequent current/yaw expressions
with their owning field. Exact zero-fuzz composition against the vortex-patched
source passes; the native correction is unchanged by this context adjustment.

## Acceptance and rejected alternatives

Final source-bound checks reduce the broadside case from 1774.61 to 36.7633 raw
damage and from 0.309979 to 0.00767824 rad/s maximum yaw. The oblique case drops
from 917.045 to 15.681 damage and from 1.29712 to 0.0371396 rad/s yaw. Both live
pair outputs remain byte-identical. A departing overlap emits zero events and
adds zero sinking displacement. Previously queued wreck impulses of 0, 3 and
300 knots produce identical repaired contact results.

At wreck depths 0/-2, contact response and extra sinking scale 1/0.5; below -4
for this 12-unit-tall hull there are no events, queued impulses or extra sinking.
For 16/33/100 ms frames, one contact adds 0.0840033/0.173256/0.525016 sink units;
the following extracted dead-pose step also adds ordinary 0.35 units/s sinking.
Thirty-second contact fixtures bound damage at 29.0247/31.3281/36.7633 rather
than growing inversely with frame time. Zero/negative/nonfinite sink inputs and
live ships are rejected; dead prediction preserves existing State/Strength bytes.
The real ship.cpp and touch.cpp pass `-fsyntax-only` with their exact native
CMake compilation flags and one complete disposable source snapshot.

The final shove fixture at live:wreck masses 10000:1000 displaces the live hull
0.301055 m and the wreck 5.71922 m. Equal 1000:1000 masses move 3.01001/3.01002 m;
the reversed mass ratio moves 5.71897/0.300785 m. Anchor/current/output XZ agree
after the next extracted death-pose step, and pure horizontal contact preserves
the preexisting sink vector. A seeded prior anchor (10,-1,20) receives contact
(0.4,0.5,-0.6), retains anchor (10.4,-1,19.4), and advances sink 0.7 to 1.2.
Save/Load ownership is source-verified; a real save-file/player replay is still
unverified.

The initial norm-only guard was rejected by a true departing-overlap probe:
with a stationary wreck, speed magnitude remains positive while the player
moves away. Oriented normal projection closes that case.

- Reproduce the original severe and departing-overlap fixtures, then run the
  final actual methods with seeded old impulses and the live/live negative.
- A heavier live ship must displace a lighter wreck more than itself; the
  shift must survive the next dead-pose update and existing save/load ownership.
- Positive contact must add sink displacement; departing/stationary contact
  must add no wreck damage or sinking impulse. Already submerged hulls stay
  excluded. Contact must remain bounded across ordinary frame intervals.
- Build and stage the exact ordered patch; verify installed engine identity.
- Player replay: ram a newly sinking enemy obliquely, retain steering and hull,
  observe faster submergence, then ram/pass a live ship and a shore as appropriate.
- Must not remove all dead collision immediately, make wrecks transparent,
  rebalance ordinary ramming, change SAVE format, or claim scene acceptance from
  the disposable probe.

Allowing dead hulls to keep all predicted sailing/rotation was rejected: it
changes their existing anchored death animation and its save/pose contract.
Suppressing script damage alone leaves the phantom recoil and position rollback.

The bounded upstream search checked the
[current TOUCH source](https://github.com/storm-devs/storm-engine/blob/develop/src/libs/touch/src/touch.cpp),
[releases](https://github.com/storm-devs/storm-engine/releases) and indexed
collision issue/PR results. No applicable released correction was identified;
the native baseline remains `4860fe13245b`. This is a local contract repair.
