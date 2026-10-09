# Corsairs & Storm 2.8 / Native Metal Engine: Complete Master Architecture

## Status

Active master architecture reference, October 6. Definitive source of truth for the entire
game system across the native C++/Metal engine and the Storm script runtime.
Required reading for all future agents before architectural, engine, or script changes.

## Architectural Model: The Two-Tier Foundation

Corsairs operates on a strict two-tier architecture:

```
+-----------------------------------------------------------------------------------+
|                           NATIVE C++ / METAL ENGINE                               |
|                                                                                   |
|  [Core & Entity ECS] <---> [Script VM / Bytecode] <---> [Metal Renderer / Shaders] |
|  [Physics / Buoyancy]       [Skeletal Animation]         [Spatial Tracing / Collide]|
|  [3D OpenAL Audio]          [Particle Engine]            [DirectInput / Event Loop] |
+-----------------------------------------------------------------------------------+
                                         |
             SendMessage / PostEvent / Event Handlers / Attributes
                                         v
+-----------------------------------------------------------------------------------+
|                        STORM SCRIPT RUNTIME (PROGRAM/)                            |
|                                                                                   |
|  [seadogs.c] Main Loop & Save/Load Coordinator                                    |
|  [worldmap/] Strategic Map, Dynamic Encounters, Island Coordinates                |
|  [sea_ai/]   Naval Battle FSM, Cannon Ballistics, Ship Damage & Sinking           |
|  [Loc_ai/]   Land AI (FSM), Melee & Firearm Combat, Multi-Deck Boarding            |
|  [locations/] 3D World Scenes, Locators, Day/Night Lighting Transitions           |
|  [characters/] RPG Progression (P.I.R.A.T.E.S.), Personal/Ship Skills, Officers   |
|  [quests/]   Quest State Machine, Condition Triggers, Delayed Event Timers        |
|  [dialogs/]  Branching Dialogue Trees, Barter, Reputation & Quest Hooks           |
|  [store/]    Colony Economy, Goods Registry, Smuggling & Pricing Model            |
|  [ships/]    Ship Definitions (Classes 1..7), Upgrades & Tuning                   |
|  [cannons/]  Artillery Calibers, Powder Consumption, Reload Calculations          |
|  [interface/] Full-Screen UI Screens & BattleInterface Combat HUD                 |
+-----------------------------------------------------------------------------------+
```

---

## 1. Engine Core & Entity Component System

### 1.1 Entities and Execution Layers
The native engine (`experiments/native-metal/.cache/storm/src/libs/core`) manages all game objects as `Entity` instances. Every entity belongs to one or more execution layers:
* `EXECUTE` / `REALIZE`: Standard land/menu layers (update logic vs render pass).
* `SEA_EXECUTE` / `SEA_REALIZE`: High-priority maritime layers for ships, sea mesh, balls, rigging, and weather.
* Layer dispatch: Scripts bind entities to layers using `LayerAddObject(layerID, &object, priority)` and remove them with `LayerDelObject(layerID, &object)`.

### 1.2 Attributes Tree System (`ATTRIBUTES` & `AClass`)
All script-accessible data in C++ is wrapped in an attribute tree:
* Each node can hold child nodes and primitive typed values (`int`, `float`, `string`, `dword`, `pointer`).
* Dynamic attribute access in scripts: `object.field.subfield`.
* Pointers: References created with `makearef(arefTarget, object.field)` or `makeref(refTarget, Characters[i])`.
* Fast type converters: `sti(val)` (string-to-int), `stf(val)` (string-to-float), `stb(val)` (boolean check).
* Existence check: `CheckAttribute(refObj, "attribute.name")`.

### 1.3 Communication Bus: Messages and Events
Scripts and the engine communicate through two channels:
1. **Synchronous Messages (`SendMessage`)**:
   Calls directly into C++ entity `ProcessMessage(MESSAGE &msg)`.
   * Example: `SendMessage(&AISea, "laffff", AI_MESSAGE_CANNONS_BOOM_CHECK, rOurCharacter, damage, x, y, z)` evaluates gun battery explosions.
   * Example: `SendMessage(&SeaOperator, "lalffffff", MSG_SEA_OPERATOR_BALL_UPDATE, ...)` updates in-flight ball tracking.
2. **Asynchronous Events (`PostEvent`)**:
   Queues an event to the engine event loop with a delay in milliseconds.
   * Example: `PostEvent(SHIP_FIRE_DAMAGE, delayMs, "lllf", victimIdx, attackerIdx, fireIdx, duration)`.
