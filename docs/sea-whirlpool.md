# Local sea whirlpool

## Status

October13 corrective candidate: the player's missing-water scene reopens mesh
and camera acceptance. The complete native patch now fixes oversized block
admission, actual-ratio LOD seams and false underwater classification, and adds
a finite real-model entry using the existing bow spray. Source-body CPU and
canonical native compilation and installed staging pass. The next player replay
shows covered sea but rejects low camera and steering. The successor corrects
the stock camera's absolute-height dimensions using the shared physical field,
and bounds inward/swirl/yaw against actual ship propulsion and inertia. The full
canonical native build/staging passes. Radius, depth and geometry are preserved; actual
camera, control and battle replay remain unresolved. Earlier GPU field receipts
do not accept these ship/camera interactions.

October 12, 2026: the earlier90m/24s field was installed and passed native/Metal
component checks, but the player rejects the solo, small, short scene. The
sustained fleet-theater revision is delivered on October13 in batch `8bacecb`:
ASan/UBSan field/control checks pass at90/700/1600m, the complete seven-section
patch is consumed by the canonical build/stage, and the R900/duration600 actual
water/foam GPU component passes motion and Clear identity. Diagnostic textures
and approximate waves do not accept the whole-fleet battle; real-scene appearance
and combat remain player-owned replays. Source or component checks do not accept the
battle in the player save. The prior local-field receipts below retain their
original scope.

## Contract and owners

The Great Recipe sea scene uses a fixed world-space depression, rotating water
and broken spiral foam. The hull follows that same displaced water, drifts inward
and around its centre, can steer out, and loses hull in proportion to actual
exposure. The support must contain the fleet's live circular battle and remain
active through combat. A ring of splash particles is not a whirlpool.

`experiments/native-metal/sea-whirlpool.patch` owns native transient state and the
shared field in `shared/sea_whirlpool.hpp`, SEA deformation/commands/debris and
SHIP current/pose. `sea_shaders.hpp` owns advected normal detail and foam using
the existing water/reflection/depth/aim paths. Great Recipe script owns phase,
persisted centre/elapsed time, hull damage application and save restoration.
`build.sh` remains the ordered patch and shared-header delivery owner.

`seafoam.cpp/h` are additional native consumers in the same patch. Vortex
contact and the entry suppress their repeated `ship_bow` impacts; ordinary
storm spray and real cannon/ram sounds keep their existing owners. The quest's
water-exposure hull helper produces no impact sound and is not disabled.

The first quest scene admits the existing island-free sea mode. A50.4m bowl
over a shallow authored island seabed can reveal terrain and ground vessels;
fixing sea indices cannot create room below that terrain. Ordinary island
scenes and terrain are unchanged.

## Ship entry and mesh correction

The existing SHIP model and rigging have one root transform. Additive
`Ship.SeaEmergence.Generation/Age/Duration/Active` attributes drive one6.35s
trajectory: fully submerged rigging, a two-second continuous launch to the
waterline at14m/s, a gravity9.81 arc with a9.99m apex, then heavy continuous
settling into the same buoyancy/current. Pitch/roll are bounded18/10 degrees.
The existing third bow emitter uses authored `seafoam_front`, scales at most3x
near breach/return and follows the local displaced waterline; no shader,
particle pool or new asset is introduced.

Native time owns age; zero delta pauses it. Restored age does not restart the
entry. Clear/replacement cancels into a1.5s settlement; expired/dead entries
stop safely. A transient cache keeps GetPos/GetAng, script pose publication,
root matrix, cannon position and TouchMove prediction consistent while leaving
raw buoyancy and binary SAVE fields unchanged. The old query bodies fail the
matrix-agreement counterprobe. Inactive/final ordinary pose identity passes.

The earlier `iMaxLOD=iMinLOD` prematurely stopped subdivision and could produce
a block larger than the entire65,500-vertex budget. Normal subdivision plus the
bounded event ceiling restores near-camera admission. Seam indexing now uses
the actual power-of-two LOD ratio. Actual extracted mesh bodies pass36 cases
at grid.07/.12/.17 and radii90/700/1600 over four views: zero dropped blocks,
zero invalid references or inverted lattice faces, and drawn area equal to
admitted area. Existing duplicate-index seam degenerates are preserved.
Inactive/Clear topology is identical. No buffer enlargement or smaller funnel
is used. Native camera already clamps to WaveXZ; the separate negative-world-Y
underwater shortcut is disabled during the displaced field.

