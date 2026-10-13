# Earned Pirate King alliance

## Status

October 13, 2026: source-only optional module. The root task owns integration
into `patch_great_recipe.py`, compilation, staging and installed-app replay.
No source inspection or VM result accepts this ending in the player runtime.

The integrated source/state probe and two native SAVE-codec rounds pass.
They retain real inventory, core fight-result bridges, stock relation writers
and shared production, with bounded world/presentation adapters. The `PIRATE`
constant must not be shadowed by a local `pirate` identifier: script names are
case-insensitive. Ordinary and active-story residence negatives, real release
requirements, tangible concessions and permanent refusal are covered; actual
boarding UI, dialogue, scene transitions and authored-story coexistence remain
player replay requirements.

## Contract

Earn Beard's respect through ship work, saving Diego, both loyal mutiny fights,
the actual optional captain practice and a won equal-sloop naval battle; spare
the defeated boarding captain, release him with the crew through the normal
prize panel, negotiate peace with Bermuda, and become his
close friend. Beard sits in the actual Bermuda residence as Pirate King, with
the original Jackman standing next to him and retaining all his authored work.
The title appoints a naval warleader: his demonstrated naval power and shared
ammunition recipe strengthen the common fleet. Jackman retains settlement
administration, diplomacy, contracts and all original quests. Lucky agrees to
peace for the living; neither restitution nor the title forgives his brother's
death.

There is no probability roll or extra fight-count grind. Repairing a mistake
and reconciling with Diego remains valid. Betrayal without reconciliation,
rebellion, sinking the final ship, killing the boarding captain, losing, and
the early class-2 victory retain the ordinary quest outcomes. The additional
ending does not rewrite those outcomes.

## Owners

- `tools/gameplay/great-recipe-alliance.c`: persistent
  `pchar.GreatRecipe.Alliance`, mercy, negotiation and shared alliance policy.
- `tools/gameplay/great-recipe-alliance-dialog.c`: the defeated captain and
  later Beard/Diego conversations; projects to
  `PROGRAM/dialogs/russian/GreatRecipeAlliance.c`.
- Existing `great-recipe.c` remains owner of naval battle, escrow, storm,
  ordinary loot and production. Its existing `GreatRecipe_TurnInPearl` and
  `GreatRecipe_StoreDaily` remain the only production path: seven game days,
  20–30 per day, stock at most 200, ordinary paid purchases. Alliance code
  creates neither ammunition nor another shop/production service.
- `PROGRAM/characters/init/Bermudes.c` owns the original `Jackman`,
  `Mayor\\Jackman.c` and his `Pirates_townhall` assignment. Neither his ID,
  dialogue file/current node, quest attributes, companion identity, inventory,
  nor storyline addresses are replaced by the alliance.
- Root is the sole owner of shared patch composition, Git, builds and runtime.

## State and integration calls

The optional tree advances `pledged` → `parley` → `king`; an ordinary lethal
outcome advances a pledge to `declined`. No migration fabricates missing
respect, loyalty wins, a naval victory, mercy or a treaty from generic `Won`.

Core calls, to be composed by the existing patch owner:

| Owner and exact anchor | Call |
| --- | --- |
| `GreatRecipe_FightFinish`, after final `won` is resolved and before task is advanced | `GreatRecipe_AllianceRecordFight(task, won)` |
| Ready captive Beard conversation, gated by `GreatRecipe_AllianceEligible()` | Explicit peace pledge → `GreatRecipe_AlliancePledge()`; keep ordinary Duel link |
| `GreatRecipe_BoardingFighter(chr, boarding_enemy)` call in the final boarding-stat owner, immediately after it | `GreatRecipe_AllianceArmCaptain(chr, boarding_enemy)` |
| `GreatRecipe_ClaimBoarding`, final `GreatRecipeDuelBeard` branch, before `GreatRecipe_Win()` | `GreatRecipe_AllianceNavalWin(GreatRecipe_AllianceSpared())` |
| `AIShip.c::ShipTakenFree`, before ordinary capture handling | `GreatRecipe_AllianceReleaseCaptain(rDead)` while the original captain is alive; then the normal claim bridge |
| `GreatRecipe_Win`, ordinary sink/death path, before its phase transition | `GreatRecipe_AllianceNavalWin(false)`; already-parley is unchanged |
| `GreatRecipe_Returned`, after normal locks/escrow cleanup | `GreatRecipe_AllianceReturned()` |
| `GreatRecipe_OnLoad`, before the `!GreatRecipe_Locked()` early return | `GreatRecipe_AllianceOnLoad()` |
| `locations_loader.c`, exact `LoginCharactersInLocation(loc);`, before it | `GreatRecipe_AllianceResidence(loc)` |
| Original Lucky and Jackman dialogue switch, new optional cases only | `GreatRecipe_AllianceAction(NPChar.id, "restitution")` or `"treaty"`; original links and node selection remain intact |

