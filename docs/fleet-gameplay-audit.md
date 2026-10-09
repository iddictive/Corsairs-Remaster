# Fleet gameplay and progression

## Status and delivery owner

October 3: the completed treasurer layer is installed through the canonical
stage-only path. All fourteen manifest pins match both the cache and played app;
deep/strict signing passes. The played engine remains `a743bc4a`; all 197 recorded
player save/configuration files retain their pre-install hashes. Native game
acceptance still belongs to the player; no player game was launched.

The earlier boarding correction is installed: two choices, free Esc cancellation,
ordinary click-to-exit, and removal of the temporary crew speaker after dialogue.
Closing a chest also restores the hero's player type without reading unloaded
interface globals.
Treasurer first-open selection is corrected too: read the scroll index after
native initialization, which turns its provisional `-1` into a valid selection.

`tools/metal_fleet_gameplay.py` and `tools/gameplay/fleet-gameplay.json` own this
layer. `tools/gameplay/treasurer.c` owns the shared treasury API appended to
`characterUtilite.c`; `treasurer-interface.c` and `treasurer-interface.ini` own the
native management window. Source is UTF-8/LF; runtime scripts use UTF-8/CRLF and
the interface INI uses CP1251/CRLF. `tools/sync_metal_gameplay.py` composes the layer
after shared gameplay, living-Caribbean and the existing loader background-alpha
correction. Ignored `gameplay/` scripts are only working copies.
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
| Treasurer management | `interface/GoodsTransfer.c`: one officer reply opens two tabs, ship-specific total cargo targets and cabin sale rules/preview. The initialized native scroll immediately selects the flagship; changing ship refreshes its own targets. Numeric input binds the original ship/good; Esc closes input before the window. Clearing targets preserves disable/contraband flags. |
| Fleet purchase ledger | `characters/characterUtilite.c` and `interface/store.c`: one shared wallet ledger includes character index zero, charges each line immediately and observes reserve money, real shop stock, cargo capacity and per-ship disable/contraband rules. Partial packs are never free. Automatic purchases never sell excess cargo; the existing explicit manual excess-sale setting remains. |
| Cabin junk sale | `characters/characterUtilite.c`: a real available friendly storekeeper receives exactly the sold stock. Categories, exclusive base-price threshold, per-item retained quantity and explicit keep locks share one preview/execution filter. Quest, rare, unique, unknown, equipped and useful equipment is retained; medicine, gems, maps, books and amulets are outside the sale set. Chest money and personal inventory are untouched. Normal Commerce/perk pricing applies. |
| Automatic port service | `locations/locations_loader.c` and `characters/characterUtilite.c`: separate opt-in sale/purchase settings, no default writes or transaction during save rehydration, stale callbacks cancelled by generation/location, bounded retries while dialogue or UI is busy. Normal shop door/night/quest/NPC restrictions apply. Sale precedes purchases; one accumulated time charge includes five minutes for an actual sale and one per cargo line. No frame transaction loop. |
| Boarding loot order | `Loc_ai/LAi_boarding.c` and the officer dialogue: one conversation offers collection or manual inspection after the current enemy cabin is loaded and filled. Collection moves every item and cash from the cabin chests to flagship `box1`, including quest, unique, rare, gold and unknown item IDs. Exact item counts are stored without personal pickup, gold conversion, inventory limits or duplicate suppression. Either reply closes immediately; Esc uses the manual-inspection exit. The temporary speaker is removed and the ordinary exit control returns. Chests, bodies, combat and active interfaces retain their own clicks. Leaving after manual inspection preserves remaining protected items in the flagship chest; personal inventory and wallet are untouched. Source removal follows verified storage; repeated completion cannot duplicate items. A missing speaker collects the full cabin chest contents before the original capture path. Fort boarding retains its existing path. |

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

All fourteen manifest transforms are byte-exact, idempotent, and reject unknown input.
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

The treasurer completion passes 34 transaction/protection checks, 22 native UI
state/event checks and 12 automatic-service checks, with empty error logs. UI
checks use the actual XInterface, normal launch and unload paths, carousel,
checkbox events, quantity buttons, both tabs, popup cancellation and full exit.
Automatic checks cover a real queued arrival event, busy dialogue/UI, old-save
load suppression, stale/departed callbacks, exact buyer/cargo conservation,
repeat callbacks and the accumulated service charge. Fixture boundaries are
seeded town/merchant/ship/item state; external combat/hostility/service/time/XP
queries are instrumented. The real world-time advance remains player-owned.
No screen composition or real player input replay is claimed.