The camera's box dimensions previously included absolute modelY, producing a
negative height in the deep field. The successor subtracts the shared physical
field height and translates the target cap by the same reference. Actual Move
and native matrix bodies preserve243 ordinary outputs bitwise and pass729
translated outputs within0.1mm; every original translated output fails. Another
972 curved-field/zoom/pitch/yaw/entry-phase outputs remain finite and above local
water using the actual installed manowar hull/rig bounds. Collision/ground adapters
are bounded fixture stubs; complete player framing is still an actual-scene gate.

The old maximum inward3.349m/s exceeded the weakest healthy fleet's2.767m/s
quarter-speed floor; tangential12.36m/s dominated actual3.722m/s propulsion.
The shared successor bounds those peaks to2.061/3.606m/s and intrinsic yaw
to.025rad/s, retaining the boss's55% swirl without inward/yaw load. Actual stock
speed/strength/drift/yaw/movement bodies pass120 hull/wind/heading cases with
decoded inertia and exact1853/3600 knot conversion. The historical weak-wind
flagship travels radius634→2104m over600s versus634→659m before; mastless zero
propulsion still drifts123m inward over60s. Opposed rudder control remains available
for every tested hull. Inactive movement preserves five hull states bitwise.
These component trajectories use bounded rudder commands and do not accept AI,
full marine passage or the visual relief of the player's battle.

