# Fleet gameplay batch audit

## Status

October 2, 2026: installed content confirmed; feature acceptance unresolved.
This is a verification result, not a repaired gameplay batch. The preceding
completion claim is not supported by the implementation below.

## Coverage and evidence

All 142 files in the ignored development `gameplay/` directory match both
the Metal cache and installed remaster resources byte for byte. The nine
behavior owners below were reviewed directly. The installed engine compiled
the exact copied PROGRAM core plus Debuger, the officer/brothel dialogues,
and GoodsTransfer in a disposable VM; exit 0, empty error.log. The fixture
exited before game initialization and used separate userdata. This accepts
compilation, not the player's actions or save.

| Requested behavior | Owner under PROGRAM | Finding |
| --- | --- | --- |
| HP progression | characters/RPGUtilite.c; Loc_ai/LAi_character.c | New base/growth and HPPlus multiplier are present. The actual baseline growth was `makeint(2 + E * 0.55 + 0.5)`, not the formula quoted in the earlier explanation. |
| Current-save HP correction | Debuger.c | F12 writes the base/rank total while retaining bonus_hp; an existing HPPlus bonus is subtracted from that total on the next maximum-HP read. The operation also fully heals and overwrites existing HP adjustments. It is not a general safe save migration. |
| Officer limit | characters/RPGUtilite.c | GetSummonSkillFromNameToOld already divides by 10. Dividing again gives an Authority bonus of 0 for skills 1–99, and 1 at 100. |
| Alternative ammunition | sea_ai/AIShip.c | Fallback is gated by BOAL_ReadyCharge == 0, normally set after a side fires. An empty selected type can remain stuck before that transition. One remaining projectile also does not satisfy the engine's intact-cannons-per-bort threshold. |
| Fleet brothel | dialogs/russian/Common_Brothel.c | Charges 30 per crew member over the active companion list and adds 10 morale to each crew. Locked companions are included. Paid fleet behavior is present; interaction replay is pending. |
| Post-boarding cabin loot, selected idea 1 | Loc_ai/LAi_boarding.c | Surrender calls the transfer before the current cabin boxes are materialized; ShipCabinLocationId can refer to the reused cabin. QUESTITEMS are left in temporary boxes which are reset on later boarding, so the filter alone does not guarantee preservation. No optional looting order was added. |
| Flagship carpenter, selected idea 2 | characters/characterUtilite.c | Skill averaging and the Carpenter perk are shared, including locked ships. The daily consumer still spends each ship's own boards/sailcloth; the promised common material reserve is absent. |
| Treasurer junk sale, selected idea 4 | dialogs/russian/Enc_Officer_dialog.c | Reads cabin box1 and conservatively filters cheap equipment. It creates proceeds at a fixed 40% without a market/location gate or Commerce calculation. Quest/map/book exclusions are present; no demonstrated sale of a unique item was found. |
| Treasurer ship carousel | interface/GoodsTransfer.c | Scroll refresh was added, but the name uses the flagship treasurer and the portrait uses the selected captain's unchecked treasurer field. |

The existing cabin officer auto-supply/common food-rum-medicine layer was
already delivered before this batch. It must not be counted as a new idea-10
implementation. The existing Crew.c matches cache/app; this audit does not
claim a fresh replay of payroll/debt behavior.

## Delivery and PR state

`gameplay/` is ignored and has no tracked source files. These development
edits are not integrated into the canonical sync package; `agent_context.py
--check` passing does not prove their durability. The installed bundle fails
`codesign --verify --deep --strict` with an invalid sealed resource after the
resource delivery. No engine replacement or player-state mutation occurred
during this audit.

PRs 11, 14 and 15 are verified merged into main. Subsequently created drafts
16 and 17 remain open; their own descriptions explicitly leave installed
staging/player replay pending. PR 17 is stacked on PR 16. They are renderer/
camera candidates, not proof that this gameplay batch is integrated.

## Required correction and acceptance

The gameplay integration owner must first repair the identified state/quantity
boundaries and preserve the result in the canonical Metal content owner.
Resource delivery must preserve unknown edits and reseal the installed bundle.

Player acceptance remains: F12 with/without HPPlus and existing HP adjustments;
entry with an empty charge and enough alternative ammo for only one bort;
two consecutive boarding/surrender sequences and a quest letter; carpenter
repair with depleted own/shared materials and a locked companion; paid fleet
morale; junk sale at sea versus a market; carousel selection of a captain
without a treasurer. No real-game success is claimed for these scenarios.
