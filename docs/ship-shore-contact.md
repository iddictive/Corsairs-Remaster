# Ship contact with islands

## Status

The October16,23:13 player report again rejects recovery at rock walls. The
source successor recovers a real nearby exit face for an initially embedded
keel using the existing camomile winding criterion and traced MODEL triangle
normals. TOUCH uses it before raster-gradient fallback; SHIP uses it before a
horizontal support face can discard all XZ/yaw motion. The query is shared in
`touch/src/ship_shore_contact.hpp`, bounded to eight rays and local to one
constraint call. Rays discover faces; they never become radial normals.
Slope support retains its actual contact point. Existing velocity/current/torque
projection and arc/nonpenetration certification remain. Unknown geometry still
fails conservatively; source correction does not prove universal recovery or the
reported scene. No gameplay test or control is performed.

The preceding October16,21:40EDT graph/contact successor was installed; `runtime.md` owns its
delivery receipt. It retains common support from the20:52 predecessor.
Ordinary/NPC recovery under sail, gravity/friction on a slope and ordinary tsunami
beaching remain unaccepted in gameplay. No game launch or gameplay tests occur.

The later21:03 player replay rejects vertical-rock recovery in that installed
batch. Source discovery finds a second veto in SHIP::Move that restores the
entire XZ/yaw step and zeros velocity before TOUCH can resolve the wall; the
exhausted TOUCH loop also resets the complete pose. The source successor exposes
the desired prediction sweep and constrains inward point velocity against actual
wall triangles while retaining tangent/separating motion. Temporary current and
event ledgers receive the same linear constraint before their normal removal.
The final fallback retains a certified arc prefix and separately certifies yaw,
rather than cancelling every motion axis. This candidate is installed but
unaccepted in gameplay. It is query-free in ordinary deep water; an already
failed ground step still uses bounded sampled keel arcs and support-prefix
queries, whose worst-case cost is not established as an FPS improvement.

## Active common-support contract

Predecessor11fa90d baseline: LandSupport pins dry Y only; no intact-ship downhill gravity exists.
Worsening depth rejects both horizontal axes, and ordinary wave rocking can lower
a tsunami-beached hull through support. The independent Strand timer charges raw
forward speed every100–200ms even while a hull is leaving tangentially.

Requested outcome: every living ordinary/NPC/Phonk hull can use admissible sail or
motor thrust to slide free; a sufficiently steep loaded support produces downhill
motion under gravity/friction. Receding water hands a tsunami-beached hull to
terrain support, then back to buoyancy when it slides clear. A flat beach or a
slope held by real static friction need not generate motion without enough force.

The shared SHIP owner reuses retained keel samples, actual island triangles,
existing propulsion/inertia, collision mass and persisted State velocity. For
support elevation H, floating origin Yfloat and draft D, the declared prism
approximation is load fraction clamp((H-Yfloat)/D,0,1). This is not an exact
displaced-volume calculation. The upward support normal owns tangent gravity and
load-bounded Coulomb friction, reusing the existing shore coefficient. Dry
bLandContact must remain distinct from partially afloat seabed support.

The ordinary temporary current ledger and Phonk world velocity must retain the
same solved motion without double application or a reversed residual. Prediction
restores every changed derived field; pause adds no impulse. Deep free water
retains its old arithmetic and avoids new terrain queries. Fixed/dead motion,
retained-geometry ownership and direct inward-impact damage retain their owners.
The redundant raw-speed Strand damage timer is removed; grounding state/sound
remain. No radial ejection, forced yaw, new reverse engine, automatic save repair
or initial-overlap upward teleport is admitted.

Primary accepting scenario is an ordinary partly grounded hull under diagonal
sail thrust, plus a tsunami carried onto sloped ground and left as water recedes.
Reject reversed current, idle level-ground energy, fixed/dead changes, tunnelling,
double prediction impulses and water motion changes. Source/diff inspection is
the permitted current evidence; numerical/gameplay tests are explicitly declined.
The player owns eventual replay. Root owns script damage, docs, Git and delivery;
the completed shore source is integrated by root. A full rigid-body engine is unnecessary for
this declared approximation; horizontal projection alone omits gravity, load and
vertical handoff and cannot meet the requested outcome.