3. **Script Event Handlers (`SetEventHandler` / `#event_handler`)**:
   Engine events dispatched to script functions:
   * `BALL_WATER_HIT`, `SHIP_BALL_DAMAGE`, `SHIP_MAST_DAMAGE`, `EVENT_LOCATION_LOAD`, `EVENT_CHARACTER_DEAD`.

---

## 2. Script Runtime Architecture (`PROGRAM/`)

The script language is a proprietary C-subset compiled to bytecode at runtime.

### 2.1 Global Arrays and Core Singletons
The script runtime maintains several master arrays initialized at boot (`globals.c`):
* `Characters[TOTAL_CHARACTERS]`: 1200 character slots. Slot 0 is reserved; `nMainCharacterIndex` points to the active player (`pchar`).
* `RealShips[REAL_SHIPS_QUANTITY]`: Instantiated ship models with rolled HP, speed, turn rate, and upgrade modifications.
* `ShipsTypes[SHIP_TYPES_QUANTITY]`: Static base ship archetypes loaded from `ships_init.c`.
* `Goods[GOODS_QUANTITY]`: 51 trade goods loaded from `store/goods.h` and `initGoods.c`.
* `Locations[MAX_LOCATIONS]`: Static world scenes loaded from `locations/init/*.c`.
* `Nations[MAX_NATIONS]`: 5 colonial powers: England, France, Spain, Holland, and Pirates.
* `Islands[MAX_ISLANDS]`: Island geographic profiles and port locators.
* `Colonies[MAX_COLONIES]`: City profiles, governors, fort commanders, and store rosters.

### 2.2 Memory Boundaries: Durable vs Ephemeral (`Tmp`) State
* **Durable State (Serialized to Savegames)**:
  * Character stats, inventory, active quests, ship parameters, colony ownership, debt ledgers.
* **Ephemeral State (`Tmp`)**:
  * Attributes prefixed with `Tmp` (`rChar.TmpSkill`, `rChar.TmpPerks`, `rChar.Tmp.fShipTurnRate`).
  * Recalculated dynamically on scene load or state transition. **Never persist or rely on `Tmp` across saves or location changes.**

---

## 3. The Four Core Game Modes & Lifecycle Transitions

### 3.1 Mode 1: World Map (`worldmap/`)
* **Engine Owner**: `libs/worldmap` (`CWorldMap`).
* **Script Owner**: `PROGRAM/worldmap/worldmap.c`, `worldmap_encgen.c`.
* **Behavior**:
  * Strategic top-down navigation across the Caribbean sea. Time runs at ~1 hour per real second.
  * Encounters generate dynamically via `worldmap_encgen.c` (patrols, merchants, pirates, storms, floating barrels).
  * Danger zones and nation borders dictate hostility and encounter fleets.
* **Transition Out**:
  * Interacting with an encounter or land locator calls `Sea_Load()`, spawning 3D Open Sea mode.

### 3.2 Mode 2: Open Sea / Naval Combat (`sea_ai/`, `ship/`, `sea/`)
* **Engine Owner**: `libs/sea`, `libs/ship`, `libs/rigging`, `libs/sea_ai`.
* **Script Owner**: `PROGRAM/sea_ai/AIShip.c`, `AICannon.c`, `AIBalls.c`, `BattleInterface.c`.
* **Behavior**:
  * 3D wave simulation (Gerstner waves), wind dynamics, ship inertia, sail aerodynamic lift.
  * Artillery combat: broadsides, ballistic projectile traces, mast/sail clipping, hull splinter damage.
  * Tactical AI FSM: `AITASK_ATTACK`, `AITASK_RUNAWAY`, `AITASK_DEFEND`.
  * Sea surrender: Enemy ships with shattered morale raise the white flag (`FLAG_WHT`).
* **Transition Out**:
  * Grappling an enemy ship triggers `Sea_Boarding()`.
  * Entering port waters triggers `Location_Load()`.
  * Disengaging from combat allows returning to World Map via `Sea_MapLoad()`.

### 3.3 Mode 3: Land Locations (`locations/`, `Loc_ai/`)
* **Engine Owner**: `libs/location`, `libs/blade`, `libs/animation`, `libs/dialog`.
* **Script Owner**: `PROGRAM/locations/`, `Loc_ai/`, `dialogs/`.
* **Behavior**:
  * Third-person character movement in towns, jungles, forts, taverns, and caves.
  * Scenes are assembled from 3D meshes with embedded locators:
    * `reload`: transition triggers between scenes.
    * `goto` / `patrol`: NPC waypoint paths.
    * `monsters` / `soldiers`: combat spawn points.
    * `item`: ground loot locations.
    * `sit` / `merchant`: NPC interaction anchors.
