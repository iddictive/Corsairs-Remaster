# Sea manual-aim envelope

## Status

Local source candidate based on `4e9a09ab425dc8266131788067080bea7c8ad2bd`.
Native Metal build, staging, performance measurement and in-game visual acceptance are pending.
Static checks and numerical probes do not establish visual acceptance.

## Contract

First-person deck camera controls the aim. One subtle, continuous world-space
ballistic envelope spans the valid guns of the selected broadside. It is weaker
near the guns and modestly more visible downrange. Its footprint follows the
actual moving water, hull, mast, fort or terrain receiver, with soft contact rims
and subtle friend/enemy/neutral/unknown relation colors. Opacity is evaluated from
the combined corridor's contour; individual gun fields, cells and triangles do
not receive separate rims or visible mesh edges. No individual
bright gun beams, center spine, distance ticks, bulky stock reticle or floating
horizontal ellipse are drawn. A tiny centered `+` shows the camera aiming ray.

The envelope is a sampled prediction of live dispersion, not a promise that every
random shell will strike one point. Real nearby surface hits use their true aim
point; nearby mechanically unreachable shots stay unavailable. Empty or genuinely
out-of-range space means the common maximum legal reach of compatible muzzles,
including through mast gaps. It never switches to a short raw-camera-pitch shot.

## Owners

- `experiments/native-metal/cannon-trajectory-aim.patch` owns the engine edits
- `BuildManualAimSolution` shares world target and gun eligibility between firing
  and presentation in `src/libs/sea_ai/src/ai_ship_cannon_controller.cpp`
- `manual_aim_geometry.hpp` owns deterministic trajectory/contour math
- `AIShipCameraController::Fire` gets the actual selected character from the firing
  solution; the obsolete per-fort-cannon HUD targeting loop is skipped for the player
- `aim_volume.hpp` and the bridge in `backend.mm` own the soft-density Metal pass;
  `manual_aim_volume_bridge.hpp` defines its checked 32/16-byte records
- `ShipAimFootprint` depth-tests surface contacts without depth writes;
  `ShipAimReticle` is the small screen-centered orientation plus

## Correctness changes

- Camera surface picking uses the available broadside range independently of camera
  pitch. Gun reach, elevation and traverse are checked afterward at every real muzzle.
  Far intent intersects the camera bearing with every compatible muzzle's legal
  range disk, using the common limit and a centimeter-scale numerical margin
- World queries use pure collision traces with per-model AABB rejection. Island
  receivers come from `ISLAND_TRACE`, including extra location models and seabed
- Sea queries sample `WaveXZ`, refine the first crossing, and detect an initially
  submerged muzzle. Preview queries never call damage-producing `Cannon_Trace`
- Flight follows the actual RawAng/HeightMultiply transform. Jittered trajectories
  continue until real contact, rather than being snapped to the nominal target
- Every eligible gun supplies a trajectory. Spatial support guns add all eight
  independent yaw/elevation/speed jitter corners; array order does not pick the battery ends
- Cross-sections intersect trajectories at common downrange planes. Exact hull-edge
  planes from both slab endpoints preserve long, thin, rolled gun lines without
  narrowing or fattening them. Reversed/turning paths and empty axial gaps are retained
- A single soft-density pass integrates camera rays inside those slabs. It copies
  current post-water depth, reconstructs world-space receiver distance, and clips
  there. No polygon walls or per-gun additive volumes are rendered. Alpha is capped
  at 0.22; internal station caps are not feathered into visible bands
- Contact-field samples originate at real gun muzzles and follow complete ballistic
  trajectories. A missed hull/mast sample continues to the first real receiver,
  including water through a gap. No local-normal or sea-axis ellipse is projected
- Interior samples and bounded mixed-receiver refinement expose gaps missed by
  corner rays. Solid-receiver triangles validate their interior with the same
  ballistic trajectory mapping and subdivide/clip recesses. Triangles never bridge
  different receivers or sharp discontinuities
- Surface triangles have a 3.5 cm normal lift. Water triangles refine against
  `WaveXZ` midpoint error; unresolved storm chords are clipped rather than floated
- Volume dispersion traces use at most four spatial support guns. Contact fields
  use five regular support/center guns and one extra field for any otherwise
  unrepresented receiver found by another gun. Each field reserves 29 shared
  samples, then gives each of four contact cells its own 16-detail/8-local-patch
  budget (at most 125 ballistic samples per field). Early refinement cannot erase
  later cells. Receiver tessellation is capped at 16,000 triangles. Discovered gap-continuation supports also enter the corridor

## Preserved gameplay and inherited limitations

Projectile updates, damage, ammunition scripts, AI fire and save layout are unchanged.
Reload timing and ammunition mechanics are preserved; an invalid manual attempt
no longer clears the broadside charge when no gun fired. Published v3/v4 already suppressed the broadside-level random
`SHIP_GET_BORT_FIRE_DELTA` offset for manual fire. That existing policy is preserved;
per-ball random speed, direction and elevation remain active and are represented.

The shipped `AIBalls.c` compares the ammunition name with integer `GOOD_KNIPPELS`.
The current engine converts nonnumeric names to zero, so both ordinary balls and
knippels receive the catalogue's `HeightMultiply * 0.4`, despite the script's
intended `0.65` branch. The preview mirrors that effective behavior and reads the
live `Cannon` catalogue. Fixing the underlying script is a separate gameplay change.

This is a finite sampled envelope, not an exhaustive probabilistic bound. Static
rigid-model receivers and waves are represented. Animated cloth, sub-sample-size
gaps, very sharp receiver transitions and camera-near geometry need game replay.
Water-before-overlay ordering and depth-test state are source-checked; the staged
sea shader, actual storm occlusion and frame time still require native verification.

## Focused verification

After source staging has applied the ordered patch stack:

```sh
cd experiments/native-metal
c++ -std=c++20 -O2 \
  -I .cache/storm/src/libs/math/include \
  -I .cache/storm/src/libs/shared_headers/include \
  cannon_aim_geometry_probe.cpp -o /tmp/cannon-aim-geometry-probe
/tmp/cannon-aim-geometry-probe
```

The standalone probe covers downward/far-camera regressions, live ballistic warp,
reachability, dispersion lifetime, port/starboard symmetry, thin exact slabs,
emitted gap-to-water contact triangles, independent cell budgets and bridge ABI. It has no engine initialization or external
test framework. The replacement patch must also trial-apply on the exact pre-aim
source, with all subsequent ordered patches applying afterward.

## Native acceptance still required

Replay from the same first-person deck camera:

1. Both broadsides, including reordered/damaged gun locators and multiple gun decks
2. Close/downward water aim, level horizon and the maximum legal elevation
3. A nearby hull, mast/rigging, coastline and steep shore, without floating discs
4. Waves passing under the footprint and over low gun muzzles
5. Actual volleys versus the predicted spread, for ordinary balls and current knippels
6. Repeated reload/fire, unavailable sectors, sea-camera changes and unaffected AI fire
7. Frame time near a heavily armed fort and with multiple visible ships

A successful native encode logs `soft aim volume: exact-slab density encoded`.
That identifies the new path; it does not prove the appearance is accepted. The
known slab/roof screenshots predate this density implementation. Replay the new
renderer in day/night water, ship/mast gaps and steep shore before accepting it.
