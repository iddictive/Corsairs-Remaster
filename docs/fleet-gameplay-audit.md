# Fleet gameplay and progression

## Status and delivery owner

October 2: reviewed source, native script checks and the canonical ARM64 build
and staging pass. Installed scripts, engine and aiming techniques are updated;
the application passes deep, strict signature verification. Native interaction
acceptance belongs to the player; compilation and state fixtures do not accept it.

The following boarding correction is verified in the native script VM and awaits
delivery after the player closes the game: two choices, free Esc cancellation,
ordinary click-to-exit, and removal of the temporary crew speaker after dialogue.
Closing a chest also restores the hero's player type without reading unloaded
interface globals.
Treasurer first-open selection is corrected too: read the scroll index after
native initialization, which turns its provisional `-1` into a valid selection.

`tools/metal_fleet_gameplay.py` and `tools/gameplay/fleet-gameplay.json` own this
layer. `tools/sync_metal_gameplay.py` composes it after the shared gameplay and
living-Caribbean layers. Ignored `gameplay/` scripts are only working copies.
The manifest binds exact before/after hashes and audited previous deliveries;
unknown edits fail closed. Canonical delivery remains
`experiments/native-metal/run.sh --stage-only`, including installed-app signing.

## Behavior contracts

| Behavior | Contract and script owner under PROGRAM |
| --- | --- |
| Hero HP | `characters/RPGUtilite.c`: base `50 + P + E`; each rank adds `6 + ceil(E/3)`, using permanent attributes. NPC curves remain unchanged. `HPPlus` retains its normal `+rank` bonus. |
| Existing-save migration | `Loc_ai/LAi_character.c`: one additive compensation against the original `floor(2 + E*0.55 + 0.5)` growth curve, recorded by `Progression.HPCurve`. Stored extra HP stays intact, wounded fraction is preserved, repeated reads/F12 give no extra HP or healing. New games mark the new curve during initialization. Removed perk bonuses are cleared once. |
| Officer capacity | `characters/RPGUtilite.c`: `min(64, 10 + floor(rank/2) + floor(base Authority/10) + A)`. Rank and trained Authority grow capacity; temporary illness and navy penalties do not shrink it. Wages, loyalty and officer roles still apply. |
| Companion ammunition | `sea_ai/AIShip.c`: a dry selected type can switch before the first volley. Availability requires rounds and powder for an intact bort. Loaded borts above 80% and an ongoing fallback reload are preserved. Native reload, consumption and enemy policy are retained. |
| Fleet carpenter | `characters/characterUtilite.c` and `battle_interface/BattleInterface.c`: a stronger flagship repair skill assists by averaging with the captain's own skill. Actual boards/sailcloth come from the existing common reserve; fractional leftovers are conserved. Locked ships and travellers retain their own materials. Existing repair timing and costs remain. |
| Brothel morale | `dialogs/russian/Common_Brothel.c`: costs `30 * eligible fleet crew`, then adds 10 morale to that same fleet. Locked ships are excluded; no money means no bonus. |
| Treasurer carousel | `interface/GoodsTransfer.c`: changing ship refreshes its cargo and purchase targets. Treasurer name and portrait consistently use the flagship's valid treasurer. |
| Cabin junk sale | `dialogs/russian/Enc_Officer_dialog.c`: only a present, available storekeeper buys it. Cheap common equipment uses the normal Commerce/perk sale modifier and transfers to that merchant. Quest, rare, unique, unknown and useful equipment is retained; medicine, maps, books and amulets are outside the sale set. |
| Boarding loot order | `Loc_ai/LAi_boarding.c` and the officer dialogue: one conversation offers collection or manual inspection after the current enemy cabin is loaded and filled. Ordinary items go to the flagship chest; money goes to the hero. Either reply closes immediately; Esc uses the manual-inspection exit. The temporary speaker is removed and the ordinary exit control returns. Chests, bodies, combat and active interfaces retain their own clicks. Leaving preserves protected items through normal inventory transfer, with unaccepted or unknown IDs retained in the own chest before source removal. Repeated completion cannot duplicate them. A missing speaker falls back to protected-loot preservation and the original capture path. Fort boarding retains its existing path. |