The candidate retains five keel samples and uses the winning actual terrain
triangle's upward normal. `State.vSpeed` owns velocity; its X/Z remain knots and
Y remains m/s, without a SAVE-layout addition. Support/falling/water-ledger flags
are transient and prediction restores them. The same tangent/friction transform
applies to temporary currents before their exact subtraction. Ordinary descending
wave/air motion crosses into support at H; dry support can fall back to buoyancy
without a second Phonk-only velocity owner. A worsening-ground check includes
actual dry terrain height and full yaw/position, beyond the wet depth raster.

Limits remain explicit: an initially embedded saved pose keeps its Y and permits
tangent thrust, but receives no new load/gravity energy until valid support is
reached. A save on the very first frame leaving a flat edge with exactly zero
vertical velocity can be indistinguishable from ordinary buoyancy; a later
negative vertical velocity restores falling. These states are not repaired by
teleporting the hull or adding a hidden half-step. Five keel points and prism
buoyancy remain an approximation, not a full6DOF displaced-volume solver.

## Contract

A ship hitting the coast loses speed, rebounds weakly and turns according to the
contact point and hull inertia. A stationary, separating or tangential hull must
not receive a fresh kick, forced pose rotation or repeated impact damage. Reverse
and steering away must remain available, including an initially overlapping save.
The coast consumes the current published collision mesh; a filled visual cut alone
does not prove nonpenetration.

## Owners and mechanism

- `engine/src/libs/touch/src/touch.cpp`: earliest sampled coast contact, surface
  normal, arc-aware contact time and bounded native-motion correction. Wet hulls
  retain their coast contour; dry hulls retain three actual-height samples.
- `ship_contact.hpp`: shared rigid impulse, with an explicit immovable second
  body for the shore. Existing live/live calls retain their arithmetic. Restitution
  is0.08 and friction0.30; point velocity, mass and polygon inertia determine the
  linear impulse and signed torque. Contact cannot inject kinetic energy.
- `COLLIDE`/the traced model: the triangle in world coordinates. The island's
  `GetCollideTriangle` stub is not a triangle owner. A depth-only shore contact
  uses central differences of canonical `GetGroundDepth`, with bounded expansion
  across raster-sized initial overlap; it never substitutes a hull-radial normal.
- `SHIP::TouchMove`/`ApplyHullContactMotion`: native prediction, velocity and
  endpoint application, including the existing Phonk motion adapter. Grounding,
  land support, saved state and ordinary controls remain with `SHIP::Move`.
  Supported land constrains Y without zeroing speed/yaw or returning before
  controls. The existing worsening-penetration rejection blocks inward travel;
  support is resampled at the accepted endpoint so reverse/turn-away can release
  a partly grounded hull. Player acceptance remains separate from installed
  delivery in `runtime.md`.
- `AIShip.c`'s shore collision event: HP/perks and accepted material fraction.
  Only inward point speed supplies `.5 * speed²` to the existing protected local
  fracture seam; tangential speed does not manufacture ram work.

Simultaneous coplanar contour hits share their resultant point. Contact time is
refined on the rotating vertex arc rather than its endpoint chord. Four response
iterations have a conservative stop for an unresolved remainder; there is no
1024-step position/yaw separation loop and no legacy radial strength queue.

Phonk previously suppressed ordinary sail input while grounding simultaneously
disabled its own motor. Grounding now constrains proposed motion without removing
Phonk thrust/yaw controls. The vertical water/flight adapter still rejects ground
contact, preserving terrain support. GroundPenetration ignores removed keel
points, using the same retained-material rule as LandSupport.

