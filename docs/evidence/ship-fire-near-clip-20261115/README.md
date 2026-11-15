# Ship-fire near-plane bounds

Base:136a7c9b,2026-11-15. No native screenshot or GPU measurement.

## Reproduction and cause

The volume renderer previously replaced its bounds with the entire screen whenever
any AABB corner crossed the near plane. Those bounds also drive the shared fire
sample-work budget. A narrow box spanning x/y±0.1,z0.5..2 with near1 occupies1% of
the screen, but was charged100%. At2560×1440 and20samples this forces scale0.25
rather than1.0 with the same2M-sample budget. This is a synthetic numerical case,
not a measured gameplay speedup.

## Repair

The existing `ship_fire_projection.hpp` owns the homogeneous near-plane clipping
of the twelve AABB edges. Admitted corners and clipped edge intersections provide
projected extrema; only final bounds are clamped to the viewport. The renderer
uses those bounds without changing thermal state, sample budget or ray steps.

## Verification

Run `python3 docs/evidence/ship-fire-near-clip-20261115/check.py` from any directory.
It compiles against the actual current header, honoring `CXX`; `SANITIZERS` is
optional. CPU regression covers the100×overestimate, forward/reverse-Z equivalence,
fully behind/offscreen boxes, eye-inside bounds, exact near plane, nonfinite input
and an unaffected box entirely in front. Initial g++17 warnings-as-errors run PASS.

Risk: overly tight bounds could crop fire around the camera. Native Metal build
and camera motion through burning hull/sails remain OPEN for owner acceptance.
No full-engine compilation, GPU cost or visual-quality result is claimed.
This change does not restore missing detail in128²smoke atlas cells or64³explosion
volumes; those effects already draw at the full scene target resolution.