The mercy module owns a 500-ms event only while its final cabin captain and
pledge remain current. Each arm/load increments its saved generation; old
queued events, a different scene/clone or a dead captain stop. It uses the
existing HP checker at 40 HP; it does not
alter a boss, ordinary boarding fighter, or unqualified final captain. A
declined mercy uses `QuestAboardCabinDialogExitWithBattleNoParam()` and resumes
the existing lethal battle. Accepted mercy uses
`QuestAboardCabinDialogSurrender()`, which sets the stock one-shot surrender
attribute and completes through `LAi_ReloadEndFade` and `SHIP_CAPTURED`.
This is mercy, not release. The standard transfer panel can still imprison or
kill the captain: both options set the original actor's HP to zero. Only the
actual `ShipTakenFree` route admits `Alliance.Released`, with an exact live
`GreatRecipeDuelBeard`, the earlier mercy and earned eligibility. `NavalWin`,
resident appearance, treaty/friendship and captain preservation require that
release as well. The mercy journal explicitly directs the player to this
normal release action. A copied prisoner or a killed original does not qualify.

Important survival boundary: `AIShip.c::ShipTaken` currently executes
`LAi_SetCurHP(rDead, 0.0)` unconditionally. The integration owner must preserve
only the genuinely spared final Beard at that line with
`GreatRecipe_AllianceKeepCaptain(rDead)`; a generic captured ship or
killed captain must still follow stock behavior. This is separate from the
source proof that `LAi_ReloadEndFade` emits a live surrender. A character copied
after a lethal branch is not mercy.

The initial terms conversation frees the existing, living Diego at the verified
town locator and verifies actual freedom before Lucky or Jackman approves.
`PrisonerFreed` records this action; approval also resolves Diego's original ID
and checks he remains alive, outside captivity and in the neutral guest group.
An absent/dead prisoner or a flag without that state cannot qualify.

The same terms conversation hands over only the existing recipe quest item through
`GreatRecipe_GrantLoot(false)` when `pearl_search` is current and the item is
missing, before any political title. `RecipeShared` records the actual item or
already-started shared production, so repeat terms cannot duplicate a moved or
consumed item. It does not fabricate a barrel pickup or grant the ordinary barrel's
money/weapons bundle. The player then uses the same Lucky turn-in.

The hero can permanently refuse the political alliance from parley.
`GreatRecipe_AllianceDecline` sets `declined` and hides only the owned royal
actor. Actual freedom, any already-given recipe, any paid restitution and
ordinary pearl turn-in/seven-day production remain intact. Declining before
concessions does not pretend those actions happened. Repeated refusal cannot
reopen the ending or generate another reward.

For refusal hooks, call `GreatRecipe_AllianceRefuseCapture(city)` before a
player-selected takeover mutates the town. The helper protects Bermuda
(`Pirates`) and current pirate settlements regardless of the hero's flag.
It reports the hero's in-character refusal. Integrate both normal fort/naval
and alternative residence/town takeover entry points. Do not blanket-return
from authored quest combat or nation-driven storyline occupation callbacks.

## Ambient relations and main-story boundaries

`GreatRecipe_AllianceRelation(nation, ordinary)` computes friend under actual
`pchar.nation == PIRATE`, neutral under any other flown flag. `BaseNation` and
the patent returned by `GetBaseHeroNation` do not own the flown flag.