The earlier direct development copy reverted parts of the already merged
white-flag/crime/capture behavior and field-repair scaling. This layer was rebuilt
on the canonical reviewed package, preserving those fixes rather than restoring
the stale files. Existing payroll/debt, common food/rum/medicine and cabin officer
auto-supply belong to their existing layers and are not new features here.

## Evidence and remaining acceptance

The native VM compiles the exact final core plus Debuger, officer/brothel
dialogues and GoodsTransfer with an empty error log. Seeded HP checks cover a
wounded rank-18 hero (125 to 196 maximum, 50% wounds retained), HPPlus, repeat
reads/F12-equivalent migration, perk removal and an unaffected NPC.

A second VM fixture checks shared material consumption, fractional leftovers,
exhaustion without using a locked donor, a dry first charge, no reload thrashing,
loaded-bort preservation, missing powder, exact unknown-loot conservation,
repeat completion and an invalid cabin boundary. Geometry/readiness, the native
reload dispatch and cargo-load refresh are abstracted in that fixture; actual
scene and reload timing still require player replay.

All eleven manifest transforms are byte-exact, idempotent, and reject unknown input.
The native engine builds as ARM64. The merged fog fix passes actual Metal pixel
checks; the merged manual-aim change passes 758 geometry checks with no failures.

Player replay exposed a blank officer/boarding dialogue: its duplicate trading
extern failed after the shop script was loaded. The layer removes that redundant
declaration and reuses the main program's registered function. A native fixture
now seeds the loaded shop, invokes the production dialogue wrapper, and verifies
the loot choices on first load and after reload in separate event frames,
plus refusal of a junk sale at sea. It exits with an empty error log. Only cabin
entity readiness is abstracted; the earlier isolated compile missed this context.

The player's next replay exposed a second defect: manual inspection removed the
reload handler permanently, while the regular talk path is disabled during
boarding. The correction restores that handler after conversation and lets the
original exit preserve protected loot without reopening the dialogue. The native
fixture checks two choices, Esc's exit node, speaker cleanup, chest/body/fight/
interface click isolation, and two successive exit events with exact unknown-item
conservation. It also loads the chest interface after the shop and officer dialogue
with an empty error log. Scene geometry, native fader, dialogue rendering and its
close operation are abstracted; actual input replay remains player-owned.

The player's chest error log also exposed a separate unload-lifetime defect in
`interface/itemsbox.c::IDoExit`: both uses of `sFaceID` after `EndCancelInterface`
read an already-invalidated global. The correction snapshots the cabin-menu
condition in a local before unloading. A native fixture reproduces the original
seven errors and retained actor type, then verifies ordinary chests restore player
type with no errors while the cabin-menu path preserves its previous type. The
compiler's actual segment unload is exercised; pre-exit UI bookkeeping and native
UI teardown are abstracted.

Player replay showed a blank captain and zero cargo until the treasurer carousel
was switched. The script read the provisional scroll index before
`MSG_INTERFACE_INIT`; the native scroll owner normalizes it during that call.
Moving the read after initialization fixes that ordering. A native VM data fixture
reproduces the old `-1` first read and verifies first-open flagship cargo/orders,
then companion cargo/orders with the flagship order unchanged. Native widget
creation and presentation are abstracted against the installed scroll contract;
the player's cold-open screen remains the accepting surface.

Player replay remains: automatic HP correction on load; treasurer ship selection
and a real sale versus refusal at sea; a paid fleet morale transaction; a daily
repair with scarce materials; a dry companion entering combat; immediate and
late surrender, captain victory, a quest letter, a missing boarding speaker and
two consecutive captures. Canonical stage-only must pass after this final batch
before any installed-delivery claim. No automated player-game launch is required.
