# Sea manual-aim envelope

## Status

Local source candidate based on `4e9a09ab425dc8266131788067080bea7c8ad2bd`.
Native Metal build, staging, performance measurement and in-game visual acceptance are pending.
Static checks and numerical probes do not establish visual acceptance.

## Contract

First-person deck camera controls the aim. One subtle, continuous world-space
ballistic envelope spans the valid guns of the selected broadside. Its footprint
is projected onto the actual moving water, hull or terrain receiver. No individual
bright gun beams, center spine, distance ticks, extra stock reticle or floating
horizontal ellipse are drawn.

The envelope is a sampled prediction of live dispersion, not a promise that every
random shell will strike one point. Unsupported/unreachable manual solutions are
excluded; a failed downward water pick never becomes a maximum-range sky shot.

## Owners

- `experiments/native-metal/cannon-trajectory-aim.patch` owns the engine edits
- `BuildManualAimSolution` shares world target and gun eligibility between firing
  and presentation in `src/libs/sea_ai/src/ai_ship_cannon_controller.cpp`
- `manual_aim_geometry.hpp` owns deterministic trajectory/contour math
- `AIShipCameraController::Fire` gets the actual selected character from the firing
  solution; the obsolete per-fort-cannon HUD targeting loop is skipped for the player
- `ShipAimVolume` and `ShipAimFootprint` in `src/techniques/_dev/ship.fx` explicitly
  enable depth testing and disable depth writes

## Correctness changes

- Camera surface picking uses the available broadside range independently of camera
  pitch. Gun reach, elevation and traverse are checked afterward at every real muzzle
- World queries use pure collision traces with per-model AABB rejection. Island
  receivers come from `ISLAND_TRACE`, including extra location models and seabed
- Sea queries sample `WaveXZ`, refine the first crossing, and detect an initially
  submerged muzzle. Preview queries never call damage-producing `Cannon_Trace`
- Flight follows the actual RawAng/HeightMultiply transform. Jittered trajectories
  continue until real contact, rather than being snapped to the nominal target
- Every eligible gun supplies a trajectory. Spatial support guns add all eight
  independent yaw/elevation/speed jitter corners; array order does not pick the battery ends
- Cross-sections intersect trajectories at common downrange planes. Contours have
  consistent ordering and winding, retaining the exact hull corners even for a
  long, thin, rolled gun line. Reversed/turning station intersections are retained
- Footprints are projected onto each exact receiver along its local contact normal,
  with a 3.5 cm lift. Disconnected receivers and sharp discontinuities are not bridged
- Expensive dispersion traces are limited to at most four spatial support guns;
  footprints use four radial rings with 24 regular angular samples plus every
  actual convex-hull corner per contacted receiver

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
rigid-model receivers and waves are represented. Visual fidelity around animated
sails, very sharp receiver transitions and camera-near geometry needs game replay.

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

The small standalone probe covers the downward-camera regression, actual ballistic
warp equivalence, reachability, dispersion lifetime, port/starboard symmetry,
convex contours and segment bounds. It has no engine initialization or external
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

Do not call this visually accepted until native frames and player replay confirm it.
