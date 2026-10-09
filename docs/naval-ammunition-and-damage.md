# Naval ammunition cargo contracts, damage pipeline and combat mechanics

## Status

Documented architecture, October 6. Captures the complete naval combat ammunition
pipeline across Storm 2.8 and Native Metal, cargo vs character inventory boundaries,
damage reduction math, and technical integration specifications for custom shot types.

## Contract

All naval ammunition (balls, grapes, knippels, bombs, powder) is ship cargo (`cargo.goods`),
measured in hundredweights and units, stored in the ship hold, and transferred only via
ship-to-ship transfer (`transfer_main.c`) or port stores. Ammunition never exists in personal
character inventory (`pchar.items`), personal chests, or cabin wardrobe containers.

## Scope

In scope:
- Cargo vs inventory ownership boundary and goods definitions.
- Gun deck firing pipeline and ammunition reload cycles.
- Ballistic flight simulation, particle attachments, and billboard atlas rendering.
- Collision dispatch, damage formulas (hull, sails, masts, crew, guns, fire, panic, surrender).
- Why round shot and bombs currently fail to kill crew (mathematical proof).
- Technical specification for integrating new ammunition types into Storm scripts and Native Metal.

Out of scope:
- Native Metal volumetric collision math (owned by `docs/sea-cannon-ballistics.md`).
- Manual trajectory overlay UI (owned by `docs/sea-aim-overlay.md`).

## Architecture and Owners

### 1. Cargo vs Character Inventory boundary

| Domain | Data Owner | Script / Engine Files | Notes |
| --- | --- | --- | --- |
| Ammunition & Powder | `rCharacter.Ship.Cargo.Goods` | `PROGRAM/store/goods.h`, `initGoods.c`, `storeutilite.c` | Registered as `TRADE_TYPE_AMMUNITION`. Measured in units and weight (cwt). Transferred via `transfer_main.c`. |
| Personal Items | `rCharacter.items.<itemID>` | `PROGRAM/items/items.h`, `items_utilite.c` | Sabres, pistols, spyglasses, potions, amulets. Stored in personal pockets or cabin chests. Never holds cannon shot. |
| Cannon Batteries | `rCharacter.Ship.Cannons` | `PROGRAM/cannons/cannons_init.c`, `cannons.c` | Gun calibers (lbs), types (cannon vs culverine), max range, reload time, damage multiplier. |

### 2. Gun Deck and Firing Pipeline

1. **Charge Selection**: Selected via `BattleInterface.c` (`hk_charge1` to `hk_charge4`, command `BI_Charge`). Sets `pchar.Ship.Cannons.Charge.Type` (`GOOD_BALLS`, `GOOD_GRAPES`, `GOOD_KNIPPELS`, `GOOD_BOMBS`).
2. **Reload Calculation**: `AICannon.c` (`GetCannonReloadTime`):
   `fReload = fBaseReload * (1.0 + (1.0 - fCrewExp) * 3.0) * (1.0 + (1.0 - fMorale/MORALE_NORMAL) * 0.2)`.
   If crew is below optimal or injured, reload time is tripled. Powder must be present in cargo hold; without powder, cannons cannot fire.
3. **Muzzle Origin and Dispatch**: `AICannon::RealFire` in `src/libs/sea_ai/src/ai_cannon.cpp` emits `CANNON_FIRE`, calling `Ball_AddBall` in `PROGRAM/sea_ai/AIBalls.c`.
4. **Trajectory and Dispersion**:
   - `SpeedV0 = Cannon.SpeedV0 * Goods.SpeedV0` (Balls 1.0, Grapes 0.6, Knippels 0.9, Bombs 0.8).
   - Angle and direction dispersion calculated from cannon skill (`Accuracy`).

### 3. Visuals, Particles and In-Flight Rendering

- **Render Technique**: Billboard quads rendered via `balls.fx` using `AIHelper::pRS->DrawRects` in `ai_balls.cpp`.
- **Atlas**: `RESOURCE/Textures/AllBalls.tga` with `SubTexX = 2`, `SubTexY = 2`:
  - Index 0: Bombs (`Particle = "bomb_smoke"`, `Size = 0.3`).
  - Index 1: Grapes (`Size = 0.2`).
  - Index 2: Balls (`Particle = "ball_smoke_low"`, `Size = 0.2`).
  - Index 3: Knippels (`Size = 0.2`).
- **Water & Obstacle Hits**:
  - `BALL_WATER_HIT` spawns particle `splash` or `splash_big` and plays `ball_splash`.
  - `BALL_FLY_NEAR_CAMERA` plays `fly_ball`.
  - Floating cargo in water: `Goods[...].Swim.Model` (e.g. `roll_of_materials`, `box_of_bottles`).

### 4. Collision and Damage Dispatch

Collision sweep checks sails (`CheckSailSquar`), masts (`SHIP::Cannon_Trace`), hull, fort, island, and water.
On hull impact, `SHIP_BALL_DAMAGE` dispatches to `Ship_ApplyBallDamage` in `PROGRAM/sea_ai/AIShip.c`.

