# Player Phonk ammo evidence, 2026-10-21

Observed defect: actual `Ship_ChangeCharge(player, GOOD_BALLS)` leaves `Sea.Phonk.Weapon=6`; native ordnance then retains laser ownership, while ordinary cannon load and availability still require real ammunition and powder. The pre-change real Storm VM fixture returns101 with zero script errors (`baseline-cases.c`, `baseline.log`).

The corrected source owns Phonk ordinary weapon selection with0, retaining native laser6/nuclear7 internally and displaying hotkeys7/8. `Ship_ChangeCharge` clears the special selection for the player only. Six ordinary ammo types use virtual availability and skip cargo load/unload debit/refund only for the player with the scene mode flag set. NPC and ordinary scarcity remain unchanged. `AIShip::Fire` gates both camera paths only for selected special weapons. Native laser hold requires weapon6; native ordnance reads and retains0 each frame.

`SeaPhonk_SetActive` uses the existing native reload: synchronous unload under the old mode flag, followed by deferred reload under the new flag. This returns already loaded physical rounds before entry and discards virtual rounds before exit, including death. Direct sailing calls the same transition before restoring the saved mode attributes. No persistent supply grants or cannon codec changes are introduced.

## Reproduce

From repository root, using the existing canonical diagnostic binary:

```sh
experiments/native-metal/.cache/build/bin/script-vm-probe docs/evidence/cannon-darts-20261021/phonk-ammo PhonkAmmoSaveCase 1 PhonkAmmoAfterLoad
```

Accepted result: entry0, after-load0, script_errors0, process exit0. `source-sha256.json` binds the inspected production sources. The fixture extracts actual goods access, ammo load/unload, change-charge, availability and HUD function bodies. Only native reload transport and entity admission are replaced by an explicit synchronous-unload/deferred-reload adapter; external UI/music and ship geometry services use small stubs. The retained complete fixture needs no regeneration to replay.

Cases cover all six ordinary selections from laser with zero ammo/powder, nonmutating virtual load/unload, cannon and battle-menu availability, ordinary versus special HUD, NPC scarcity/debit, normal-mode scarcity/debit/refund, physical-to-virtual-to-physical loaded-round provenance, repeated paused toggles, zero-supply exit, death with raw mode flag retained, and the GreatRecipe restriction outside Phonk. A real VM save/load roundtrip preserves the scene flag/selection and verifies virtual rounds are not refunded after load. The fixture supplies the VM-required OnLoad callback.

## Evidence limit

The cannon transport adapter follows `AIShipCannonController::Reload`/`Execute` source. The real native cannon codec retains bEmpty/bReload independently; this fixture does not instantiate a complete live cannon controller. `Sea_Load` restores native cannons before posting `Sea_FirstInit`, which resets the still-retained Phonk scene flag via the new transition. Full game scene, controls, salvo, native save and direct-sailing acceptance remain with the parent runtime owner. No build, GPU probe, installed binary, player state or save was modified by this evidence run.
