# Ship contact with islands

## Status

October16 source candidate. Source-bound CPU sanitizer checks and native compiler
syntax pass; canonical build/staging and the installed player scene are unresolved.
The installed engine remains the last verified image recorded in `runtime.md`.

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