`GreatRecipe_AllianceActorRelation(actor, ordinary)` applies that policy only
after authored character-pair relations and `relation.UseOtherCharacter` have
been resolved. `AlwaysEnemy`, `QuestShip`, quest actors and locked sea groups
retain their authored relation. `GreatRecipe_AllianceLandRelation` affects
only the ordinary pirate-citizen/player relationship; an explicit enemy
relation and every other group remain unchanged. Alarm/quest attacks are not
cancelled, no quest relation arrays are erased, and quest groups are never
assigned a new task by this module.

Source integration must bind flag changes to the existing sea/group and land
consumers, including the pair relations cached during `Sea_Login`. Updating
only `NationsRelations2MainCharacter[PIRATE]` once is insufficient; blanket
overriding every nation or group relation can disable authored battles.
These consumers require the root's combined VM probe and player flag replay.

At the exact ordinary `Sea_Login` cache-write boundary, call
`GreatRecipe_AllianceRegisterPair(iCharacterIndex, iRelation)` after the
existing `SetCharacterRelationBoth`. Before any subsequent explicit
`SetCharacterRelation(first, second, relation)` write, call
`GreatRecipe_AllianceRelationWritten(first, second)` to revoke this marker.
In `GetRelation`, after `UseOtherCharacter` and same-actor resolution but before
the cached character-pair return, apply `GreatRecipe_AllianceHasPair` and
`GreatRecipe_AlliancePairRelation` to the nonhero actor. The saved ordinary
value remains separate from the dynamic friendship. A later explicit write,
including an enemy value equal to an earlier cache, still revokes admission.
An empty `quest` attribute is allowed; a nonempty value or child tree is not.

The stock `Flag_PIRATE` sets `PIRATE_CITIZENS` to neutral, while the other flag
commands do not reset that group. The integration owner must replay the
ambient flag change at its actual owner without erasing authored hostility or
alarms. In particular, `Mayor/Jackman.c` line 240 explicitly makes that same
group hostile during `KillLoy_toJackman`, and `LAi_group_Attack` changes alarm
state directly through a native message. A blanket friendship reset fails
these main-story and attack negatives. The five existing `Flag_*` functions
and native `AISea::UpdateRelations` are the flag/refresh owners.

Installed native `location/src/characters_groups.cpp::ProcessMessage` exposes
actor `IsEnemy`, setters, alarm/attack and save/restore messages, but no group
relation getter. `SaveData` rewrites the entire relation snapshot; it is not
a bounded getter. Two normal saved script globals therefore track the
authoritative ordinary group writes, including writes before the alliance
exists. They do not create `pchar.GreatRecipe` for ordinary new games.
`GreatRecipe_AllianceRefreshLand()` runs before each flag command's existing
`UpdateRelations`, after load and after friendship. It refreshes only known
friend/neutral groups with no active or nonzero player alarm. Explicit enemy
and absent prior-save metadata remain untouched until an authoritative
ordinary relation write establishes their state.

The discovered generic naval/land takeover paths converge on player choices in
`Common_Mayor.c::City`, `City_patent` and `City_nation`: guard those cases
before their first mutation. `City` queues `Residence_Captured_Any`;
`City_nation` calls `SetCaptureTownByNation`. The apparent
`Town_Capture_1` link is commented out, not a live alternative path. Do not
blanket guard `PrepareTownBattle`, `SetCaptureTownByNation` or
`Capture_Forts`: authored main quests call the same fort/battle machinery.

## Residence discovery

The installed `RESOURCE/MODELS/Locations/Inside/residence05/`
`residence05_locators.gm` has SHA-256
`d7d35bbd80223bb5690923e754e95beda8cf381714fcb93aff1082ae6a314f69`.
The engine `geometry/src/rdf.h` defines its 72-byte header, 56-byte material
and 108-byte label records. Read-only extraction found:

| Real locator | Position |
| --- | --- |
| `sit/sit1` | `(2.038, 0.181, 0.750)` |
| `goto/goto6` | `(1.166, 0.000, 0.989)` |
| `goto/governor1` | `(0.172, 0.000, 0.847)` |

