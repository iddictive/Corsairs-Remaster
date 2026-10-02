# Sea manual-aim envelope

## Status

Source candidate based on `cc42b40`. Runtime feedback showed that a pixel-sized
rounding filter could not fix the large angular footprint, and broken contours
made relation colors difficult to read. This revision changes the contact region
and actual rendered ownership. Native Metal compilation, appearance, motion and
frame-time acceptance remain pending.

## Contract

First-person deck camera controls aim. One subtle ballistic volume spans every
eligible broadside muzzle. Air density is weaker near the guns and strengthens
modestly downrange. A tiny centered `+` marks the shooting direction.

An invisible rectangular range probe matches the user's approximately 60×34-pixel
annotation at 2048×1285, scaled with the viewport. Actual mast/hull/fort polygons
inside that view region supply depth. The target remains on the center camera
ray at that depth: the rectangle does not redirect aim toward an off-center ship.
Selection is instantaneous, with no retained target, tracking or dwell. A nearer
center obstruction wins. Empty or genuinely out-of-range space uses common legal
maximum reach; nearby mechanically unreachable targets stay unavailable.

The visible contact area is an approximate rounded dispersion-density region,
not a guaranteed boundary containing every shell. The same smooth field is
evaluated on current water, hull, mast, terrain and fort depth. Physical first-hit
samples gate where it can appear, including continuation through real openings.
Those visibility masks do not draw their own polygon outlines. A faint interior
and colored rim share one composite; air still uses the stopped ballistic volume.

## Owners and implementation

- `cannon-trajectory-aim.patch` owns the engine controller, geometry and bridge edits
- `BuildManualAimSolution` shares range selection and per-muzzle eligibility between
  preview and manual fire. `MODEL::Clip` finds real polygons inside the small view
  frustum; pure traces confirm visibility. Bounding boxes only reject candidates
- The plus uses the current range-source relation. Firing events do not inherit a
  farther center-hit character after a nearer aperture receiver changes the range
- `manual_aim_geometry.hpp` mirrors live projectile warp and supplies exact section
  geometry; `manual_aim_volume_bridge.hpp` defines checked 64/32/16/8-byte GPU records
- `aim_volume.hpp` and `backend.mm` draw one soft-density/contact composite from
  current post-water depth and color snapshots. Per-gun surface triangles are gone
- Contact color and character identity come from actual depth-writing model draws,
  using the existing `GetRelation` mapping. One accumulated depth/token/color map
  captures registered hull/upper-model, fort, intact mast and owned sail/rope/vant
  groups. Exact current depth selects the actual receiver; water stays neutral
- Alpha-tested holes remain holes. Sails/ropes are non-stopping rendered contacts,
  gated by finite airborne support; they never stop a projectile or alter damage.
  Own hull and separately drawn own rigging are excluded by actual ownership
- The contact perimeter adapts its brightness to the underlying scene, with a
  restrained dark outer keyline for midtone/daylight contrast. This does not raise
  nighttime air opacity. Friend/enemy/neutral colors are green/red/yellow; water and
  unknown terrain remain neutral. Identity is not blurred through receiver gaps

## Physics, geometry and bounded work

Camera selection is followed by actual muzzle reach, elevation and traverse checks.
Far intent intersects the camera bearing with every compatible muzzle's legal
range disk and uses an inward numerical margin at the maximum-range boundary.

World queries are pure: no `Cannon_Trace`, damage events or random draws. Model
AABBs reject unrelated receivers; island tracing covers the full `ISLAND_TRACE`
layer. `WaveXZ` refines first crossings, including initially submerged muzzles.

