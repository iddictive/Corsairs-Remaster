# Sea manual-aim envelope

## Status

Polish source candidate based on `84f3c4e` after the water-continuity and zoom
updates. Gameplay feedback now accepts the overall appearance and continuous
water contact. This follow-up softens polygon corners and reduces contour artifacts
at solid silhouettes. Native compilation, motion replay and performance remain
pending; it does not claim to eliminate every source of ship-contact jitter.

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

Solid contact uses the corridor intersection with the current rendered surface.
Water contact uses connected actual first-impact coverage on the current rendered
sea: the stopped airborne hull cannot cover water between sampled impact stations.
Both have a faint interior and a soft colored perimeter, without
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
  geometry; `manual_aim_volume_bridge.hpp` defines checked 32/16/8-byte GPU records
- `aim_volume.hpp` and `backend.mm` draw one soft-density/contact composite from
  current post-water depth and color snapshots. Per-gun surface triangles are gone
- Contact color comes from exact clipped receiver polygons rendered into a private
  identity mask. They carry existing `GetRelation` ownership only, never visible
  coverage. Tight depth agreement is required; collision/render disagreement stays
  neutral. Actual before/after main-ship depth ownership excludes the firing ship
  from contact while preserving its ordinary depth occlusion, independent of LOD
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
Each projector keeps a 4×4 nominal-speed field and 2×2 fields at both live speed
dispersion limits. Each cell has its own 12-sample refinement/local-patch allowance
(at most 355 total samples per projector; ordinary uniform water uses 131).
Shared traced edge midpoints improve curved boundaries and can discover a solid
receiver inside an otherwise all-water cell. Ordinary sampling cost increases
from 67 to 131 per projector; the worst-case bound is unchanged.
Connected same-first-water fans are unioned once in a retained 2048×2048 XZ
coverage atlas; internal gun/triangle edges do not
define contours. Mixed receivers remain clipped instead of filling their shadows.
Actual main-water depth ownership gates this chart onto current visible water.
No trajectory is extended beyond its physical first impact.

The water stroke uses a four-screen-pixel local filter with at most 1.25 pixels
of inward contour recession. The original faint physical coverage remains,
including thin components. The filter cannot add coverage or close real gaps;
large physical lobes remain. Solid contour tangents use conservative one-sided
depth neighbors, rejecting own ship, water, large depth jumps and opposing folds.
Relation identity and target selection are neither blurred nor retained.

Common downrange sections preserve actual muzzle/contact endpoints, reversed or
turning paths, thin rolled batteries and empty axial gaps. Exact hull-edge planes
from both slab endpoints avoid fixed-direction narrowing/fattening. Internal
station caps do not become visible contact seams. Air alpha remains capped at .22.

Range picking visits at most 4096 clipped polygons and confirms at most 32 nearest
candidates. Relation collection is capped at 49,152 vertices (16,384 triangles),
with corridor-AABB rejection. Missing/capped identity is neutral, not invented.
Water contact triangles are capped at 49,152 vertices. Ownership capture is enabled
by the current deck-camera state, resets each frame/camera, and preserves later
occluders. Missing either live ownership capture withholds contact rather than
reusing a stale/fragmented fallback; air and the plus remain available. It adds
two depth copies and two ownership passes while aiming; the
water composite conservatively covers the viewport. GPU records are capped at
1024 sections and 65,536 side planes. Native frame-time
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
Collision mesh/render LOD differences still affect relation coloring. Animated
cloth, sub-sample openings, separately drawn ropes/crew and sharp silhouettes
require native replay. Main-ship depth ownership covers only its actual draw scope. Depth/color snapshots
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
`soft aim volume v3: endpoint-water union + rendered receiver ownership`.
