# Лунная бухта и тайник старой команды

## Status

October 13, 2026: source/VM/SAVE checks and canonical staging to the played app
pass for the current Great Recipe batch. Root owns
composition, native VM/SAVE probes, staging and installed delivery. The player
owns the actual landing, visible moon, exploration, combat, chest pickup and
return acceptance. Source inspection alone does not accept those scenes.

## Contract and state

After the powder task, the captain sends the captive to a moonlit bay to recover
his old ship's ledger. The player physically searches a shore chest for the key,
enters the crypt, defeats exactly two skeletons, opens the actual chest and
brings its ledger to the captain. The beach has no additional enemy wave.
An explicit launch choice admits Beard's combat help in the crypt only when
`GreatRecipe.CaptainRespect` is true from the existing captain practice. Solo
completion remains available; assistance creates no respect or king eligibility.

The phase remains `captive`; only `Task = shore` selects this chapter.
`GreatRecipe.Shore` owns `Active`, `Help`, `Seeded`, `VaultClear`, `Recovered`,
`ReturnPending` and `Delivered`. Existing `mutiny`, `mutiny_final` and `ready`
saves are never rewound, and generic quest success is never translated to trust.
`VaultClear` requires both real enemies to be dead. `Recovered` requires the
real ledger in inventory; only actual handover consumes it and advances mutiny.
No hit counter, probability roll, wait gate or calendar mutation is introduced.

The real cache contains the ledger, two stock `potion2` and one `jewelry5`.
It never contains the red pearl, recipe, expensive final reward or shop stock.
These are dream items inside the existing escrow: this chapter does not change
the final core restoration/reward contract.

## Owners and strongest reuse

- `tools/gameplay/great-recipe-shore.c` owns only this chapter, quest items,
  two cloned stock locations, quest-only actors and tagged combat damage.
- `tools/gameplay/great-recipe-shore-dialog.c` projects to
  `PROGRAM/dialogs/russian/GreatRecipeShore.c`, the boatman and helper dialogue.
- The existing core owns the ship rooms, captain practice, phase, escrow,
  journal, mutiny and every ending. Root owns its integration and patch hashes.
- Stock `Box_EnterToLocator` → `OpenBoxProcedure` → `LaunchItemsBox` remains
  the search/inventory transfer owner. `GreatRecipe_ShoreQuestBox` intercepts
  only the two new clone IDs at `SetLocationQuestBox`, preserving their saved
  inventories instead of generic respawn. `Seeded` writes cache contents once;
  a depleted chest, retreat or reload never duplicates loot.
- Stock `NPC_GenerateCharacter(..., "Skel1"/"Skel2", "skeleton", "skeleton", ...)`,
  `FantomMakeCoolFighter`, `LAi_SetWarriorTypeNoGroup` and `LAi_group_FightGroups`
  own actual enemy combat. All four installed `skel*.gm` character models exist.
  The two actors have bounded player rank 1–25, fencing 26–50, health 35–220
  capped from 55% of player max HP, `blade1` and no firearm. Their outgoing
  blade damage to the hero is capped at 18% max HP; ordinary actors are unchanged.
- The optional quest-only RedBeard copy uses existing captain fighter setup,
  joins `GreatRecipeShoreFriends`, and fights `GreatRecipeBones` in the crypt.
  It does not mutate or relocate the core captain or any original story actor.

Cloning the stock connected shore/dungeon is the strongest reuse: it supplies
actual geometry, path patches, doors and chests without altering ordinary
Bermuda quests. Attribute-only locators, a menu that substitutes for searching,
new geometry and a separate quest framework were rejected as unnecessary.

## Physical layout evidence

Read-only binary extraction uses the stock `rdf.h` 72-byte header, names plus
offset array, 56-byte material and 108-byte label records.

| Input model | SHA-256 |
| --- | --- |
| `Outside/Shores/Shore04/shore04_locators.gm` | `c9cd6a8b3db858a7ad254e421441cb71031a0692774ff48ffa341a25c58d2541` |
| `Inside/DungeonVault1/DungeonVault1_locators.gm` | `0bb48b6e9a9f6611c96a72ecd56d03fce8776c83cfb88232222fc1c16a17807c` |

