# Sea cannon ballistics and volumetric projectiles

## Status

The October 7 installed batch extends the volume path with bounded sail-hole
enumeration and monotonic cumulative sail damage. Actual-script VM checks pass,
the corrected shared event header is delivered, and the current saved sea battle
opens on the combined37598a37 engine. The sinking-ship firing action and visible
sail-health replay remain unresolved; startup alone does not accept them.

## Contract

Naval round shot, knippels, grapes and bombs fly the stock Storm trajectory and
honestly clip rigging: mast poles and sail cloth register hits within the
projectile's effective radius instead of requiring an exact zero-radius ray hit.

## Scope

In scope: firing integration spec, point-vs-volume verdict, capsule collision
for sails and mast poles, per-type rigging radii.

Out of scope: hull/fort/island/sea traces (exact rays by design), aim overlay
(sibling Singer owns `cannon-trajectory-aim.patch`), damage economy retune.

## Owners

- Flight integration: `AIBalls::Execute` in
  `src/libs/sea_ai/src/ai_balls.cpp` (engine), fed by
  `PROGRAM/sea_ai/AICannon.c` (`Cannon_FireCannon`) and
  `PROGRAM/sea_ai/AIBalls.c` (`Ball_AddBall`).
- Muzzle origin and firing solution: `AICannon::RealFire` in
  `src/libs/sea_ai/src/ai_cannon.cpp` (cannon locator world position, height
  angle solver, `CANNON_FIRE` event).
- Sail hits: `SAIL::Cannon_Trace` -> `SAILONE::CheckSailSquar` in
  `src/libs/rigging/src/`, damage event `SHIP_SAIL_DAMAGE`.
- Mast hits: `SHIP::Cannon_Trace` mast loop in `src/libs/ship/src/ship.cpp`,
  damage event `SHIP_MAST_DAMAGE` / `SHIP_MAST_TOUCH_BALL`.
- Sail health: canonical `PROGRAM/battle_interface/BattleInterface.c`
  `ProcessSailDamage` retains cumulative `arSail.dmg`; hole-derived damage can
  raise that floor but cannot erase earlier hits. Repair remains the owner of
  intentional damage reduction. `_RandomHole2Sail` visits all bounded cloth slots,
  including a pristine zero mask, instead of stopping at its highest set bit.
- Damage model: `Ship_MastDamage` in `PROGRAM/sea_ai/AIShip.c`
  (ball +0.10, grapes +0.05, knippel +0.25, bomb +0.15; mast falls at 3.0).

## Firing integration spec (aim-viz mirror contract)

Muzzle: cannon locator world position (`ship matrix * vPos`) passed through
`CANNON_FIRE` into `Ball_AddBall(fX, fY, fZ)` as `vFirstPos`.

Muzzle velocity: `Ship.Cannons.SpeedV0 = cannon.SpeedV0 * goods.SpeedV0`
(`Cannon_RecalculateParameters`, x1.15 with LongRangeShoot).
`cannon.SpeedV0 = sqrt(FireRange * 9.81 / sin(2 * FireAngMax))`,
`FireAngMax = 0.60` rad stock. Goods multipliers (`store/initGoods.c`):
balls 1.0, grapes 0.6, knippels 0.9, bombs 0.8.

Per-shot dispersion (`Ball_AddBall`): `Dir += K * 12deg * (rnd-0.5)` with
`K = Bring2Range(0.5, 1.2, 0.2, 1.2, spread)`;
`SpdV0 += spread * (10 * 12degRad) * (rnd-0.5)`;
`Ang += spread * 15degRad * (rnd-0.5)`.
`spread = clamp(1.2 - min(1.25, TmpSkill.Accuracy + GunProfessional*.12
+ LongRangeShoot*.06) - arcade*.10, .05, 1.20)` is owned by `Ball_GetAccuracy`.
This per-shot formula differs from `AIShip.c`'s common broadside-position jitter.

Flight (`ai_balls.cpp`, analytic, NO drag):
`t += dt * 2.0 * TimeSpeedMultiply` (`dt` = frame ms / 1000,
`SpeedMultiply = 2.0` from canonical `AIBalls.c`, `TimeSpeedMultiply = 1.0` stock);
`x = V0*t*cos(Ang)`, `y_raw = V0*t*sin(Ang) - 9.81*t^2/2`;
HeightMultiply warp: rotate (x, y_raw) by RawAng, scale Y by HM, rotate back,
where HM = cannon.HeightMultiply (1.0 stock) * 0.70 normal shot / 0.85 knippel,
RawAng = signed angle between flat and true firing direction (0 for level
deck fire). World = FirstPos + Ry(Dir) * (0, y, x).

Collision sweep: per-frame segment src->dst, order sail entity, all
`SHIP_CANNON_TRACE` ships (masts, hull details, hull model), fort, island,
sea. Sail and mast hits are side effects only; the ball dies on hull, fort,
island or sea (`fRes <= 1`).

## Frozen baseline: point-like

The pre-volume baseline used zero-radius rays: `CheckSailSquar` was an exact ray-triangle
test, mast/hull traces are `NODE::Trace`/`GEOS::Trace` mesh rays, and
ropes/vants/yards are not in the cannon-trace set at all. Thin mast poles plus
knippel fall threshold (12 direct hits at +0.25) explain visible fly-throughs.

## Fix

`cannon-ball-volume.patch` adds `SetCannonTraceRadius` (default no-op) to
`CANNON_TRACE_BASE`; `AIBalls::Execute` sets a per-type rigging radius
before sail/ship traces and resets after: knippels 1.0 m (spinning chain span),
grapes 0.35 m, round shot and bombs half their visual size
(`fSize * 0.5 * SizeMultiply`: 0.10 / 0.15 m).

- Sails: plane-crossing hits accept cloth within radius (point-triangle
  distance); grazing passes accept near endpoints. Hole-poking and damage
  events unchanged; non-cannon picks stay exact.
- Masts: exact mesh ray kept (catches yards sharing the mast node); on a ray
  miss, an exact segment-segment capsule test against the mast axis
  (`mastMtx * vSrc/vDst`, pole radius 0.35 m) fires the same damage event.
  No double damage: ray hits skip the capsule.
- Hull, fort, island, sea: untouched exact rays; ball lifetime and damage
  values unchanged. No save-format change (transient members only).

## Acceptance criteria

- Knippels aimed at rigging knock masts/sails down over a sustained engagement
  instead of visibly passing through; round shot hull damage rate unchanged.
- Must not: grow hull/fort/island/sea hitboxes, change trajectory or damage
  numbers, or touch the sibling aim overlay and crime files.

## Discovery and evidence owners

Read `ai_balls.cpp` Execute/AddBall, `ai_cannon.cpp` RealFire,
`AIBalls.c`/`AICannon.c`, `sailone.cpp` CheckSailSquar,
`ship.cpp` SHIP::Cannon_Trace, `AIShip.c` Ship_MastDamage before editing.
Falsifier: `git apply --check` (exact, no fuzz, same as
`apply_source_patches.py`) plus staged sea-battle replay.

## Rejected

- Offset parallel rays in `AIBalls` (3x sail-trace cost, still inexact):
  true capsule math inside already-visited triangles is cheaper and exact.
- Growing hull/fort/island/sea hitboxes: would alter the damage economy for
  big targets that need no help.

## Unresolved

- Yard spars keep the zero-radius ray (register as hull hits via the ship
  model trace); node membership of yards vs masts unproven from source alone.
- Long parallel cloth skims (segment inside the slab, both endpoints far from
  cloth) still miss; crossing and endpoint geometry is covered.