`goto6` is approximately 0.904 metres horizontally from `sit1`; it is a real
adjacent locator. Jackman uses it only while his current state is ordinary
`huber` at the original seat, or the module's own `huberstay` pose at `goto6`.
The existing `LAi_SetHuberStayTypeNoGroup` preserves his governor role and
current faction/group. Its main locator is `goto6`; empty window/map alternate
locators keep him adjacent. `RPGUtilite` classifies both governor types as a
citizen; the seated-only `LAi_dialog` branch owns seated animations. The
officer-tax dialogue's seated-type checks do not apply to Jackman's original
mayor dialogue. A different
actor in the seat, a companion, another location, or an authored actor type
suppresses Beard's residence appearance instead of moving the story actor.
All geometry and visible separation remain subject to player screenshot QA.
The friendship dialogue queues a single ordinary residence reload on exit;
the loader applies the two new poses before character login. The reload's
pending marker is consumed before the transition and does not repeat on visits.

The actual `KillLoy_toJackman` branch suppresses Beard's residence appearance
until the original story leaves that state. New Beard/Diego actors use the
separate neutral `GreatRecipeAllianceGuests` group: Jackman's authored attack
against all `PIRATE_CITIZENS` must not gain extra immortal alliance actors.
Jackman and every original group's identity remain unchanged.

The freed, existing quest-only Diego uses the actual town `goto/goto1`;
`Town_IslaMona/Town/IslaMona_locators.gm` SHA-256
`1b6f639f8734fe8834bd5d549da415e2ac95eafba22fae5cde91d0c4dd8577da`
contains that locator at `(-16.257, 0, 44.527)`. His original identity is reused;
the original town residents and storyline actors are untouched.

## Read-only base identity

These SHA-256 values identify LF-normalized read-only portable inputs, not
current patch outputs or installed evidence:

| `PROGRAM/` relative source | SHA-256 |
| --- | --- |
| `nations/nations.c` | `5e512520468f2482588ca849ba10d391d4c6b905dab4868d628ed009abced31c` |
| `Loc_ai/LAi_groups.c` | `a19ccce46c33974727abeb687694ce9af93282a0e93042ea86589e255ba41470` |
| `locations/locations_loader.c` | `3770affd92a8d7927f33b573a98d26a80f6d4d042561fcaf4cebe2f5be7e4c9f` |
| `dialogs/russian/Mayor/Jackman.c` | `58d50e63f43c27cb9b87bcd3913c5584dfb8504b5d8df5980d111a00a87e157a` |
| `dialogs/russian/Common_Mayor.c` | `5ddfbeeff5cf36e53a578b74d4d197631869bb52fbb3b3c985be137a915665e3` |
| `scripts/colony.c` | `2a05aaa37c8ea26e83abcebfb9a268132ae0630a8f765d85294acbbfba357bb2` |
| `interface/interface.c` | `7eefc35ab68cc2e622333e8555ffffda93180267435beb231facf18646ba332c` |
| `Loc_ai/LAi_boarding.c` | `5367e7fbd5b131c8f7a43592ae953c82578fdeb60fd1f93edb7a55c72d9dd7d0` |
| `characters/QuestsUtilite.c` | `bea1955c7e95618ad98a41191aade92eec2072fb784dfde8089bca33d89e9b36` |

## Acceptance and unresolved scenarios

Source/VM acceptance needs the complete path seeded from prior valid states:
loyal earned captain respect → actual surrender/release → actual Diego freedom
and recipe handover → one paid restitution →
Jackman's assent → friendship. Probe saved/reloaded pledge, cabin mercy,
parley and royal residence; repeated talks must not deduct money twice or
repeat rewards. Ordinary kill/sink/early win/loss/pearl production must remain
valid. New and prior ordinary saves must still load without alliance fields.

Required independent negatives: authored `AlwaysEnemy`/quest/group attack
remains hostile; Jackman's exact dialogue/current node and companion exit
remain reachable; nonpirate takeover remains allowed; pirate takeover under a
foreign hero flag is refused; changing only BaseNation/patent must not change
ambient friendship; no medicine/practice/mercy proof must not crown Beard.

Player replay remains unresolved: tough practice and fair naval boarding,
visible mercy conversation, stock return, real paid restitution/treaty,
Beard seated with Jackman visibly beside him, direct visits to both original
Jackman quest chains, sea and land flag transitions, the two takeover routes,
and actual production seven days later with finite shop stock. No new
permanent test harness belongs to this module.