* **Transition Out**:
  * Stepping into a town gate, tavern door, or port pier locator triggers `Location_Load()` to the next scene or `Sea_Load()` to sea.

### 3.4 Mode 4: Boarding (`LAi_boarding.c`)
* **Engine Owner**: `libs/location` (loads dedicated ship deck models: `Deck_Sloop`, `Deck_Frigate`, `Deck_Manowar`).
* **Script Owner**: `PROGRAM/Loc_ai/LAi_boarding.c`, `transfer_main.c`.
* **Behavior**:
  * Phase 1: Upper deck melee. Player crew vs enemy crew scaled by ship crew counts, weapons, and morale.
  * Phase 2 (for large ships): Gun deck or lower deck skirmish.
  * Phase 3: Captain's cabin duel against the enemy captain (or cabin surrender).
* **Transition Out**:
  * Victory launches `transfer_main.c`: ship hold looting, crew recruitment, taking prisoners, ship swapping, or scuttling.
  * Once transfer is closed, returns to Open Sea mode with the defeated ship removed or assigned as an escort.

---

## 4. Subsystem Deep-Dive

### 4.1 Artillery and Damage Pipeline
* **Ammunition vs Items**:
  * Cannon shot and powder are **strictly ship cargo (`rCharacter.Ship.Cargo.Goods`)**.
  * Registered as `TRADE_TYPE_AMMUNITION` in `store/goods.h`.
  * Measured in hundredweights (cwt) and units of 20. Carried in the cargo hold, never in personal inventory (`items`).
* **Firing Cycle**:
  1. `BattleInterface.c` selects charge (`GOOD_BALLS`, `GOOD_GRAPES`, `GOOD_KNIPPELS`, `GOOD_BOMBS`).
  2. `AICannon.c` calculates reload time based on cannons, gun crew count, crew experience, and morale.
  3. Gun firing triggers `AICannon::RealFire` -> `CANNON_FIRE` -> `Ball_AddBall` in `AIBalls.c`.
  4. `ai_balls.cpp` simulates ballistic trajectory and traces collisions against sails, masts, and hull.
* **Damage Calculation (`AIShip.c` & `initGoods.c`)**:
  * Hull Damage: `fHP = fDistanceDamageMultiply * fCannonDamageMultiply * stf(rBall.DamageHull)`.
  * Critical Hit: `bSeriousBoom` triggers 8..12x hull damage and volumetric explosion.
  * Crew Damage: `fCrewDamage = stf(rBall.DamageCrew) * fCannonDamageMultiply * Perks * GetCrewDamageReduction(rOurCharacter)`.
    * `GetCrewDamageReduction` reduces crew damage by 45–65% on healthy hulls.
    * `Ship_ApplyCrewHitpoints` applies Defense skill reduction (`1 - 0.75 * Defence`) and doctor perks, resulting in minimal crew loss from round shot and bombs.
    * 25% minimum crew is completely immortal against naval gunfire.
* **Secondary Damage Events**:
  * Gun Battery Destruction: `AI_MESSAGE_CANNONS_BOOM_CHECK` destroys guns on the struck side.
  * Fire: `bInflame` activates `SHIP_ACTIVATE_FIRE_PLACE` and `SHIP_FIRE_DAMAGE`.
  * Mast Destruction: `Ship_MastDamage` accumulates damage until reaching threshold 3.0, causing the mast to fall.

### 4.2 RPG System: P.I.R.A.T.E.S. and Progression
Defined in `PROGRAM/characters/characters.h` and `rpg.c`:
* **SPECIAL Primary Attributes (1..10)**:
  * **S**trength (Сила), **P**erception (Восприятие), **E**ndurance (Выносливость), **C**harisma (Харизма), **I**ntellect (Интеллект), **A**gility (Ловкость), **L**uck (Удача).
* **Skills (1..100)**:
  * **Personal**: Light Blades, Medium Blades, Heavy Blades, Pistols, Fortune.
  * **Ship**: Leadership, Commerce, Accuracy, Cannons, Sailing, Repair, Grappling, Defence, Sneak.
* **Skill Thresholds & Officer Contribution**:
  * Officers assigned to roles (Navigator, Boatswain, Gunner, Doctor, Carpenter, Purser) contribute their skills directly to the flagship.
  * The hero's Effective Skill = `Max(HeroSkill, AssignedOfficerSkill) - ShipClassPenalty`.

