# Naval ammunition cargo contracts, damage pipeline and combat mechanics

## Status

The common hull-material successor is installed on October16,20:52EDT;
it is not accepted in gameplay. `runtime.md` owns the delivery receipt.
The ammunition baseline below was
documented on October6. It captures the naval combat ammunition
pipeline across Storm 2.8 and Native Metal, cargo vs character inventory boundaries,
damage reduction math, and technical integration specifications for custom shot types.

## Common hull-material implementation contract

The player uses the laser to expose defects in the destruction system, not as
an isolated weapon feature. A continuous finite laser stroke, concentrated
round-shot/bomb/knippel impacts and a physical ram must remove material from the
same hull and weaken the same connections. A completed cut through a load path
must release its real section; unrelated damage elsewhere must not fabricate a
whole cut plane. Weapon-specific footprints and heat remain distinct. Script HP,
crew casualties, immunity, quests and TOUCH collision work retain their current
owners; publishing a mechanical hole cannot debit the same HP twice.

Source discovery found two independently maintained material projections:
accepted artillery hits enter `Blots::AcceptedHullHit`/`GEOM::SetHullCuts`, while
laser/ram fields enter `SHIP::ApplyPhonkLaserCut`. GEOM currently combines them
by replaying authored geometry, every cannon mark and every laser operation.
The finite ship history also stops after512 sample-cost units; increasing that
limit increases replay cost, and deleting old operations would restore wood.
These mechanisms do not satisfy the requested continuing destruction contract.

The shared owner is GEOM's retained material, published by SHIP's existing
atomic value-job queue. The implementation reuses the current exact finite
subtraction, material-role meshes, broadphase, fragment physics and aggregate5%
geometry-worker allowance. Its checkpoint preserves outer skin, canonical caps,
timber interior and finite walls separately, including object IDs, UVs, normals
and colours. A rebased clipping boundary must not replace the immutable authored
support domain or grow timber again. Current-material queries, visible/collision
geometry, support loss, ingress and the saved checkpoint publish together.

`Ship.HullMaterial` is an additive versioned attribute checkpoint, bound to the
model's authored content and a monotonic material revision. Ordinary attribute
SAVE serialization remains the only persistence transport; binary SAVE layouts
do not change. A complete checkpoint is authoritative. Older `Ship.Blots` and
`Ship.PhonkLaserCuts` reconstruct once through their established order, without
HP, crew, chips or historical fragment replay. Renderer reset restores current
material, and stale/cancelled/malformed projections retain the last valid state.
Only bounded pending work and current retained geometry remain live; an ever
growing operation log must not be required to continue damaging a ship.

`SHIP::ApplyHullProjectileImpact` owns the common artillery producer, including
the near-hull radial-burst caller in AIBalls. Its native profile identifies live
goods through the projectile registry, not copied goods IDs. Accepted script HP
loss is inverted through TOUCH's existing work curve E/(E+strength); strength
remains `SHIP_BASE::GetHullContactStrength`. Wrecks use the existing shot mass
and trajectory derivative without another HP event. This is a whole-hull
specific-work domain and an area-density material approximation, not calibrated
timber engineering data. The finite local contact footprint bounds where work
can remove material. Direct laser depth retains its existing delivered-time rule.

The shared kernel/checkpoint, GEOM/SHIP publication, legacy Blots import and
immediate crew-field consumption have one native owner. Finite ablation/crush
plugs produce bounded cosmetic chips; real bodies come from retained sections
that actually lose support. Already detached bodies retain their contact and
lifetimes. Legacy separating planes retain their detached section semantics.
Existing
shore, fire-anchor and fragment-query source fixes remain in the same successor.
The player owns gameplay testing. The integration owner may compile the engine
to catch C++ integration errors; compilation does not accept gameplay or FPS.
The owner subsequently requests installation of this frozen batch; root uses
`run.sh --stage-only`. Game control and gameplay probes remain declined.
Source review must distinguish local
notches from complete cuts, preserve old damage across another weapon/reset,
reject stale publication, and preserve undamaged/free-water behavior. Gameplay
acceptance remains open until the player observes these outcomes. Generic
cross-node attachments require real model support data; a transform origin is
not an attachment socket and cannot justify deleting neighboring assemblies.

Performance is a core acceptance criterion. Merely moving an ever growing
whole-hull reconstruction behind a worker does not close it. The shared material
implementation retains stable authored cell IDs/bounds, sparse current overrides
and a bounded exact tail before folding checkpoints. Untouched material is shared;
membership queries do not allocate. Workers pack checkpoints; frame publication
reuses unchanged packed bases and writes a bounded tail. A fold still serializes
the changed node's full current boundary, including untouched authored faces,
alongside sparse changed cell solids. Complete component connectivity, collision
rebuild and changed-node GPU upload still have a real cost; sustained cutting/FPS
and memory remain unmeasured until player replay. This is not fully sparse mesh
persistence or an accepted frame-time result.