The existing tsunami geometry, scalar/SSE queries, temporary current inside
Move/TouchMove, ordinary buoyancy, authored PENA foam and `seatrash` rect draw
are reused. [Crest's maintained whirlpool input](https://docs.crest.waveharmonic.com/Components/Inputs/Whirlpool.html)
is the strongest ready reference: separate displacement and bounded advecting
flow. Its Unity runtime cannot replace the Storm sea/SHIP/Metal owners without
an engine migration; adapt that separation inside the existing native field.

## Bridge and lifecycle

Write `Sea.Whirlpool.CenterX`, `CenterZ`, `Radius`, `Intensity`, `Duration`,
`ResumeAge`, then `Start=1`. Radius defaults to90, intensity to1; finite native
bounds are radius30–1600, intensity0–1 and duration6–900 seconds. The fleet
scene chooses700–1600m from actual spawn extent and600s duration. Repeating the
same live configuration is idempotent. A changed descriptor replaces the field.
`Start=0` starts a smooth decay; `Clear=1` clears immediately. Commands are
consumed, so saved Sea attributes cannot silently restart an event.

Native execute time owns age. Radius smoothly scales onset/fade from5/3s below
240m to20/10s at1000m and above;700m uses14.83/7.59s. Pause freezes age.
`ResumeAge` seeds a new native activation from saved quest
elapsed time. Active/Age/Envelope are readbacks. Init, teardown and render reset
discard native state; an eligible restored quest explicitly reissues its descriptor.

`MSG_SEA_WHIRLPOOL_CONTACT`50204 uses `"lae"`: character attributes and a float
script-variable result. It returns this activation's cumulative fraction of maximum
hull, at most0.5, with an independent record per ship. The script queries every
companion, applies only positive query deltas, preserves existing damage and
guards, and caps each character's saved total across native resets at0.5. The native
clock/sample integrates contact; no script impulse or independent radius damage
may duplicate it. Full core exposure needs20 integrated seconds to reach the cap;
onset, rim dwell and escape reduce damage. An ordinary collision remains owned
by the existing collision/damage chain.

`character.Ship.WhirlpoolControl` is a generic finite0–1 handling coefficient,
default0. Full control removes inward trapping, additional yaw/tilt and native
exposure, while retaining55% of the tangential water current. The boss rides the
swirl and uses ordinary steering/propulsion; there is no native identity rule,
teleport or hull-height override. Intermediate values blend continuously.
Every ship, including full control, samples the same displaced water and ordinary
buoyancy. Dead/fixed and storm-protection guards retain their existing owners.
Spawning on the inner face and navigating outward rises along the funnel; the
quest/combat owner controls the entrance and actual attack.

## Mechanism and verification

Normalized radius drives a compact smooth bowl, raised broken rim and rotating
three-arm ridges. The initially uniform lip and shallow ridges produced a smooth
dish in the component GPU view, so the shared physical field now has a sheared
primary fold, two secondary wave scales and a lip varying in radius and height.
The short wave is about15m long at radius90m; it is actual geometry rather than
fine normal-map or particle detail. Analytical slopes drive buoyancy and rendered normals. Height,
flow and foam have zero boundary support. Radius scales physical dimensions
continuously; intensity and the time envelope scale all consumers together.
Depth, ridge amplitudes, current speeds and debris size use physical length
`min(radius,180m)`: a1600m battle theater is not a448m deep hole with110m/s flow.
Maximum full-strength centre depression is50.4m; sampled successor total current
speed is about4.16m/s. Pattern/normal/foam clocks use the same physical-length/radius ratio.
Translation/yaw are temporary currents, never stored in propulsion/save state.
Surface debris reuses the existing authored rect texture with a48-item event
pool advected by the physical current; its draw uses a fixed stack buffer.
Ordinary underwater trash has its separate existing lifecycle. Existing adaptive
sea blocks refine only inside the active footprint, with spacing proportional
to radius; original65,500-vertex/165,000-triangle limits and block admission
remain authoritative. There is no new fullscreen mesh or buffer expansion.
Ordinary near-camera blocks keep their finer spacing. Actual fleet-scene timing
remains required before a performance claim.

Disposable header probes compare analytical slopes with independently sampled
height differences after the fold correction: maximum error0.0003366 with3cm
sample separation (against a minimum15m secondary wavelength). The centre at `(14567,-27890)` is
25.2 metres lower at full90m/intensity1 strength. At24 seconds, passive starting
radii0/35/65/85/91m lose50.0/44.0/22.1/0.775/0 percent of maximum hull.
An outward5m/s trajectory starting35m from the centre loses5.85 percent and
escapes. These are point-current/header trajectories; they do not include a real
ship's controls, footprint, inertia, collisions or shore depth.
Same-age contact queries, nonzero origin translation, identical start, resume at
12s, three-second stop, Clear and invalid-coordinate rejection pass. Actual
SEA/SHIP compilation passes; actual prediction/grounding and game checks remain
player-owned.

The earlier sustained-field ASan/UBSan pass covers finite-difference slopes at700/1600m
(maximum errors0.0001095/0.0000454), bounded50.4m height/12.81m/s current,
600s descriptors,900s expiry, resume/pause, per-character cumulative cap and
full-control zero radial velocity with55% preserved tangential current.
Controlled and ordinary samples have exactly equal height/coarse slopes.
Those receipts describe the original stronger flow. The camera/control successor
has its own canonical compilation and body probes above; real game checks remain.

At age12s/radius90m, angular height RMS at normalized radius0.50/0.65/0.78
increases from0.716/0.933/0.741m to2.646/3.124/2.840m. The outer lip ranges
from−5.11 to+5.48m rather than the previous−0.08 to+2.02m. Current, yaw,
contact load and all exposure/escape numbers remain identical; buoyancy and
coarse normals necessarily change with the revised water. The paired GPU view
uses a fixed, explicitly approximate ordinary-wave baseline for both fields,
with synthetic detail textures and environment cube rather than installed scene
assets. The oblique surface shows separated folds and a lobed lip; active output
changes21,901pixels, age evolution changes21,240, and Clear restores
pixel-identical ordinary water. This accepts component anatomy, not the complete
player scene.

Cheapest decisive source/component checks are finite-difference slopes at a
nonzero world centre; scalar/SSE/geomorph equality; core/rim/outside damage and
escape trajectories; same-age queries, resume, identical start, pause, decay and
Clear. Native GPU readback must show an actual displaced funnel, advected water
normals and broken foam at low/oblique views using installed sky/normal/foam
assets. Inactive and cleared water must match ordinary sea, with tsunami-only,
rain, reflection and aim/depth identities intact. Ship prediction must match
actual current while fixed/dead/grounded vessels keep their existing guards.

Installed acceptance still requires the quest's onset, trapped-hull/escape,
assault transition, save/load and teardown replay. A frame dump without live
ship/shore/camera interactions cannot close those scenarios.