| Clone and actual locator | Position / role |
| --- | --- |
| `GreatRecipeBay`, `reload/boat` | `(-10.385,-1.078,13.385)` landing and explicit ship return |
| Bay `reload/reload2_back` | `(25.624,1.986,-6.821)` stock door to dungeon; key required |
| Bay `box/box1` | `(32.690,0,4.380)` actual searchable key chest near eastern rocks |
| Bay `goto/goto5`, `goto/goto10` | `(-14.516,0.510,0.278)`, `(-13.120,0.679,-0.992)` boatman and optional Beard |
| Vault `reload/reload2`, `reload/reload2_back` | `(-41.701,-2.748,-14.327)`, `(-43.949,-0.259,-14.222)` entry and shore return |
| Vault `monsters/monster9`, `monsters/monster13` | `(7.408,-0.003,25.999)`, `(6.263,-0.111,32.437)` exactly two skeletons |
| Vault `monsters/monster30` | `(-20.152,-0.111,-17.585)` optional ally arrival |
| Vault `box/box2` | `(-2.042,0.623,37.001)` real cache in the farther stone hall |

`GreatRecipeBay` copies `Shore3`; `GreatRecipeVault` copies `Bermudes_Dungeon`.
Original stock locations, residents, contents and reload routes remain untouched.
All other clone exits are removed. The bay's boat returns to the existing
`GreatRecipeDeck/reload/reload_hold`; the vault returns to the bay's `reload2`.

## Integration contract

| Existing owner/anchor | Required integration |
| --- | --- |
| Script include/project | Include new shore source; project new shore dialogue |
| Catalog initialization and old SAVE item upgrade | `GreatRecipe_ShoreItems()` after the existing supply catalog initialization; add both item translations; stock `Items_FindItemIdx` retains already-saved item indices |
| Successful powder task, current core `TaskAdvance("mutiny", ...)` | Advance to `shore`, with a note directing the hero to the captain; no old-save rewind |
| Core captive captain dialogue, `Task == shore` | Solo launch → `GreatRecipe_ShoreStart(false)`; earned optional help → `GreatRecipe_ShoreStart(true)`; existing practice still available |
| Core captain handover dialogue | `GreatRecipe_ShoreHandOver()` requires physical ledger and real `VaultClear`; advances mutiny only on true |
| `GreatRecipe_LocationLoaded` start | `GreatRecipe_ShoreLocationLoaded(location)`; if still `GreatRecipe_ShoreActive()`, return before ordinary captive actor relocation |
| `GreatRecipe_OnLoad` before captive scene replay | `GreatRecipe_ShoreOnLoad()` rebuilds only Task shore rooms/items and pending landing; then normal location callback |
| Core deferred allowlist and saved-event rearm | Add `GreatRecipe_ShoreLand` and `GreatRecipe_ShoreReturn` to `GreatRecipe_DeferredFunction` and `GreatRecipe_RearmDeferred`; shore callbacks use only `GreatRecipe_Delay` |
| `GreatRecipe_ClaimDeath` before existing branches | `if (GreatRecipe_ShoreClaimDeath(index)) return true;` |
| `SetLocationQuestBox` before ordinary generation | `if (GreatRecipe_ShoreQuestBox(_location, _locatorName)) return true;` |
| Blade damage after existing Beard damage | `dmg = GreatRecipe_ShoreBladeDamage(attack, enemy, dmg);` |
| Core dream cleanup before restoring the phase/property | `GreatRecipe_ShoreCleanup()` removes only chapter actors/groups and active continuation |
| Weather presentation owner, if strict full moon is requested | `GreatRecipe_ShoreMoonScene()` is the exact active beach predicate; use a presentation-only override, never change calendar |

Item translations for the composition owner:

| ID | Name | Description |
| --- | --- | --- |
| `GreatRecipeCacheKey` | `Ключ от тайника` | `Ржавый ключ из ящика у восточных скал. Открывает проход к тайнику старой команды Бороды.` |
| `GreatRecipeCacheLedger` | `Судовая книга старой команды` | `Промасленная книга с невыплаченными долями и подписью Ратли под последней выдачей пороха. Борода приказал принести её в каюту.` |