Full repair, ship replacement and quest/traffic restoration share
`ShipsUtilites.c::Ship_ResetHullDamage`. Partial paid repair writes cumulative
`Ship.HullMaterialRepair.percent`; native material acknowledges only committed
work and restores actual authored material under area quota. Encounter handoff
copies checkpoint/repair/legacy/flood state, while map visuals use the same
versioned node decoder as SHIP. The ordinary attribute SAVE codec is unchanged.
Partial material addition and checkpoint folding are implemented in the installed batch;
compilation and source review cannot establish SAVE/repair gameplay acceptance.
Mast support uses the authored bottom-contact vertex set and base-to-tip path.
Removing the complete old tip witness together with the base while a middle
segment survives remains a detection limitation; generic cross-node attachments
also remain outside the supported model data.

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

### Heated-shot perk

October 13 source candidate. `HeatedShot` is a passive ship perk learned by the
hero or inherited from the assigned cannoner, requiring `CannonProfessional`
and `ImmediateReload` through the existing perk tree. Old saves rebuild the
registry through `PerkLoad` without granting or clearing learned perks.

`AIBalls.c` rolls `rand(4) == 0` once at launch, only for `GOOD_BALLS` with the
effective perk. The existing saved projectile event string carries
`@heated-shot`; cargo, velocity, direct damage and the binary SAVE layout remain
unchanged. `native-metal/heated-shot.patch` consumes that tag as data (never as
a script callback), tints the ordinary sprite warm red and resets
`CurrentBallHeated` before each projectile collision. Ordinary shots and other
ammunition must not inherit a preceding heated shot's flag.

`AIShip.c` admits local hull/cloth ignition with 20% probability for a heated
hit through the existing fire owner. A true hull critical retains its 50%
chance and combines the independent 20% hot-shot contribution into one 60%
roll and one fire site. A noncritical round-shot hit therefore has a 4% overall
hot-shot ignition probability before suppression, not a guaranteed fifth shot
or fifth hit. Other ammunition retains its existing fire probabilities.
The initial cannon-professional icon alias was rejected as incomplete identity.
The corrected source uses `PERK_HEATEDSHOT_ENABLE` / `PERK_HEATEDSHOT_DISABLE`
and a distinct hot iron ball held by furnace tongs, matching the existing rope
frame and golden perk inset. `src/assets/ui/manifest.json` owns the retained
generation/cleanup sources and the checksum-bound 256x128 TX sheet: learned
128px tile on the left, sepia unlearned tile on the right. Character menus and
prerequisite links both use `GetPerkPictureGroup`; the two old generic aliases
are removed. `prepare_perk.py` reproduces this format and the existing raking
sheet without changing that prior asset. It adds no cargo goods or fire timer.
The read-only installed delivery plan admits the exact new texture and rejects
an unrelated texture path. Decoded TX pixels equal the resized canonical source,
both full-bleed 128px states were reviewed beside the existing perk, and the
real script VM passes `HeatedShotArtCases=0`, `script_errors=0` for learned /
unlearned group selection and unchanged raking / ordinary perk groups.
This accepts source, format and consumer selection; the installed menu remains
unreplayed while game control is stopped.

Random NPC acquisition was a separate limitation in the original perk batch:
`PROGRAM/Loc_ai/LAi_equip.c:LAi_NPC_EquipPerk` granted a fixed eight-tier cannon
list, rather than the dynamic registry. It included neither `HeatedShot` nor
`RakingFire`; player learning/assigned-officer inheritance does not imply random
NPC acquisition. The original source/art batch did not change that selector.
The subsequent registry-driven NPC replacement and its delivery state belong
to `npc-generation.md`; its isolated VM proves HeatedShot reachability with
actual prerequisites, while real encounter replay remains pending.

Acceptance requires learning from both the hero and assigned cannoner,
red shots mixed with ordinary shots, local ignition on a vulnerable target,
and unchanged non-perk/other-ammunition behavior in the installed Metal game.
The canonical native build consumed the new last-stack patch successfully.
The refreshed `script-vm-probe` compiled the full installed PROGRAM chain with
the current read-only delivery projection and passed `HeatedShotCases=0`,
`script_errors=0`: registry/prerequisites, preservation of a prior learned perk,
hero/officer inheritance and removal, 20%/60% chances, and unchanged other ammo.
Only the fixture's `Main` is renamed to prevent real game startup. Calling
the dynamically unloaded perk-init segment twice within one uninterrupted probe
run produces an existing extern-call error; the accepted fixture uses the
normal single initialization and makes no claim about complete SAVE reload.
Source/native VM checks cannot accept the battle replay. Installation and
game input remain outside this source batch while the prior game-control stop
is in force.

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