Player logs and native replay also reproduce a cross-segment compiler collision:
loading and unloading the old GoodsTransfer registers generic `Add`, after which
shipyard's local `add` fails at line 1068; ship cannon controls and tavern code
fail at their `add` parameters. All three compile clean in a fresh VM. Treasury
quantity callbacks now use the `TreasurerUI_` prefix, preserving the button
events. After the corrected treasury segment unloads, all three compile with
an empty error log. A cold isolated compile alone missed this lifecycle boundary;
changing the three unrelated consumers was unnecessary. The automatic callback's
UI busy guard also requires `sti(InterfaceStates.Launched)`; the unconverted
attribute failed the real busy-interface negative before this correction.

Player replay remains: automatic HP correction on load; treasurer ship selection,
configured port auto-sale/purchase, shipyard opening after treasury close, and a
real sale versus refusal at sea; a paid fleet morale transaction; a daily
repair with scarce materials; a dry companion entering combat; immediate and
late surrender, captain victory, a quest letter, a missing boarding speaker and
two consecutive captures. Canonical stage-only has passed for this batch.
No automated player-game launch is required.

## Treasurer button geometry and ordinary loot

`Treasurer_ItemSaleCategory` is the shared preview/manual/automatic category owner.
Alongside blades, guns and armor, `SellLoot` accepts the ordinary `jewelry*` and
`mineral*` families, including emeralds. This source revision replaces the older
gem exclusion above. Existing price, rarity, quest/unique flags, retained quantity
and explicit keep locks still apply. Only flagship `box1.items` supplies a sale;
personal inventory is never a fallback. Supplies and amulets stay outside it.
The sale table omits non-sale families; protected equipment remains visible with
its refusal reason. Turning a category off preserves its rows and stock.

Authored treasury buttons and the ship's morale/chest buttons use the existing
normal UI font and explicit offsets inside their unchanged rectangles.
`TEXTBUTTON2` places text at `rect.top + strOffset`; it does not vertically center
it. `FORMATEDTEXT` clears vertical alignment on every `SetFormatedText`, even if
the INI declares `valignment`. Reapply node message 5 after the treasury tab text
and the final shared-chest tab label. Changing default engine geometry would
also alter unrelated original controls, so these fixes remain at their authored
consumers.

The isolated native VM verifies emerald/mineral sale quantities and exact
chest/merchant/wallet conservation, personal inventory identity, repeat-sale
refusal, disabled-category/keep/quest/price protections and unchanged medicine,
amulets and unknown stock. World service availability, XP/time and notification
queries are fixture boundaries; player-save interaction and rendered alignment
still require the installed app replay recorded in `docs/runtime.md`.
Ten actual XInterface state/event checks also pass: first-open flagship, valuables
preview, omission of non-sale families, category off/on, keep/unlock recalculation,
unchanged preview stock/money, return to purchases and full interface unload.
The normal widget initialization runs at 800×600; this proves state and lifecycle,
not captured screen composition. The fixture reuses the established seeded world
queries; a delayed game-time exit cannot fire while this interface pauses time,
so it closes via the normal cancel event before the queued process-exit check.

## Treasurer row activation and readable stock

Native tables emit `TableActivate` with a zero-based row. The treasury registers
and removes its own handler, converts to the one-based table row, and acts on the
clicked item rather than a previous selection. Purchase activation opens its
target editor; chest double-click toggles only the existing explicit item lock.
Single-click remains selection and neither activation performs a transaction.

The three purchase checkboxes share one baseline; cash reserve and actions share
one footer row. Sale preview separates `Останется` (chest quantity minus preview
sale quantity, always visible) from `Особенности` (the existing restriction or
ordinary-item status). The global retained-copy rule and all protection filters
remain unchanged. `Защитить от продажи` / `Снять свою защиту` toggles the manual
lock without claiming that rarity, quests or useful equipment become sellable.

October 3: the closed-game canonical stage delivered this follow-up. The earlier
row-event fixture passed before the player stopped additional checks; the final
layout/copy revision has no new runtime replay. Player input and visual acceptance
remain separate from installation.

## Treasurer default retention

Ordinary eligible chest items default to zero retained copies. An explicitly
configured `Treasury.KeepCount` still applies; quests, rare items and manual locks
remain independent exclusions checked before the quantity rule.
The sale preview, quantity editor and shared sale calculation use the same zero
fallback. No saved setting is overwritten.

October 5: the latest three campaign saves have no `Treasury.KeepCount`; the
previous fallback of one caused the remaining junk copy without any player
setting. Installed delivery and replay disposition are recorded in `docs/runtime.md`.

Equipment retention is the player's choice: treasury does not infer need from
officer equipment or unused upgrades. It sells only the shared chest, never
personal/officer items. The final preview column shows rarity. The October 5
player capture reopened the earlier default-reserve fix and exposed the separate
equipment veto.
