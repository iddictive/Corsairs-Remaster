# Contact with sinking ships

## Status

October 12, 2026: original repeated-contact defect reproduced; final safe-contact,
mass/persistence probes and native CMake compiler syntax checks pass. Canonical build, installed
delivery and player battle replay remain pending with the batch integration owner.

## Contract

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
