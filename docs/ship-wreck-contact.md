# Ship collision and contact with sinking ships

## Status

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