Ordinary player AI speed/rotation/avoidance controllers are bypassed, and zero
speed still admits yaw. SailDown reefs sails to zero; it never commands reverse.
The contract above means physically separating movement must remain admissible,
not that ordinary S supplies a new reverse engine. An irregular conservative
depth raster can give an expanded central-difference normal opposed to actual
retreat. For an initial wet depth-only overlap, TOUCH now admits strictly deeper
endpoint travel only when12 native-arc samples have finite nonincreasing ground
depth and every intervening sampled segment misses the exact mesh. Exact initial
mesh hits, other hull points, rendered island immersion and the independent
worsening-penetration guard retain their constraints. This is a bounded sampling
correction; it does not prove continuous nonpenetration or this screenshot's cause.

## Grounding state and damage ownership

Native `SHIP::CheckShip2Strand` emits actual grounding edges. In the new source
successor `AIShip.c::Ship_OnStrand` owns only the grounded attribute and looping
sound, stopping it directly on exit. The two `SHIP_TO_STRAND_NEXT` producers,
their registrations and `Ship_OnStrandNext` are removed. There is no recurring
HP charge for static support, sail egress or downhill travel. Genuine inward
coast impacts still reach `Ship_Ship2IslandCollision` and its existing HP/perk/
accepted-material path. Native structural/flooding hazards remain independent.
Active-player Phonk protection belongs to `sea-phonk.md`.

The rejected timer taxed `4*Ship.Speed.z/7` every100–200ms, not dissipated contact
work. It could kill a normally escaping hull. Its older duplicate-chain defect
had already been repaired in installed11fa90d by per-character generations:
entry0ms, exit50ms, re-entry70ms previously left both100/170ms callbacks charging
the current grounding episode indefinitely. Removing the unsupported periodic
damage makes that repaired scheduler unnecessary rather than moving its formula
to another consumer.

Normal loading clears posted events, and compiler serialization does not save
that queue. Old saved generation attributes may remain unused; no SAVE migration
or live-VM replacement path is introduced. The exact reported instant ordinary
sinking still lacks HP-versus-pose replay evidence; source timer removal must not
be presented as proof of that one scene's cause.

## Evidence and rejected approaches

Actual-method adapters reproduce the previous stationary5m overlap as a0.280rad
rotation and5.582m displacement in one frame. A parallel1kn scrape also generated
recoil, yaw and material work despite zero inward speed. A pure no-torque sliding
candidate was superseded by the user's explicit weakly elastic physical-impact
requirement. A1mm movement cutoff and chord-only rotation time were rejected after
slow creep and up to3.04mm angular penetration were measured.

The corrected source passes ASan/UBSan cases at16/50/100ms: idle/initial overlap,
reverse and parallel negatives,0.01kn inward motion, angular contact/turn-away,
rotated coast,1000kn thin-wall sweep and0/.05 material admission. A straight10kn
ram rebounds at0.8kn with zero yaw; mirrored oblique impacts produce opposite
signed yaw. Two seconds of repeated oblique drive produce0.1397/0.1425/0.1466rad
at those intervals without frame energy injection. Ten thousand deterministic
live/live impulse comparisons against4832b2e produce zero bitwise differences.
Native `touch.cpp` syntax uses the current Ninja compiler/defines/includes.

The CPU fixture extracts actual TOUCH, TouchMove and ApplyHullContactMotion bodies,
but its Move adapter omits native propulsion, damping, currents and Phonk drift.
These results do not accept those full native paths or actual island geometry.

## Remaining acceptance

The shared integration owner must consume this source through canonical staging.
On the intended player save, replay a straight and oblique coast impact, release
the controls, then reverse/steer away; repeat a gentle scrape and an existing
beached/Phonk state. Observe no fresh idle spin, no hull passage through the coast
and normal control recovery. Rendered coast geometry, full native motion and frame
cost remain unverified. Contour/height sampling is not a continuous solid-volume
collision guarantee for arbitrary thin geometry or extreme full-turn sweeps.