### 4.3 Land AI and Melee Combat (`Loc_ai/`)
* **AI Templates**:
  * `LAi_type_player`: player keyboard/mouse control.
  * `LAi_type_warrior`: hostile/friendly combatant seeking targets, evaluating distance, and attacking.
  * `LAi_type_guard` / `patrol`: law enforcement, investigates crimes and initiates combat.
  * `LAi_type_citizen` / `merchant`: non-combatants, flee when swords are drawn.
* **Combat Mechanics (`LAi_fightparams.c`)**:
  * Actions consume energy/stamina: Regular attack, Fast attack, Power attack, Round slash, Parry, Feint, Block.
  * Pistols have per-model reload timers and require paper cartridges / lead bullets in inventory.

### 4.4 Economy, Colony Stores & Smuggling (`store/`)
* **Store Inventory**:
  * Colony stores (`initStore.c`) stock goods categorized into:
    * `TRADE_TYPE_NORMAL`: consumer commodities (coffee, sugar, rum, tobacco).
    * `TRADE_TYPE_AMMUNITION`: round shot, grapes, knippels, bombs, powder, planks, sailcloth.
    * `TRADE_TYPE_CONTRABAND`: banned goods specific to each colony nation.
* **Smuggling Workflow**:
  * Speaking with the tavern smuggler agent initiates an off-port deal.
  * Meeting smugglers on a deserted shore triggers a cargo trade interface.
  * Coast guard patrols have a dynamic probability of ambushing the deal.

### 4.5 Dialogue Engine (`dialogs/`)
* Dialogues follow a state machine driven by node strings:
  * `Dialog.CurrentNode`: active node in the NPC script.
  * `Link.l1.go = "target_node"`: node transition on option selection.
  * `Link.l1.text = "dialogue line"`: text presented to player.
  * Node logic checks skills, quest attributes, nation relations, and inventory before presenting branches.

### 4.6 Quests and Event Machine (`quests/`)
* Quests are event-driven state machines attached to `pchar.quest.<quest_id>`:
  * Condition triggers: `QuestCheckLocation`, `QuestCheckNPCDeath`, `QuestCheckTime`, `QuestCheckBattleOver`.
  * Delayed execution: `DoQuestCheckDelay(questName, delaySeconds)` queues future callbacks.
  * Quest journals are logged to `QuestBook/` records.

---

## 5. Native Metal Engine Build & Staging Pipeline

The native macOS port lives in `experiments/native-metal`:
* **Canonical Executable Target**: `/Applications/Corsairs Iddictive Remaster.app`.
* **Renderer**: `experiments/native-metal/src/` with Metal backend (`backend.mm`, `resources.hpp`, `land_shadow.hpp`).
* **Source Patch Stack**: `experiments/native-metal/build.sh` calls `apply_source_patches.py` to apply ordered patches.
* **Canonical Build & Staging Commands**:
  * Engine build: `./experiments/native-metal/build.sh`
  * Staging inputs into the app bundle: `./experiments/native-metal/run.sh --stage-only`
* **Verification Gate**:
  * `python3 tools/agent_context.py --check` must pass with zero nonignored untracked files before closing a task.

---

## 6. The 10 Iron Rules for Agents

1. **Ammunition is Cargo**: Cannonballs, bombs, grapes, knippels, and powder belong exclusively to `Ship.Cargo.Goods`. Never put them in `Character.items` or loot boxes.
2. **Never Edit Engine Without Staging**: Editing `.cpp` or `.mm` does nothing in the game until `build.sh` compiles it and `run.sh --stage-only` stages it into `/Applications/Corsairs Iddictive Remaster.app`.
3. **Respect Save Boundaries**: Native Metal keeps its own isolated saves. Do not touch or cross-pollinate with archived Wine/CrossOver saves.
4. **BaseShips vs RealShips**: Do not edit `ShipsTypes` for live ship changes. Edit `RealShips` or `Character.Ship`.
5. **Never Store State in `Tmp`**: `TmpSkill` and `TmpPerks` are overwritten on every location transition.
6. **Preserve Dialog Links**: A broken `Link.l1.go` node crashes dialogue execution or freezes the game.
7. **Keep `agent_context.py --check` Green**: Every newly created file must be tracked in Git or added to `.gitignore`.
8. **Honor Layer Priorities**: When creating new entities, register them in both `EXECUTE` and `REALIZE` with valid priorities.
9. **Never Block on Sleep > 60s**: Keep agent operations responsive and avoid long polling loops.
10. **Verify on the Real App Surface**: Compilation and log messages are supporting evidence; only in-game execution on `/Applications/Corsairs Iddictive Remaster.app` confirms acceptance.

