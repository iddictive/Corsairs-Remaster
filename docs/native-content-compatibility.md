# Native content compatibility

## Status

October 3, 2026: provenance established for the named materials and cabin
scripts; combined delivery remains blocked by active gameplay revisions.
This document proves source/content compatibility, not installed game behavior.
No staging, app write, engine build, signing, launch or game input was performed.

The local review branch is `audit/native-content-provenance`, based on PR17
commit `2efb8155d2f066d25cdcc6669f1f5678a8b2e61a`. Main is
`3b2462f58922fbdfeb702b6054ea052800b75654`; PR11/14/15 are already merged.
PR16 and PR17 remain separate drafts; PR17 is stacked on PR16.

## Material and renderer ownership

`cannon-trajectory-aim.patch` owns engine-authored additions to
`src/techniques/_dev/ship.fx`. The runtime file belongs at
`RESOURCE/techniques/_dev/ship.fx`. Workspace, cache and installed app contain
the same historical revision `092a2b5a...`; this is source authored in Git,
not an unidentified third-party shader or texture modification.

| Revision | Exact SHA-256 | Reconstruction and consumer |
| --- | --- | --- |
| Original ShipCannonAngles | `e868be15c45b8b2d91489b939933b0c420669f241ac6ea5cde1c9e04465fd8e9` | Unchanged 360-byte prefix of both versions. |
| Installed Arc/Volume | `092a2b5a087dd44d2e3c0eb779d167819fa672398b80189e1f05a75fffaf9621` | Prefix plus exact 1,074-byte technique hunk from `168c245` through `8b57e71`. Historical aim code requested ShipAimArc/ShipAimVolume. |
| Current main Footprint/Reticle | `1629e130e608483aa5ab640d8d429d3d882810889306875ad3d9e515f87b46f9` | Same prefix plus exact 1,163-byte hunk from main. PR16 and PR17 preserve this hunk byte for byte. |

Current `AIShipCameraController::Realize` calls DrawManualAimPlus, which requests
ShipAimReticle with XYZRHW/diffuse triangles. That technique disables depth
testing and writes and uses source-alpha/inverse-source-alpha blending.
The installed executable contains the ShipAimReticle name, while its installed
ship.fx has only ShipCannonAngles/ShipAimArc/ShipAimVolume. The renderer's
ExecutePassStart returns false for an absent technique; named DrawPrimitiveUP
then skips its draw. This establishes a missing-definition failure path,
not an observed screenshot or proof of live reload.

The world envelope uses StormMetalDrawAimVolume and the backend's SoftAimVolume
depth/receiver pass. ShipAimVolume is not its current material owner.
ShipAimFootprint remains defined, with explicit depth testing on and depth
writes off; no current C++ call to that name was found in the built source.
Adding Arc/Volume aliases does not restore the current reticle contract.

An independent review-only FilePatch candidate replaces only the historical
technique addition. It uses the existing runtime_script_patch.transform,
wrapped by exact input/output SHA checks, and has no runtime-writing CLI.
Forward and reverse bytes match the known revisions; both are idempotent.
Four negatives reject changes in the preserved base, the legacy block,
unknown trailing bytes and the upgraded rollback input. All 360 base bytes
remain unchanged. No allowlist or staging consumer was modified.

Review artifacts live under ignored
`experiments/native-metal/.cache/content-provenance/`: contract.json,
snapshot.json, ship-technique-upgrade.json, ship-technique-candidate.py and
ship-technique-evidence.json. They can be regenerated from the pinned Git
hunks above without game assets. The fixture's initial import-path error was
corrected before the byte/negative checks ran; its final execution passed.

## Cabin and menu compatibility

The two installed cabin scripts reproduce byte for byte from committed
transforms at `c7e4df5` (the parent of `48fb957`). They are older reviewed
outputs, not new anonymous gameplay work. Current sync already recognizes
both through PREVIOUS; they require no new hash admission.