Both use stock quest inventory presentation; root adds `itmname_` / `itmdescr_`
entries. New journal text reports only observed discoveries and immediate next
actions, with no advance ending or future chapter spoilers.

## Recovery and acceptance

The actual boat door and boatman dialogue always offer ship return, even before
finding the ledger. Crypt escape remains open during combat. Crossing the boat
door consumes the active excursion through `GreatRecipe_ShoreReturned`, hides
only its actors, and restores the normal captive scene. Unfinished two-enemy
combat resets enemy HP after retreat; cleared combat and depleted chests remain
cleared. Reload inside a fight preserves enemy HP/deaths rather than healing
or granting victory. At 20% hero HP the existing check-min-HP protection and
death bridge rescue him to the ship at at least 30% HP; it does not set victory.
`ReturnPending` persists the interrupted rescue. On load it rearms return,
including a save inside the shore; it never falls through to landing. A saved
pending landing in the cabin or on deck rearms landing instead. The deck's
location callback distinguishes that pending landing from a completed return.
Both callbacks wait for pause, interface, boarding and current-reload owners,
and a false `DoQuestReloadToLocation` result retries after 0.5 seconds through
the existing core `GreatRecipeDeferred` owner. Its saved generation rejects
stale events and removes each pending key before calling a function, permitting
self-retry. `DoQuestFunctionDelay` is not used by this module. Retreat cancels
both former shore continuations before scheduling return; completed return and
cleanup remove both keys. Late callbacks also check phase/active/return state
before moving, and landing already in the bay or crypt does nothing.

Decisive root VM/SAVE probes: prior valid powder state → shore launch → real
item transfer → two real actor deaths → ledger transfer → explicit ship return
→ captain handover. Repeat box load/return/handover must retain no duplicate.
Seed one injured/dead enemy in an active SAVE and retain it through reload;
seed pending defeat/return and complete only return. Nearest negatives: missing
key keeps the crypt door closed, one living skeleton keeps the cache closed,
missing ledger prevents handover, generic good dialogue/no captain win denies
help, old mutiny/ready saves remain unchanged, and an ordinary NPC/box remains
unmodified. No permanent test suite is added.

`QuestlockWeather = "23 Hour"` is a weather preset only: no hours/minutes
children call `SetCurrentTime`. Installed `WhrWeather` requests this preset;
the stock loader changes actual time only for those two child fields.
That proves no calendar mutation, not visible full moon. Stock `WhrSun` selects
its lunar texture from the actual date, and the native sky has a separate
presentation owner. Strict fullmoon presentation is therefore a root weather
integration dependency; a night preset alone cannot close it.

Root integration now consumes the exact lower-case `PROGRAM/weather/WhrSun.c`
owner. Only the active bay overrides the native SunGlow moon flag, stock Moon15
texture and glow size after the ordinary weather calculation; the calendar
remains untouched. The native renderer already reads these SunGlow attributes.
Visible moon and complete scene composition remain player acceptance checks.

Root also disables stock random monster generation on both clones using
`LAi_LocationDisableMonstersGen`: disabling kill-spawned fantoms alone does not
disable dungeon monsters. This preserves the requested two enemies. Opening
the crypt records actual key possession, so later storing the key cannot lock
the player out. The helper uses the stock 40-HP safeguard and withdraws to the
boat on a heavy wound; his scene copy cannot die while the main captain lives.

The source candidate compiles with the current native VM. Isolated state probes
cover supply consumption, single cache seed, one-live-enemy negative, return
retry, actual inventory handover, ordinary box/monster negatives and two SAVE
codec reloads. Presentation, reload and actor creation are bounded fixture
adapters; two enemy deaths are seeded state rather than played kills. The first
fixture's integer-only HP/damage literals produced return-type errors; valid
float inputs matching the game owner pass. These probes do not accept physical
combat, box UI or helper pathfinding. Installed delivery is accepted separately
through the canonical batch recorded in `docs/runtime.md`.

Remaining player scenarios: visibly moonlit complete bay/crypt scene, door and
chest access, solo and respected-Beard combat, escape/defeat/retry, in-combat and
post-loot save/load, boat return and actual captain handover. Build/VM source
proof and delivered-app state must remain separately reported.