#### Why Balls and Bombs Fail to Kill Crew (The Math)

The crew damage formula in `AIShip.c:3295` is:
`fCrewDamage = stf(rBall.DamageCrew) * fCannonDamageMultiply * AIShip_isPerksUse(...) * GetCrewDamageReduction(rOurCharacter)`

Followed by `Ship_ApplyCrewHitpoints` in `AIShip.c:2708`:
`fDamage = fCrewDamage * (1.0 - 0.75 * stf(rOurCharacter.TmpSkill.Defence))`
`if (Doctor2) fDamage *= 0.8; else if (Doctor1) fDamage *= 0.9;`

Base parameters in `initGoods.c`:
- `Goods[GOOD_BALLS].DamageCrew = 0.2`, `DamageHull = 11.5`
- `Goods[GOOD_BOMBS].DamageCrew = 0.5`, `DamageHull = 19.5`
- `Goods[GOOD_GRAPES].DamageCrew = 2.4`, `DamageHull = 1.0`
- `Goods[GOOD_KNIPPELS].DamageCrew = 0.3`, `DamageHull = 1.5`

Mathematical degradation on a healthy target (100% hull):
1. `GetCrewDamageReduction` returns 0.35 to 0.55 for Class 1-3 ships (65% to 45% reduction).
2. Captain Defence skill (0..1) reduces remaining damage by up to 75% (`1 - 0.75 * 1.0 = 0.25`).
3. Doctor perk reduces another 20% (`0.25 * 0.8 = 0.20`).
4. Result for Balls: `0.2 * 0.35 * 0.20 = 0.014` crew lost per ball hit.
   A 40-gun broadside hitting cleanly kills 0.5 to 1.0 sailors while dealing 2,000+ hull damage.
5. Result for Bombs: `0.5 * 0.35 * 0.20 = 0.035` crew lost per bomb hit.
   The target ship sinks long before losing even 5% of its crew.
6. The 25% immortal floor (`MinCrew * 0.25`) prevents crew death from falling below 25% of minimal crew via artillery.

#### Existing Secondary Damage Mechanisms in Scripts

1. **Gun Destruction**:
   `SendMessage(&AISea, "laffff", AI_MESSAGE_CANNONS_BOOM_CHECK, rOurCharacter, fTmpCannonDamage, x, y, z)`
   Evaluates gun battery explosions at coordinate `(x, y, z)` via `AIShipCannonController::CheckCannonsBoom`.
2. **Fire and Explosion**:
   `bInflame = true` triggers `PostEvent(SHIP_ACTIVATE_FIRE_PLACE, ...)` and `PostEvent(SHIP_FIRE_DAMAGE, ...)`.
   Critical boom (`bSeriousBoom`) triggers `Ship_Serious_Boom(x, y, z)` with 8..12x hull damage.
3. **Surrender at Sea**:
   `Ship_CheckSeaSurrender` evaluates whether the enemy captain yields based on remaining fleet threat.
   Sets `rCharacter.SeaSurrender`, `rCharacter.Surrendered = true`, and raises the white flag (`FLAG_WHT`).

### 5. Boarding and Prize Cargo Lifecycle

1. Boarding triggers `LAi_boarding.c`: deck combat phases, then captain cabin duel or cabin surrender.
2. After victory, `transfer_main.c` launches the sea transfer interface:
   - Cargo holds are opened between player flagship and captured prize.
   - Ammunition, food, rum, and trading goods are transferred via cargo slots.
   - Prize ship can be captured with an officer captain or scuttled / sunk.

## Specification for New Ammunition Types

To add custom ammunition cleanly without breaking native engine stability:

1. **Registry**: Extend `PROGRAM/store/goods.h` (`GOODS_QUANTITY` and `GOOD_*` defines) and `initGoods.c`.
2. **Visual Atlas**: Expand `RESOURCE/Textures/AllBalls.tga` from 2x2 to 4x2 sub-textures, updating `SubTexX = 4` in `PROGRAM/sea_ai/AIBalls.c`.
3. **Particles**: Add trail and detonation particle effects in `RESOURCE/Particles/`.
4. **Hit Dispatch**: Add custom logic in `AIShip.c:Ship_ApplyBallDamage` for special status effects (gun dismounting via `AI_MESSAGE_CANNONS_BOOM_CHECK`, steering damage, panic fire timers, moral shock).
5. **HUD Selector**: Expand `BI_ChargeState` in `PROGRAM/battle_interface/BattleInterface.c` with extended hotkeys or shift-combos.
6. **Cargo Distribution**:
   - Fort arsenals via port authority dialogues (`Common_Portman.c`).
   - Bermuda blacksmith craft order (`Pirates_Shipyard.c`).
   - Contraband beach merchants (`canbecontraband = 1`).
   - Plundered enemy warship holds in `transfer_main.c`.

