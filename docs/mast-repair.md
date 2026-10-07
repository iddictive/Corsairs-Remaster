# Emergency mast repair at sea

## Contract and owners

The flagship's quick menu offers `Ремонт мачт` only in safe, active sea mode,
with crew, fallen main masts and enough planks for at least one mast. Combat,
nearby enemies, storms, tornadoes and scene transitions reject it.

The dedicated mast/mallet icon follows the existing command atlas, with a thin
antique-brass rim, dark brown normal and muted blue selected states. These rims
are painted into the command textures; the native menu draws each tile whole.
`src/assets/ui/manifest.json` binds the generated PNG and prepared 128×64 TX
texture to their hashes and runtime destination. The existing native command
texture list adds slot 5 (two 64px tiles); the shared command atlas stays intact.
`tools/sync_metal_gameplay.py` delivers icon and script through the ordinary
receipt/backup/rollback transaction, reusing the installed engine. Missing icons
are created on first delivery; unknown edits and mismatched source hashes reject
delivery. Selected canonical text-only updates retain their existing scope.

Standing main masts after repair may not exceed `floor(total * 0.6)`. Existing
standing masts count toward that ceiling, preventing repeat repairs from raising
every mast. Topmasts belong to their main mast and do not inflate the denominator.
A three-masted ship can have one main mast raised; a five-masted ship can have
three. Hull damage and damage to unrelated sails remain unchanged.

Each main mast costs `100 * GetHullPPP(captain) / total` planks. The ordinary
repair helpers own shared reserve access, Builder's discount, fractional credit
and integer withdrawals. The confirmation shows the actual integer withdrawal,
mast count and time. Cancel leaves materials, damage and time unchanged.

Duration is rounded up to whole hours: `72 - 64 * efficiency`, where efficiency
is effective Repair/100, multiplied by `(0.5 + 0.5 * carpenterPerkRatio)` and
`(0.5 + 0.5 * crew/optimalCrew)`. Each factor is bounded to its normal range.
Zero Repair takes 72 hours; Repair 100, every available carpenter perk and a
full optimal crew take 8 hours. Understaffing and missing perks increase time.
Confirmation advances game time through ordinary `WaitDate`, including quests.

`tools/gameplay/mast-repair.c` owns planning, confirmation state and execution;
the fleet composer adds its flagship command to BattleInterface. The registered
`src/gameplay/PROGRAM/interface/LeaveBattle.c` reuses the existing two-button
confirmation and retains ordinary leave-battle behavior. A quote is cleared on
cancel/sea teardown and revalidated before mutation. Native failure restores
the previous mast/sail attributes before any plank withdrawal or time advance.

`experiments/native-metal/mast-repair.patch` owns `MSG_SHIP_REPAIR_MASTS` and the
native model operation. It loads clean authored geometry, preflights selected
mast subtrees and attaches them to the existing ship model. It preserves ship
and model IDs, AI tasks, positions and native mast array order. Rig groups are
deleted, flushed and recreated; existing `Ship.Sails` hole masks rehydrate
unaffected sail damage. The native save codec records the restored mast flags.
Shared script headers and compiler readiness bind this bridge to its engine.

## Rejected routes and evidence boundary

Changing only `Ship.Masts` cannot restore geometry already detached by MastFall.
Re-entering the sea recreates encounters and ship formation. A Sea_Save/Sea_Load
round trip overwrites new script mast values with serialized native broken flags.
Neither route satisfies restoration in the current encounter.

Native engine compilation, whole-PROGRAM plus lazy confirmation compilation,
and a native VM contract fixture pass. The fixture exercises limits, time bounds,
resource shortages, cancellation, stale quotes, rollback, topmast selection and
repeat callbacks; its geometry bridge and external queries are instrumented.
These checks do not prove rendered mast attachment, confirmation fit or player
save/load replay. Their current disposition belongs to `docs/runtime.md`.
