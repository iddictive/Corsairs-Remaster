# Sea manual-aim overlay

## Status

Source candidate, October 1. v4 patch trial-applies exact; build, stage and replay pending (game running).

## Contract

First-person deck aim shows where the crosshair ray lands: three honest trajectory arcs plus one dispersion ellipse floating over the impact surface.

## Scope

In scope: overlay arcs/ring/cross, the shared point-and-shoot target with Fire(), manual-fire bort-delta removal, reticle restore.

Out of scope: projectile volume (docs/sea-cannon-ballistics.md), damage economy, AI fire (keeps the scripted scatter).

## Owners

- DrawManualAimOverlay, ManualAimTarget, AimMarchArc, EmitAimDrapeRing, AimDrapeY in src/libs/sea_ai/src/ai_ship_cannon_controller.cpp, via experiments/native-metal/cannon-trajectory-aim.patch.
- Technique ShipAimArc in src/techniques/_dev/ship.fx (ShipAimVolume stays unused in fx after the tube kill).
- Gunner scatter model: Ship_GetBortFireDelta in PROGRAM/sea_ai/AIShip.c (fAccuracy x Bring2Range grows to tens of meters at range; AI-only after v3).
- Wave ceiling: fMaxSeaHeight in PROGRAM/weather/WhrSea.c (calm 0.5, normal 2.0).

## Acceptance criteria

- Aiming at water shows a visible ellipse at the splash point in normal weather; aiming at a ship shows arcs ending on it plus a small ring; aiming at terrain shows one clean ellipse above the slope; close range shows short arcs with no screen-filling sheets.
- Must not: fill sheets, per-vertex ring chords through terrain, sea markers below 1 m, any random offset between the crosshair and the manual-fire impact.

## Discovery and evidence owners

Read the overlay block in ai_ship_cannon_controller.cpp, Ship_GetBortFireDelta and the WhrSea.c wave heights before editing. Falsifier: git apply --check --whitespace=nowarn against the reversed base plus a staged sea replay (water/ship/shore/close-range).

## Rejected

- Volumetric tube (v3): white highway at range, sheets plus stray lines close up; killed after 4 player screenshots, October 1.
- Per-vertex draped ring (v3): chords sliced through cliffs (crooked cork); replaced by a max-sample raised-flat ring.
- Sea marker 0.35 m (v2/v3): buried in normal 2.0 m chop, so no ring was visible on water or waterline ship locks; raised to 1.5 m.
- Ribbon with a bright far end (v2): blob at impact; replaced first by the tube, then by bare arcs.
- Deleted stock reticle (v2): out-of-sector aim showed nothing at all; reticle restored in v3.
- Random bort-delta on manual fire: splashes landed tens of meters off the ring; removed (AI keeps it).

## Unresolved

- Alleged left-bort asymmetry (no arc from the left bort): the code path is side-symmetric; needs a repro screenshot with reticle state and whether the guns fire.
- Far-aim (horizon/sky) readability: arcs honestly run to max range; a pin marker is speculated, not built.
- Heavy-storm ring wash: the 1.5 m marker can submerge past fMaxSeaHeight 2.0; accepted.
