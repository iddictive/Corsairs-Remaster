# Sail capacity from cloth area

## Status and outcome

Next-batch source candidate; not yet consumed by the installed app. The player
requested different maximum sail HP for different ships, visible in the ship
table and used by projectile damage. Existing normalized condition, repair,
weather/fire damage and saved hole masks retain their units.

The native and gameplay hooks are integrated into the canonical source after
the preceding gameplay correction batch released its shared inputs. Already
applied authoring diffs are recovery evidence under the ignored
`experiments/native-metal/.cache/fire-next/activation-recovery` directory;
they are not additional ordered build inputs. Installation remains pending.

## Owners

- `tools/sail_cloth_capacity.py` measures authored cloth corners in the actual
  GM hierarchy and generates the complete capacity table.
- `src/gameplay/PROGRAM/scripts/sail_capacity.c` owns the generated per-archetype
  capacity lookup, derived current HP and direct-hit HP-to-SP conversion.
- `PROGRAM/battle_interface/BattleInterface.c::GetRigDamage` admits projectile
  damage; `ProcessSailDamage` owns cumulative normalized damage and hole masks.
- `PROGRAM/interface/interface_utils.c::SetShipOTHERTable` owns the shared sail
  current/max cell used by ship, shipyard and transfer screens.
- Native `SAIL` owns physical holes, geometry and propulsion. Its next-batch
  correction must retain cumulative sub-hole damage when updating condition.
- Existing gameplay manifest/delivery and ordered native patches own runtime
  activation. A generated file or compiler probe does not prove installation.

## Capacity and units

All 57 active ship definitions were resolved: 55 sailing models, Boat without
cloth and Fort outside the sailing model hierarchy. Measure the actual cloth,
not hull dimensions, mast height or ship-class buckets. Compose authored GM
node transforms, then sum triangle areas from polygon-ordered `sail1..4` labels.
Native SAIL remaps its internal hard-point order; applying that order to the
authored polygon would measure a crossed surface.

The density anchor is intact Lugger cloth: 325.797168685 GM units squared
correspond to 100 HP. Capacity is the nearest integer of area times density,
with a 100 HP minimum for sailing craft. This keeps starting-craft resistance
while increasing capacity in proportion to measured cloth. Boat and Fort have
zero sail capacity. The complete numeric owner is the generated C lookup;
the extractor regenerates it from the intended PROGRAM/RESOURCE inputs.

`SP`, `RealShips.SP`, per-sail `dmg` and hole thresholds remain normalized.
Current HP is derived as capacity times condition; it is never another saved
health field. Capacity comes from the existing real ship's BaseType. No global
catalogue migration or capacity copy is needed for prior saves.

Only projectile damage is converted from raw HP to normalized SP after its
existing distance/perk/defence multipliers. Fire/weather remain exposure-based
normalized loss; repair remains normalized work with its existing materials.

## Damage admission gap

SP=100 did not mean every target behaved identically: current native damage
also clamps to a hit sail's propulsion share. A median Manowar cloth could die
from one knippel despite the remaining sails surviving. A separate contact-hole
floor can also charge more damage than the projectile admitted. Merely scaling
GetRigDamage leaves that floor and cannot meet the capacity contract.

Contact tracing must record touched cells as transient preference only, without
writing a provisional hole. The script admits damage against committed masks;
the native hole writer prefers the contacted cells when cumulative damage crosses
its threshold. This preserves prior holes and avoids removing an old hole from an
immune ship. Randomly deleting provisional excess, as in the existing weak
airburst correction, is rejected because it can erase prior damage. Keep
sub-hole loss in native condition updates. Falling masts retain their independent
loss.

## Acceptance and limits

Equal-calibre, equal-range hits must debit the same admitted HP on intact cloth
until its own remaining capacity clamps the hit. Whole-ship percentage loss
therefore decreases with total cloth capacity. Native Execute must not refund
sub-hole loss. Prior normalized damage/masks, partial repair and save/load must
retain their condition without healing or double conversion. All three shared
table consumers must show derived current/max HP; condition bars stay percent.

