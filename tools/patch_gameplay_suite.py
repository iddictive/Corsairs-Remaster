#!/usr/bin/env python3
"""Install verified gameplay scripts into native-storm; explicit Windows support remains."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

from runtime_script_patch import (
    FilePatch,
    TARGET_ROOT,
    atomic_write,
    lifecycle_lock,
    require_idle,
    sha256,
    transform,
)


SNAPSHOT_ROOT = TARGET_ROOT / ".codex-gameplay-suite" / "20260914-v1"
NATIVE_ROOT = Path(__file__).resolve().parents[1] / "experiments/native-storm/.cache/runtime"
NATIVE_ENGINE = NATIVE_ROOT.parent / "CorsairsNative.app/Contents/MacOS/native-engine"
SUPPORTED_NATIVE_SHA256 = "30c1b91e2cecbbadeec96d7954b7d6fe536b03763c73d50c24a6cc87519c0863"
NATIVE_PREVIOUS_SHA256 = {
    "PROGRAM/Loc_ai/LAi_fightparams.c": "5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",
    "PROGRAM/dialog.c": "b141897109b0e751e592e064b9f2ea23bdb3c01334ee8f98d643cc0be05ab699",
    "PROGRAM/locations/locations_loader.c": "113764a8246bcc3987217c76fb1966330b2b6d39a5d724cc09a4556fbc3afd8c",
    "PROGRAM/quests/quests_reaction.c": "9aabee25dd86dffe3c3f6f7cfd8b4460b116ee0c2d7a18f53d920b3cd459852b",
    "PROGRAM/sea_ai/AIShip.c": "1cb84155342dadf657db518395ae519d47772cba67d64ce112956eb705a618bd",
}
SUPPORTED_ENGINE_SHA256 = (
    "861e4ba02752b2303058202e074d8d80d37c51b716ed475dc20e4adbb08c963f"
)

# These are hashes of the cumulative outputs, not the standalone component
# outputs. Keep the vector explicit so a changed provider or ordering cannot
# silently redefine the installed state.
COMPOSITE_SHA256 = {
    "PROGRAM/Loc_ai/LAi_boarding.c": "ff47f627f7ab264585f751653d85949bee70f7c40a61b013ceb04118acc17934",
    "PROGRAM/Loc_ai/LAi_fightparams.c": "0e5c671e97b2575586ee87379db027c0393689f1e60b242b5c9d215a7cf37f78",
    "PROGRAM/Loc_ai/LAi_monsters.c": "b1b957d11540d120e48c2fc1a5591d7ec24481fc8d3b7f363c68e8be1bb6ab11",
    "PROGRAM/Loc_ai/LAi_utilites.c": "b76b427775e1ad0555865bb6c05b3e04307b694f03d0812bc3c876b6ae5a0449",
    "PROGRAM/battle_interface/landinterface.c": "6b467ce0f8de248f2dd8865378a7ce88fefe8af369e5147428da356035e976b7",
    "PROGRAM/battle_interface/loginterface.c": "88a187aec46bf600ff6c239eb2d358b8155775a006f70d085917fa7bc79a849c",
    "PROGRAM/characters/characterUtilite.c": "104e5a88131306aba33bfd07a11a79b71ddb945a1252a55579cf7c870f11d1dd",
    "PROGRAM/dialog.c": "75cf22a1481414f20629afd922ac4afebc2621ddd8b1852918a8003908b7635d",
    "PROGRAM/dialogs/russian/Common_Prison.c": "fbe34cd9460157870f2754c3037a4b6c5949bedfb5833ea844bcd96209ba4b64",
    "PROGRAM/dialogs/russian/Common_ItemTrader.c": "ccd0009666c9d65c0613d34022fa7df1b23036d7dee5c19d745143242b56b874",
    "PROGRAM/dialogs/russian/Common_Soldier.c": "e90b6564c144589ffa5ffcd01ae5ff8551f911c1d9615bc6f3b1e223a8111de9",
    "PROGRAM/dialogs/russian/Common_Store.c": "b4d1dd473489ff0506e04466dbcdd61d235d891d61701e34232fff2837c01ab2",
    "PROGRAM/dialogs/russian/Common_Tavern.c": "d247238cead8cf02870e750010875dbf08ed21f219a6cd5f764ea3a40ddd1f76",
    "PROGRAM/dialogs/russian/Enc_Patrol.c": "d6b22e4cbf9ff97fe5759798d9157ca84783e9b8c9ef758fbb825c1f35f47758",
    "PROGRAM/dialogs/russian/Hunter_dialog.c": "f19f783d5b8d3a388983f29ba21f3b1eddc7f88092e8ac1c477f436cf1a6b3a0",
    "PROGRAM/interface/hirecrew.c": "13f66bb3ac70fa7d7a9edfa33235ec1941e9c824a1e1debad1ddd4efdee9eedb",
    "PROGRAM/interface/salary.c": "125a9604e6237fc2332dbfdd8c0ecdee9a22568ce0b288c82f484e7bf64abbc5",
    "PROGRAM/interface/ship.c": "c198c1df317cc9f2c179291fac71c7bf0c88d21207468ed871f41ec188aabe34",
    "PROGRAM/locations/locations_loader.c": "122adcdd62bccbe1d60f2860d6513f88682856141366fc365f61a46b74105a31",
    "PROGRAM/quests/quests.c": "4e3bbd97bfad571c631c3dcf17b0e6783279d652c78915b4a6f9149ec4654bdc",
    "PROGRAM/quests/quests_reaction.c": "4ef98d00aa9ae4b0eef913c817e62ec6f0db0b53292f20633a897128a8bd0de6",
    "PROGRAM/scripts/GoldFleet.c": "1ea9fc5a3d95983fc6ac4b664de90541ae1324b60bca187666441ed134631c3e",
    "PROGRAM/scripts/Crew.c": "072f41ae931d3c19946dab7aca5da9fa7f85c76e14aeebd8836c102a9230b490",
    "PROGRAM/scripts/bountyhunters.c": "90e1ec28ea2ef5455f67a8097e33ce7b894713220f2d44a104ff1fed41235e8d",
    "PROGRAM/scripts/colony.c": "c3225c680846a904fd7a8228887aea9879f5fa4482772596b222ee2ee0380c3a",
    "PROGRAM/scripts/islandships.c": "bc03feaf048837187e22bad6fc7671b860de0fe565cd7dace027b76bc96cca20",
    "PROGRAM/scripts/time_events.c": "9da55205414ac74922b3ae9fb0d1cde6de7257566c5f84536da2bc0921566e43",
    "PROGRAM/sea_ai/AISeaGoods.c": "39050a04396461aaf2cb05d59d8d1269f67e48de2b75af5ccebcb76f8b44e162",
    "PROGRAM/sea_ai/AIShip.c": "b9a0639d4471e6aa95ad36da7d333f2decae8906b5cb979d2d8e1a8b760ecbe6",
    "PROGRAM/sea_ai/Cabin.c": "9334dc64ffcc17ebe991a82a64f2c69e63b288791ea69b16ad6964e8d1df2271",
    "PROGRAM/sea_ai/sea.c": "107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1",
    "PROGRAM/seadogs.c": "8019f758cbc516ba70583719c6abd96d1d13a641c73b4db3de46a28c2a334340",
    "PROGRAM/scripts/custody.c": "26dffe1eaefa640f0f489db839bcc8f4ee486a9e35216771ddcf5693a802a558",
    "RESOURCE/INI/interfaces/ship.ini": "e5822481464d182b80c2e89318e02e6183ec8e3d934bfd3aa0b3e89148cc7b4a",
}

CREATED_FILES = {"PROGRAM/scripts/custody.c"}
LEGACY_SHOP_PATH = "PROGRAM/scripts/shop_rotation.c"
LEGACY_SHOP_SHA256 = "c4af5f1e97719a8010aebbf2b33a58c20f038a77d7b9cde3258387f5831cdccf"
SHOP_SEGMENT_SHA256 = "a97d9206f3e6c05226492ae7d71b62c9f679087720cfad1ce98d7f08ec248666"
PREVIOUS_SHA256 = {
    "PROGRAM/Loc_ai/LAi_fightparams.c": "5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",
    "PROGRAM/quests/quests_reaction.c": "99934a1d95f959714280dc366d611140fdb6f3d5ca90d19983e8a4ccab1176ec",
    "PROGRAM/scripts/colony.c": "ad03ea0623a4c367ed1a9698e15a01cd39078ab36ee8f0a69c5a750ea7b8f799",
    "PROGRAM/seadogs.c": "50248f0e85736c3e8ff35211db110dbb96fb206a7bad507106c6af89c7d15841",
    "PROGRAM/scripts/custody.c": "c75b9a3b3a605d6fa310ff5d632ad5ac57bfcb5c2402e32addf019a9651e7c3e",
}


def providers() -> tuple[tuple[str, tuple[FilePatch, ...]], ...]:
    """Return providers in one stable dependency and composition order."""
    from patch_cabin_sleep_weather import PATCH as cabin_patch
    from patch_crime_debt_suite import patch_sets as crime_debt_patch_sets
    from patch_dialog_cancel import PATCH as dialog_cancel_patch
    from patch_land_encounter_combat_lock import FILES as land_files
    from patch_prison_surrender import FILES as prison_files
    from patch_hero_firearm_damage_cap import FILES as firearm_cap_files

    result = [
        (label, tuple(patch_set.files))
        for label, patch_set in crime_debt_patch_sets()
    ]
    result.extend(
        (
            ("cabin sleep weather", tuple(cabin_patch.files)),
            ("land encounter combat lock", tuple(land_files)),
            ("prison surrender", tuple(prison_files)),
            # Composed after the dialog attack patch so both target dialog.c.
            ("dialog cancel", (dialog_cancel_patch,)),
            # Composed after crime attribution, which already owns this file.
            ("hero firearm damage cap", tuple(firearm_cap_files)),
        )
    )
    return tuple(result)


def file_fragments() -> dict[str, tuple[tuple[str, FilePatch], ...]]:
    fragments: dict[str, list[tuple[str, FilePatch]]] = defaultdict(list)
    for label, specs in providers():
        for spec in specs:
            fragments[spec.relative_path].append((label, spec))
    result = {path: tuple(items) for path, items in sorted(fragments.items())}
    expected = set(COMPOSITE_SHA256) - CREATED_FILES
    if set(result) != expected:
        missing = sorted(set(result) - expected)
        stale = sorted(expected - set(result))
        raise RuntimeError(
            f"composite hash vector is stale; missing={missing}, removed={stale}"
        )
    return result


def base_hashes() -> dict[str, str]:
    return {
        path: fragments[0][1].original_sha256
        for path, fragments in file_fragments().items()
    }


def _apply_replacements(source: bytes, spec: FilePatch) -> bytes:
    """Apply one provider to cumulative bytes without its standalone hash gate."""
    newline = "\r\n" if b"\r\n" in source else "\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline).encode("utf-8")
        new_bytes = new.replace("\n", newline).encode("utf-8")
        count = result.count(old_bytes)
        if count != 1:
            raise RuntimeError(
                f"{spec.relative_path}: cumulative block for provider is not unique ({count})"
            )
        result = result.replace(old_bytes, new_bytes, 1)
    return result


def expected_bytes(
    relative_path: str,
    source: bytes,
    fragments: tuple[tuple[str, FilePatch], ...],
) -> bytes:
    expected_base = fragments[0][1].original_sha256
    if sha256(source) != expected_base:
        raise RuntimeError(
            f"{relative_path}: expected base {expected_base}, got {sha256(source)}"
        )

    # Keep every declared standalone exact-hash edge valid while separately
    # composing independent same-base edits into the cumulative output.
    nodes = {expected_base: source}
    cumulative = source
    for label, spec in fragments:
        declared_input = nodes.get(spec.original_sha256)
        if declared_input is None:
            raise RuntimeError(
                f"{relative_path}: {label} input {spec.original_sha256} is not a known node"
            )
        declared_output = transform(declared_input, spec)
        nodes[spec.patched_sha256] = declared_output
        cumulative = _apply_replacements(cumulative, spec)
        nodes[sha256(cumulative)] = cumulative

    expected_final = COMPOSITE_SHA256[relative_path]
    actual_final = sha256(cumulative)
    if actual_final != expected_final:
        raise RuntimeError(
            f"{relative_path}: cumulative output expected {expected_final}, got {actual_final}"
        )
    return cumulative


def classify(root: Path) -> tuple[str, list[tuple[str, str, str]]]:
    from patch_mod_journal import BASE_SHA256, UPDATED_SHA256
    from patch_sea_battle_mode_reset import UPDATED_SHA256 as COMBAT_UPDATED_SHA256

    bases = base_hashes()
    bases.update({p: h for p, h in BASE_SHA256.items() if p not in bases})
    delivered = {**COMPOSITE_SHA256, **UPDATED_SHA256, **COMBAT_UPDATED_SHA256}
    rows: list[tuple[str, str, str]] = []
    states: set[str] = set()
    for relative_path in sorted(delivered):
        path = root / relative_path
        digest = sha256(path.read_bytes()) if path.is_file() else "-"
        if relative_path in CREATED_FILES and not path.exists():
            state = "original"
        elif relative_path in CREATED_FILES and digest == delivered[relative_path]:
            state = "patched"
        elif relative_path in CREATED_FILES:
            state = "unsupported"
        elif digest == bases[relative_path]:
            state = "original"
        elif digest == delivered[relative_path]:
            state = "patched"
        else:
            state = "unsupported"
        rows.append((relative_path, state, digest))
        states.add(state)
    aggregate = states.pop() if len(states) == 1 else "mixed"
    if aggregate == "mixed":
        original_paths = {path for path, state, _ in rows if state == "original"}
        unsupported = {path for path, state, _ in rows if state == "unsupported"}
        shop = root / LEGACY_SHOP_PATH
        shop_digest = sha256(shop.read_bytes()) if shop.is_file() else "-"
        if not unsupported and original_paths == {"PROGRAM/seadogs.c", "PROGRAM/scripts/custody.c"} and shop_digest == LEGACY_SHOP_SHA256:
            aggregate = "legacy"
    if aggregate in {"mixed", "unsupported"}:
        previous_delivered = dict(delivered)
        previous_delivered.update(NATIVE_PREVIOUS_SHA256)
        if all(digest == previous_delivered[path] for path, _, digest in rows):
            aggregate = "upgrade"
        previous_external = dict(delivered)
        previous_external.update(PREVIOUS_SHA256)
        if all(digest == previous_external[path] for path, _, digest in rows):
            aggregate = "upgrade"
    return aggregate, rows


def _engine_hash(root: Path) -> str:
    engine = NATIVE_ENGINE if root.resolve() == NATIVE_ROOT.resolve() else root / "engine.exe"
    return sha256(engine.read_bytes()) if engine.is_file() else "missing"


def _require_engine(root: Path) -> None:
    actual = _engine_hash(root)
    expected = SUPPORTED_NATIVE_SHA256 if root.resolve() == NATIVE_ROOT.resolve() else SUPPORTED_ENGINE_SHA256
    if actual != expected:
        raise RuntimeError(
            f"installed engine does not support the cumulative gameplay suite: {actual}"
        )


def print_status(root: Path) -> int:
    state, rows = classify(root)
    engine = _engine_hash(root)
    engine_state = "supported" if engine in {SUPPORTED_ENGINE_SHA256, SUPPORTED_NATIVE_SHA256} else "unsupported"
    print(f"target: {root}")
    print(f"state:  {state}")
    print(f"engine: {engine_state} {engine}")
    for path, item_state, digest in rows:
        print(f"{item_state:11} {digest}  {path}")
    return 0 if state in {"original", "patched"} else 1


def _read_originals(root: Path, state: str) -> dict[str, bytes]:
    bases = base_hashes()
    originals: dict[str, bytes] = {}
    for relative_path in sorted(set(COMPOSITE_SHA256) - CREATED_FILES):
        if state == "original":
            candidates = (root / relative_path,)
        else:
            snapshot = SNAPSHOT_ROOT / relative_path
            candidates = (snapshot, TARGET_ROOT / relative_path, root / relative_path)
        path = next(
            (
                candidate
                for candidate in candidates
                if candidate.is_file()
                and sha256(candidate.read_bytes()) == bases[relative_path]
            ),
            None,
        )
        if path is None:
            raise RuntimeError(
                f"missing exact original {bases[relative_path]} for {relative_path}"
            )
        data = path.read_bytes()
        originals[relative_path] = data
    return originals


def _build_outputs(originals: dict[str, bytes]) -> dict[str, bytes]:
    from patch_prison_surrender import CUSTODY_SEGMENT_BYTES, CUSTODY_SEGMENT_PATH
    from patch_mod_journal import BASE_SHA256, UPDATED_SHA256, transform_outputs
    from patch_sea_battle_mode_reset import (
        UPDATED_SHA256 as COMBAT_UPDATED_SHA256,
        transform_outputs as combat_transform,
    )

    outputs = {
        path: expected_bytes(path, originals[path], fragments)
        for path, fragments in file_fragments().items()
    }
    outputs[CUSTODY_SEGMENT_PATH] = CUSTODY_SEGMENT_BYTES
    for path, digest in BASE_SHA256.items():
        if path in outputs:
            continue
        snapshot = NATIVE_ROOT.parent / "journal-baseline" / path
        source = snapshot if snapshot.is_file() else NATIVE_ROOT / path
        data = source.read_bytes()
        if sha256(data) != digest:
            raise RuntimeError(f"journal baseline mismatch: {source}")
        outputs[path] = data
    outputs = transform_outputs(outputs)
    for path, digest in UPDATED_SHA256.items():
        if sha256(outputs[path]) != digest:
            raise RuntimeError(f"journal output mismatch: {path}")
    outputs = combat_transform(outputs)
    for path, digest in COMBAT_UPDATED_SHA256.items():
        if sha256(outputs[path]) != digest:
            raise RuntimeError(f"battle-mode reset output mismatch: {path}")
    return outputs


def _snapshot(originals: dict[str, bytes]) -> None:
    bases = base_hashes()
    for relative_path in sorted(originals):
        data = originals[relative_path]
        if sha256(data) != bases[relative_path]:
            raise RuntimeError(f"{relative_path}: source changed before snapshot")
        snapshot = SNAPSHOT_ROOT / relative_path
        if snapshot.exists():
            if not snapshot.is_file() or sha256(snapshot.read_bytes()) != bases[relative_path]:
                raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
            continue
        atomic_write(snapshot, data)
        os.chmod(snapshot, 0o444)


def _verify_cross_feature_contracts(outputs: dict[str, bytes]) -> None:
    from patch_crew_debt_ledger import verify_fixture as verify_debt_fixture
    from patch_dialog_cancel import verify as verify_dialog_cancel
    from patch_dialog_ambush import _verify_dialog, _verify_utilities
    from patch_prison_surrender import _sentence, _standing

    _verify_dialog(outputs["PROGRAM/dialog.c"].decode("utf-8"))
    verify_dialog_cancel(outputs["PROGRAM/dialog.c"].decode("utf-8"))
    _verify_utilities(outputs["PROGRAM/Loc_ai/LAi_utilites.c"].decode("utf-8"))
    verify_debt_fixture()
    if _sentence(-5, False) != 3 or _sentence(100, True) != 11:
        raise RuntimeError("custody sentence fixture failed")
    if _standing(5, -18) != 0 or _standing(95, 14) != 100:
        raise RuntimeError("custody standing fixture failed")

    required = {
        "PROGRAM/Loc_ai/LAi_boarding.c": (
            "ref mchr = GetMainCharacter();\r\n\tLAi_LockFightMode(mchr, false);",
            'SendMessage(&mchr, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);',
        ),
        "PROGRAM/Loc_ai/LAi_fightparams.c": (
            "float LAi_LimitMainCharacterFirearmDamage(aref enemy, float damage)",
            "MakeInt(LAi_GetCharacterMaxHP(enemy) * 0.49)",
            "damage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);",
            "enemy.Killer.Index = attack.index;",
            "Crime_HasPlayerIntent(enemy)",
        ),
        "PROGRAM/quests/quests_reaction.c": (
            "if (bCabinStarted) bCabinSleepWeatherPending = true;",
            "LAi_LockFightMode(pchar, false);",
            'case "MainHeroFightModeOff":\r\n\t\t\tLAi_SetFightMode(pchar, false);\r\n\t\t\tLAi_LockFightMode(pchar, false);',
            'case "LandEnc_RaidersBegin":',
            'case "LandEnc_RapersTalk":',
            'case "LandEnc_PatrolBegin":',
            'LAi_group_SetCheck("EnemyFight", "LandEnc_RapersAfrer");',
            'case "LandEnc_RapersAfrer": // грохнули бандюков\r\n\t\t\tchrDisableReloadToLocation = false;',
        ),
        "PROGRAM/locations/locations_loader.c": (
            'CheckAttribute(loc, "boarding")',
            "LAi_LockFightMode(Pchar, false);",
            'SendMessage(&Pchar, "lsl", MSG_CHARACTER_EX_MSG, "SetFightWOWeapon", false);',
            "CrimeLand_ResolveLocation(loc.id);",
            'CheckAttribute(Pchar, "DialogAmbush") || CheckAttribute(Pchar, "DialogAmbushSneak")',
            'CheckAttribute(Pchar, "DialogAmbush.ActiveTargetIndex")',
            'LAi_group_MoveCharacter(oldAmbushTarget, oldAmbushTarget.DialogAmbush.OriginalGroup);',
            'LAi_group_Delete(oldAmbushGroup);',
            'CheckAttribute(Pchar, "GenQuest.EncGirl.LocIdx")',
            'LAi_group_RemoveCheck("EnemyFight");',
            'if (rapersAlive) LAi_group_SetCheck("EnemyFight", "LandEnc_RapersAfrer");',
            'DoQuestCheckDelay("LandEnc_RapersAfrer", 0.1);',
        ),
        "PROGRAM/scripts/GoldFleet.c": (
            "extern bool Custody_CanSurrender(ref captor);",
            "extern bool Custody_TryCombatSurrender();",
            "extern void Custody_Release();",
        ),
        "PROGRAM/scripts/custody.c": (
            "bool Custody_CanSurrender(ref captor)",
            "string nationKey = NationShortName(nation);",
            'captor.Dialog.Filename == "Enc_Patrol.c"',
            'string jail = city + "_prison";',
            "Custody_MoveFleetTo(dock);",
            "Custody_MoveFleetTo(destination);",
            'shoreReload.name == "boat"',
            "pchar.questTemp.jailCanMove = pchar.Custody.PreviousJailCanMove;",
            'ChangeCharacterAddressGroup(mate, loc.id, "goto", "goto24");',
            'if (!LAi_IsCharacterControl(pchar)) LAi_SetPlayerType(pchar);',
            'LAi_LockFightMode(pchar, true);',
            'pchar.quest.CustodyReleaseReservation.function = "Custody_ClearReleaseReservation";',
            'CheckAttribute(pchar, "GenQuest.CaptainComission")',
            'SetNationRelation2MainCharacter(nation, RELATION_NEUTRAL);',
            "bool Custody_TryCombatSurrender()",
            'captor.Dialog.CurrentNode = "Custody_CombatArrest";',
            'for (int i = 1; i < MAX_CHARACTERS; i++)',
            'candidate.location != pchar.location',
            'GetCharacterDistByChr(pchar, candidate, &distance) == false',
            'CharactersVisibleTest(pchar, candidate)',
        ),
        "PROGRAM/seadogs.c": (
            'LoadSegment("scripts\\shop_rotation.c")',
            'LoadSegment("scripts\\custody.c")',
            'Trace("shop rotation segment failed to load")',
            'Trace("custody segment failed to load")',
        ),
        "PROGRAM/battle_interface/landinterface.c": (
            'Commands.Surrender.note\t\t= "Сдаться";',
            'case "BI_Surrender":',
            "Custody_FindCombatCaptor() >= 0",
        ),
        "PROGRAM/battle_interface/loginterface.c": (
            'case "Surrender": bEC = Custody_TryCombatSurrender();',
        ),
        "PROGRAM/scripts/bountyhunters.c": (
            "LAi_SetFightMode(pchar, false);",
        ),
        "PROGRAM/scripts/colony.c": (
            "LAi_LockFightMode(Pchar, false);",
            "LAi_SetFightMode(Pchar, true);",
        ),
        "PROGRAM/Loc_ai/LAi_utilites.c": (
            "chr.DialogAmbushAllowed = true;",
            "Custody_RehydrateJail(loc);",
        ),
    }
    for relative_path, markers in required.items():
        text = outputs[relative_path].decode("utf-8")
        for marker in markers:
            if marker not in text:
                raise RuntimeError(f"{relative_path}: missing cumulative marker {marker!r}")


def verify(root: Path) -> int:
    state, _ = classify(root)
    if state not in {"original", "patched", "legacy", "upgrade"}:
        raise RuntimeError(f"refusing verify: aggregate state is {state}")
    originals = _read_originals(root, state)
    first = _build_outputs(originals)
    second = _build_outputs(originals)
    if first != second:
        raise RuntimeError("cumulative transform is not deterministic")
    _verify_cross_feature_contracts(first)
    print(f"PASS: {len(first)} exact cumulative transforms and linked fixtures")
    for relative_path in sorted(first):
        print(f"verified    {sha256(first[relative_path])}  {relative_path}")
    engine = _engine_hash(root)
    label = "supported" if engine in {SUPPORTED_ENGINE_SHA256, SUPPORTED_NATIVE_SHA256} else "unsupported for apply"
    print(f"engine: {label} {engine}")
    return 0


def apply_suite(root: Path) -> str:
    if root.resolve() == NATIVE_ROOT.resolve():
        return apply_native_suite(root)
    raise RuntimeError("current gameplay delivery is native-storm only; Windows and Metal are not updated")


def apply_native_suite(root: Path) -> str:
    """Update only the verified native copy; never copy saves or touch Metal."""
    if root.resolve() != NATIVE_ROOT.resolve():
        raise RuntimeError("not the native-storm runtime")
    _require_engine(root)
    holders = subprocess.run(["/usr/sbin/lsof", "-t", "--", str(NATIVE_ENGINE)],
                             capture_output=True, text=True, check=False)
    if holders.returncode != 1 or holders.stdout or holders.stderr:
        raise RuntimeError("native-storm must be closed before staging")
    state, _ = classify(root)
    if state == "patched":
        return "already patched"
    if state != "upgrade":
        raise RuntimeError(f"refusing native upgrade: {state}")
    outputs = _build_outputs(_read_originals(root, state))
    _verify_cross_feature_contracts(outputs)
    previous = {p: (root / p).read_bytes() for p in outputs}
    from patch_mod_journal import BASE_SHA256
    for p, digest in BASE_SHA256.items():
        if p in COMPOSITE_SHA256:
            continue
        snapshot = NATIVE_ROOT.parent / "journal-baseline" / p
        if not snapshot.exists():
            if sha256(previous[p]) != digest:
                raise RuntimeError(f"journal original changed: {p}")
            atomic_write(snapshot, previous[p])
    changed = [p for p in outputs if previous[p] != outputs[p]]
    written = []
    try:
        for p in changed:
            if (root / p).read_bytes() != previous[p]:
                raise RuntimeError(f"concurrent runtime change: {p}")
            atomic_write(root / p, outputs[p])
            written.append(p)
        if classify(root)[0] != "patched":
            raise RuntimeError("native post-apply hash mismatch")
    except BaseException:
        for p in reversed(written):
            atomic_write(root / p, previous[p])
        raise
    return f"native-storm patched ({len(changed)} files; saves unchanged)"


def revert_suite(root: Path) -> str:
    if root.resolve() != TARGET_ROOT.resolve():
        raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
    with lifecycle_lock():
        require_idle()
        state, _ = classify(root)
        if state == "original":
            return "already original"
        if state != "patched":
            raise RuntimeError(f"refusing revert: aggregate state is {state}")
        originals = _read_originals(root, state)
        patched = {
            relative_path: (root / relative_path).read_bytes()
            for relative_path in sorted(COMPOSITE_SHA256)
        }

        written: list[str] = []
        try:
            for relative_path in sorted(originals):
                atomic_write(root / relative_path, originals[relative_path])
                written.append(relative_path)
            for relative_path in sorted(CREATED_FILES):
                (root / relative_path).unlink(missing_ok=True)
            final, _ = classify(root)
            if final != "original":
                raise RuntimeError(f"post-revert state is {final}")
        except BaseException:
            for relative_path in reversed(written):
                atomic_write(root / relative_path, patched[relative_path])
            for relative_path in sorted(CREATED_FILES):
                atomic_write(root / relative_path, patched[relative_path])
            raise
    return "original"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=NATIVE_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return print_status(root)
        if args.action == "verify":
            return verify(root)
        result = apply_suite(root) if args.action == "apply" else revert_suite(root)
        print(result)
        return print_status(root)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
