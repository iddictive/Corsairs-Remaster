# Random NPC generation

Status: source/isolated VM accepted October13; installed activation and player
interaction pending under the shared Phonk installation window.

## Contract

The user requests independently varied starting experience, role-specific skills
and eligible perks, useful equipment, actual pocket money and carried items.
Experienced opponents can appear early; player progress can influence the spread
without defining every opponent. Merchants favour trade, and occasional unrelated
competence remains. Early farming must not unlock top equipment; rare late rewards
remain reachable. Broken items and omitting unavailable loot were offered
alternatives, not requirements or permanent prohibitions.

Keep rank as the existing aggregate of actual skills, ordinary NPC expiration,
persistent rival/officer progression, authored explicit ranks, boarding encounter
and crew contracts, and special quest/officer loot. Saved people are not rerolled
on load. Initial population variation does not add a world-aging simulator.

## Owners

`characters/GeneratorUtilite.c` owns independent rank sampling, role skill budgets,
profile identity and pocket cash. `scripts/utils.c` creates phantoms once;
`scripts/officers.c` supplies existing profession modifiers. Random guard callers
use the same profile instead of overwriting it from the hero.
`characters/RPGUtilite.c` copies profile identity and clears stale destination data.
`chr.rank` is the sole rank value; the profile does not mirror it. The ambient
role classifies the population; `quest.officertype` owns officer profession and
takes precedence through the shared role resolver.

`Loc_ai/LAi_equip.c` consumes `Items` and `ChrPerksList`: their definitions own
item quality, rarity, generation and role goods, and perk prerequisites/professions.
No second item/perk catalogue is maintained. `Loc_ai/LAi_utils.c` snapshots remaining
marked-random pockets at death and owns progression-based gear admission.
Persistent rivals use the same weapon selector while retaining their progression.

Mixture weights, bounded player influence, role wealth and loot admission numbers
are implementation balance choices, not user-specified numbers. The first policy
retains ordinary rank5/good rank15 and makes excellent gear reachable at rank30
with a low positive chance; the executable owns the exact policy.

## Delivery and acceptance

Order: complete sources -> full delivered-PROGRAM VM and saved-state checks ->
source checkpoint -> content-only installed activation -> player interaction.
New sources remain unregistered during other installation windows so unfinished
NPC inputs cannot enter their frozen batches. Source/VM success does not prove
combat, officer hiring or corpse-looting behavior.

Seed existing state. Verify role competence and rank math, independent low/high
experience, NPC rather than hero Intellect, authored/boarding negatives,
prerequisite-complete registry perks including a newly added eligible entry,
compatible equipment/ammunition, remaining cash/consumables, early gear rejection
and positive late excellent admission. Body reopening cannot recreate taken
contents. Save/load and character copy retain profile/inventory; copying an
unmarked source clears stale profile identity.

Real scenes require a new citizen/trader, guard, recruitable officer and captain,
then consuming an item and looting a corpse, plus an unaffected authored quest
opponent. Missing player-control replay stays explicit rather than inferred from
compilation.

## Source evidence — October13

The actual sync/recipe projection includes all nine promoted complete sources.
The real script compiler runs `NPCGenerationCases=0`, then two native codec
rounds with `NPCGenerationAfterLoad=0`, `script_errors=0`. The fixture follows
ordinary item startup (`InitItems`, `GenerateGenerableItems`) and perk startup;
base generable items alone cannot equip under the existing inventory contract.

The rank1-player sample spans rank1–50, with2934 of20000 draws at rank15 or
higher. Merchant/soldier/cannoner budgets, NPC Intellect, explicit authored ranks,
boarding inputs, officer price/loyalty, new registry perks and HeatedShot pass.
Compatible musket gear at rank1, women’s existing animation restriction, no gun
without ammo, repeated equipment/pocket conservation, remaining medicine/cash,
early spare-gear rejection and exact generated late excellent transfer pass.
Character copies clear a source corpse marker; unmarked copies clear stale
profile identity. Codec rounds retain the profile and depleted live pockets.

Only fixture Main/OnLoad entry points are replaced: real OnLoad requires scene
entities and is not accepted from this entity-free probe. Real player SAVE,
startup upgrade and combat/hiring/body scenes remain untested. Authored special
rewards, officer inventory, PGG SaveItemsForDead and GhostShipCrew retain their
existing paths; no generic broken-item or repair subsystem was added.