Both Dutchman variants contain one three-corner group named literally `sail`.
Native `SAILSTATE ss{}` defaults to triangular cloth; the absent type suffix does
not imply an uninitialized type. The integrated native parser safely preserves the
original group-number semantics, including typed names without a number and
underscore suffixes. All 513 measured cloth keys match the existing consumer,
without node/group collisions. Installed coverage still awaits delivery.

The old fire scan also skips short typed names. The integrated model imports
saved thermal records before adding
new cloth, then retain stable hull keys and exact node/group cloth keys. The
former index-based candidate is rejected because it relabelled an existing hot
hull record as newly supported cloth. The successor retains all old 48 records,
pending damage and missing-mast state through repeated loads. Manowar's authored
fire coverage increases from 4 to 19 cloth groups; its preserved hull budget is
44 and full CPU capacity is 63. Renderer records remain 32 bytes with a 48-point
limit; every sail reads its own authoritative char field.

Installed battle balance, native cumulative-loss/repair replay and table visual
acceptance remain pending integration of this next batch.

Activation is one combined native/gameplay batch. The repository model header,
shared headers and CMake ABI/include changes are canonical inputs. The ordered
native stack consumes `ship-damage-admission.patch`, `ship-hull-damage.patch`
and `ship-airburst-hull-proximity.patch` after `ship-fire.patch`; the existing
manifest/recipe owners compose the sail helper, hit adapter and shared table.
A content-only installation of the scaled hit adapter
with the old provisional-hole engine is not a supported intermediate state.
The canonical `run.sh --stage-only` must consume that final batch before a real
prior save can prove retained condition, repair, displayed HP and actual battle
behavior. The complete native build and prepared PROGRAM compilation pass;
neither result proves delivery or player acceptance.

The player explicitly requires preserved FPS. Accept the combined scene through
a matching before/after battle, including a dense broadside, several burning
ships and restoration of damaged ships. CPU or isolated-demo timings cannot
accept fleet FPS. The corrected hull component's 256-cut cold rebuild remains
unresolved at 19.263 ms; ordinary and additional-50-cut updates measured 0.932
and 2.334 ms respectively on the same Santisima1 object.

## Source evidence

The extractor covers all 57 definitions without missing model inputs. Generated
C and CSV agree on every capacity. An isolated real-compiler fixture passed 30
hit comparisons, prior cumulative damage/masks, immunity and two serializer
rounds with zero script errors. Those rounds are not an installed player's save.

The full installed PROGRAM copy, with the proposed include/hit adapter/immune
branch and shared table changes, also compiled with zero script errors. The
fixture renamed only the stock Main entrypoint, preserving its body, and used
the executable-relative shared headers required by the actual compiler.

A direct table call in the headless VM is rejected as UI evidence: it crashed in
`_XI_ConvertString`, whose native StringService is absent in that fixture. No
product or harness repair follows from an uninitialized inspection environment.
The table must be observed in the real installed interface after delivery.

The frozen native successor passes 755 state/cloth/projection CPU cases, 105
existing damage/guard cases, five translation-unit syntax checks and two real
VM SAVE-codec rounds preserving 770 fields with zero script errors. The codec
fixture provides its required OnLoad callback; it does not exercise the installed
game's complete OnLoad or battle.

A one-off production-Model comparison at O3 measured a dense 48-cell thermal
step at 9.499 microseconds and 63 cells at 16.345 microseconds, with zero hot-path
allocations. Twenty simultaneous steps total 0.190 versus 0.327 ms; the thermal
step occurs at 0.25-second intervals. Candidate hull plus volume projection took
6.312 microseconds and retained both 48-point limits. These are CPU-only bounds,
not whole-scene frame-time or FPS evidence.