Every eligible gun supplies its nominal trajectory. Spatial support guns add all
eight independent yaw/elevation/speed dispersion corners. Additional contact
samples follow complete trajectories through missed hull/mast gaps to their first
physical receiver. They discover continuation supports, not visible surface tiles.
Contact projectors use stable ship-local longitudinal/up extrema and a middle
gun, with near-tie handling. Each of at most five ordinary density fields contains
65 fixed axial sections derived from that gun's untruncated nominal trajectory and
independent dispersion corners. Each ellipse is inscribed in its sampled support
hull, preventing covariance overhang at close range. Muzzle-relative plane solves and explicit endpoint
insertion keep translation from dropping a field. A fourth-power mean combines
field densities once, retaining isolated regions without per-gun alpha blending.
Receiver discoveries add visibility support only, never a density field.

Each physical-support projector keeps a 4×4 nominal-speed grid and 2×2 grids at both
live speed limits. Twelve optional probes per cell bound refinement (355 maximum
marches per projector; uniform water 131, uniform solid 227). Same-first-water fans
form a retained 2048×2048 coverage atlas. Same-first-solid fans form bounded surface
prisms; their thickness comes from measured local sample deviation plus 1.5 cm, with
refinement or rejection above 0.5 m. Current-depth uncertainty is handled as a depth
bin, not arbitrary world expansion. Character prisms require exact receiver tokens.
Mixed receiver samples clip/refine visibility and never gain a line of their own.
No physical trajectory is extended past its first stopping impact.

Common downrange sections preserve actual muzzle/contact endpoints, reversed or
turning paths, thin rolled batteries and empty axial gaps. Exact hull-edge planes
from both slab endpoints avoid fixed-direction narrowing/fattening. Internal
station caps do not become visible contact seams. Air alpha remains capped at .22.

Range picking visits at most 4096 clipped polygons and confirms 32 nearest
candidates. GPU input caps are 16 density fields (1040 sections), 8192 contact prisms,
49,152 water vertices, 1024 air sections and 65,536 air side planes. Actual model
registration is capped at 128 receivers. Registry metadata is rebuilt without
rebuilding world model bounds.

Ownership resets on frame/camera changes and preserves later occluders. A model
capture starts only at its first depth-writing draw, so culled, empty and purely
non-depth-writing groups do no capture GPU work. Each visible registered group
still adds one full-depth copy and one depth/token/color pass, in addition to own
ship/water capture, the eligibility masks and full-viewport composite. Native
profiling remains necessary with many ships and rigging groups.

## Preserved gameplay and limits

Projectile updates, damage, ammunition scripts, AI fire and save layout are unchanged.
Reload timing/ammunition mechanics remain intact; invalid manual attempts do not
clear a broadside charge when no gun fires. The published manual broadside-level
random-offset suppression remains unchanged, while live per-ball jitter stays active.

The shipped ammunition script compares the ammunition name with integer
`GOOD_KNIPPELS`; effective current behavior applies catalogue `HeightMultiply*.4`
to ordinary balls and knippels. The preview mirrors it without changing gameplay.

The display is a finite sampled approximation. Rough unresolved curvature and
sub-sample openings can still lose eligibility; a flat-plane check does not prove
arbitrary cliff continuity. Blended cloth with depth writes disabled, mixed-owner
flag draws and unregistered detached models remain unsupported for ownership.
Actual storm occlusion, motion stability and colored day/night readability need
native replay. There is no temporal target retention or fake collision plane.

## Focused verification

After applying the ordered source-patch stack:

```sh
cd experiments/native-metal
c++ -std=c++20 -O2 \
  -I .cache/storm/src/libs/math/include \
  -I .cache/storm/src/libs/shared_headers/include \
  cannon_aim_geometry_probe.cpp -o /tmp/cannon-aim-geometry-probe
/tmp/cannon-aim-geometry-probe
```

Also trial-apply the replacement patch on the pinned pre-aim source and validate
all subsequent ordered patches. Probe and syntax checks are not Metal execution.

Native replay must cover day/night water and shore; the contact perimeter and
relation-colored plus; mast inside/outside the invisible rectangle; camera roll;
real volleys through gaps; both broadsides; storm crests and own deck occlusion;
reload/fire; a heavily armed fort and multiple ships. The new path logs
the current aim renderer revision on successful initialization.
