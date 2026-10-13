# Local sea whirlpool

## Status

October 12, 2026: native source candidate. Disposable ASan/UBSan field checks
pass. The revised field passes an actual Metal water/foam component readback,
with the same synthetic textures and ordinary-wave approximation as its smooth
predecessor. Canonical build/staging and installed Great Recipe scene replay
remain integration-owned checks.
Source or component checks do not accept the cinematic in the player save.

## Contract and owners

The Great Recipe sea scene uses a fixed world-space depression, rotating water
and broken spiral foam. The hull follows that same displaced water, drifts inward
and around its centre, can steer out, and loses hull in proportion to actual
exposure. A ring of splash particles is not a whirlpool.

`experiments/native-metal/sea-whirlpool.patch` owns native transient state and the
shared field in `shared/sea_whirlpool.hpp`, SEA deformation/commands/debris and
SHIP current/pose. `sea_shaders.hpp` owns advected normal detail and foam using
the existing water/reflection/depth/aim paths. Great Recipe script owns phase,
persisted centre/elapsed time, hull damage application and save restoration.
`build.sh` remains the ordered patch and shared-header delivery owner.

The existing tsunami geometry, scalar/SSE queries, temporary current inside
Move/TouchMove, ordinary buoyancy, authored PENA foam and `seatrash` rect draw
are reused. [Crest's maintained whirlpool input](https://docs.crest.waveharmonic.com/Components/Inputs/Whirlpool.html)
is the strongest ready reference: separate displacement and bounded advecting
flow. Its Unity runtime cannot replace the Storm sea/SHIP/Metal owners without
an engine migration; adapt that separation inside the existing native field.

## Bridge and lifecycle

Write `Sea.Whirlpool.CenterX`, `CenterZ`, `Radius`, `Intensity`, `Duration`,
`ResumeAge`, then `Start=1`. Radius defaults to90, intensity to1; finite native
bounds are radius30–240, intensity0–1 and duration6–90 seconds. Repeating the
same live configuration is idempotent. A changed descriptor replaces the field.
`Start=0` starts a three-second decay; `Clear=1` clears immediately. Commands are
consumed, so saved Sea attributes cannot silently restart an event.

Native execute time owns age, five-second onset and the final three-second fade;
pause freezes age. `ResumeAge` seeds a new native activation from saved quest
elapsed time. Active/Age/Envelope are readbacks. Init, teardown and render reset
discard native state; an eligible restored quest explicitly reissues its descriptor.

`MSG_SEA_WHIRLPOOL_CONTACT`50204 uses `"lae"`: character attributes and a float
script-variable result. It returns this activation's cumulative fraction of maximum
hull, at most0.5. The script applies only positive query deltas, preserves existing
damage and guards, and caps saved total across native resets at0.5. The native
clock/sample integrates contact; no script impulse or independent radius damage
may duplicate it. Full core exposure needs20 integrated seconds to reach the cap;
onset, rim dwell and escape reduce damage. An ordinary collision remains owned
by the existing collision/damage chain.

## Mechanism and verification

Normalized radius drives a compact smooth bowl, raised broken rim and rotating
three-arm ridges. The initially uniform lip and shallow ridges produced a smooth
dish in the component GPU view, so the shared physical field now has a sheared
primary fold, two secondary wave scales and a lip varying in radius and height.
The short wave is about15m long at radius90m; it is actual geometry rather than
fine normal-map or particle detail. Analytical slopes drive buoyancy and rendered normals. Height,
flow and foam have zero boundary support. Radius scales physical dimensions
continuously; intensity and the time envelope scale all consumers together.
Translation/yaw are temporary currents, never stored in propulsion/save state.
Surface debris reuses the existing authored rect texture with a48-item event
pool advected by the physical current; its draw uses a fixed stack buffer.
Ordinary underwater trash has its separate existing lifecycle. Existing adaptive
sea blocks refine only inside the active footprint, with spacing proportional
to radius; the original vertex/index buffer limits remain authoritative.

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
SEA/SHIP compilation, prediction/grounding and GPU/game checks are still needed.

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