| Consumer | Installed state | Current main relationship |
| --- | --- | --- |
| PROGRAM/interface/ship.c | `d8a46d6d...`, direct ProcessExitCancel then LaunchCabinChest, click only; original fleet supply rows. | `bf94ec32...` gates with CabinChest_CanOpen, posts LaunchIAfterFrame/I_CABIN_CHEST, supports activate and incorporates merged PR14's two supply blocks. |
| PROGRAM/scripts/food.c | `e447c4af...`, LaunchItemsBox without the CabinChest face identifier; no CabinChest_CanOpen. | patch_squad_common_supply produces `b9f0d3df...`; adds the predicate and opens INTERFACE_ITEMSBOX with the CabinChest identifier. Other auto-supply/reserve changes are preserved by exact reconstruction. |
| PROGRAM/interface/interface.c | `7f37bbf8...`, already equal to main. | I_CABIN_CHEST dispatches LaunchCabinChest after the prior menu closes. |
| PROGRAM/interface/itemsbox.c | `f9fe490c...`, already equal to main. | CabinChest-specific pause/actor/exit/quest behavior is already installed. |
| PROGRAM/ITEMS/itemLogic.c | `59d52202...`, already equal to main. | Existing refill callback after cabin boxes materialize is preserved. |
| RESOURCE/INI/interfaces/ship.ini | `5fc98351...`, exact reviewed chest-separator hotfix; old supply layout. | Current main output is `b4e7ec7d...`, including PR14's supply layout. Ship.c and layout must move together. |

Do not update ship.c alone: it would reference an undefined
CabinChest_CanOpen in the installed food.c. Moving ship.c, food.c and ship.ini
together restores the main contract while interface.c, itemsbox.c and
itemLogic.c can retain their identical bytes. This remains a proposed delivery
group; no chest interaction was replayed here.

## Active gameplay owner and camera compatibility

The installed Debuger.c (`2cedf4e3...`) is the original portable input
(`8ca6a3d1...`) with one F12 HP block and line-ending normalization. The
gameplay owner's recorded edit and dev push establish the writer/delivery
path. BOAL_Control2 in seadogs.c dynamically loads Debuger.c, calls
ActiveF12Control and unloads it. The new block calls the changed RPG HP
functions and LAi_SetHP. Its HPPlus/bonus_hp defect is recorded by the existing
fleet-gameplay audit; it must not be promoted merely because it compiled.

During this investigation the same owner reset nine workspace scripts to
canonical outputs, preserved its preceding development versions, and began
new corrections. Debuger.c was observed first as `8ca6a3d1...` and later as
`c5c84e5d...`; RPGUtilite.c also changed. Installed/cache bytes still held the
prior delivered batch at the bounded snapshot. This is an active writer,
not a frozen nine-file candidate. Those nine paths are Debuger.c,
characters/RPGUtilite.c, Loc_ai/LAi_character.c, interface/GoodsTransfer.c,
sea_ai/AIShip.c, dialogs/russian/Common_Brothel.c, Loc_ai/LAi_boarding.c,
characters/characterUtilite.c and dialogs/russian/Enc_Officer_dialog.c.

PR16 affects vegetation fog in renderer source; it does not own these scripts
or the technique upgrade. PR17's exact reversible AICameras adapter recognizes
installed/workspace `eb62a79d...` and preserves saved-mode guards. Its first-Tab
aim order and zoom support remain undelivered. The current material hunk is
identical between main, PR16 and PR17, so this legacy FX upgrade does not select
between their camera/fog behavior. Engine and script acceptance remain separate.

## Minimal preserving delivery path

1. The gameplay integration owner supplies a frozen durable transform over
   current main outputs for its corrected nine scripts, with exact before/after
   identities and unknown-input negatives. Its temporary canonical resets are
   not permission to erase the delivered feature intentions.
2. The parent reviews the independent historical FX transform and the coupled
   main cabin/menu group. Integrate them into the existing exact-hash staging
   owners, rather than bypassing gates or admitting the whole ignored workspace.
   The FX transform alone does not clear the Debuger/sync blocker.
3. Freeze development writers and preserve fresh workspace/cache/app bytes,
   installed executable/signature and player SAVE/configuration. Enumerate the
   final delivery group before using the canonical stage command, which can
   mutate the cache before its later checks. Keep all other delivered content.
4. After the game closes under the parent's delivery decision, stage the exact
   reviewed main + PR16/17 + gameplay group once, verify installed identities and
   reseal the existing bundle. Reopening the installed app and replaying the
   actual actions is still required; source/probe evidence cannot substitute.

The present blocking owner decision is to finish and freeze the nine-script
repair in its durable Metal content owner. Dependent integration is stopped.
No new PR was created, no main merge occurred, and the canonical dirty helper
was preserved untouched.
