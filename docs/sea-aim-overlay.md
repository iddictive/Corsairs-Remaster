# Sea manual-aim envelope

## Status

Follow-up source candidate based on `7144e7895ff57f5a47fce436f7d4a8036e7c3a0c`.
The supplied PR2 gameplay images show unacceptable contact tiles, poor daylight
readability and range jumps. This revision addresses those paths. Native Metal
build, shader compilation, performance and new gameplay acceptance remain pending.

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

The visible contact is the intersection of the complete corridor with the current
rendered surface. It has a faint interior and a soft colored perimeter, without
individual gun-field tiles, grid edges, floating ellipses or alpha accumulation.
Actual water depth follows rendered waves; hull, mast, terrain and fort depth
supply their own 3D surfaces. Air remains depth-occluded by foreground geometry.

## Owners and implementation

- `cannon-trajectory-aim.patch` owns the engine controller, geometry and bridge edits
- `BuildManualAimSolution` shares range selection and per-muzzle eligibility between
  preview and manual fire. `MODEL::Clip` finds real polygons inside the small view
  frustum; pure traces confirm visibility. Bounding boxes only reject candidates
- The plus uses the current range-source relation. Firing events do not inherit a
  farther center-hit character after a nearer aperture receiver changes the range
- `manual_aim_geometry.hpp` mirrors live projectile warp and supplies exact section
  geometry; `manual_aim_volume_bridge.hpp` defines checked 32/16-byte GPU records
- `aim_volume.hpp` and `backend.mm` draw one soft-density/contact composite from
  current post-water depth and color snapshots. Per-gun surface triangles are gone
- Contact color comes from exact clipped receiver polygons rendered into a private
  identity mask. They carry existing `GetRelation` ownership only, never visible
  coverage. Tight depth agreement is required; collision/render disagreement stays
  neutral. The firing ship is explicitly contact-excluded while still occluding air
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
Each projector uses 41 mandatory samples plus at most 64 boundary samples.

Common downrange sections preserve actual muzzle/contact endpoints, reversed or
turning paths, thin rolled batteries and empty axial gaps. Exact hull-edge planes
from both slab endpoints avoid fixed-direction narrowing/fattening. Internal
station caps do not become visible contact seams. Air alpha remains capped at .22.

Range picking visits at most 4096 clipped polygons and confirms at most 32 nearest
candidates. Relation collection is capped at 49,152 vertices (16,384 triangles),
with corridor-AABB rejection. Missing/capped identity is neutral, not invented.
GPU records are capped at 1024 sections and 65,536 side planes. Native frame-time
measurement remains necessary near a fort and with many ships.

## Preserved gameplay and limits

Projectile updates, damage, ammunition scripts, AI fire and save layout are unchanged.
Reload timing/ammunition mechanics remain intact; invalid manual attempts do not
clear a broadside charge when no gun fires. The published manual broadside-level
random-offset suppression remains unchanged, while live per-ball jitter stays active.

The shipped ammunition script compares the ammunition name with integer
`GOOD_KNIPPELS`; effective current behavior applies catalogue `HeightMultiply*.4`
to ordinary balls and knippels. The preview mirrors it without changing gameplay.

The envelope is a finite sampled dispersion prediction, not an exhaustive bound.
Collision mesh/render LOD differences, animated cloth, sub-sample openings, own
ship masking and sharp silhouettes require native replay. Depth/color snapshots
are current and post-water in source; actual storm occlusion is still unverified.

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
`soft aim volume v2: unified depth contact + verified relation mask`.
