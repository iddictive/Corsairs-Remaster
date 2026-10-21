#!/usr/bin/env python3
"""Atomically install or revert the pending primary-KVL gameplay suite."""

from __future__ import annotations

import argparse
import os
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
SUPPORTED_ENGINE_SHA256 = (
    "861e4ba02752b2303058202e074d8d80d37c51b716ed475dc20e4adbb08c963f"
)

# These are hashes of the cumulative outputs, not the standalone component
# outputs. Keep the vector explicit so a changed provider or ordering cannot
# silently redefine the installed state.
COMPOSITE_SHA256 = {
    "PROGRAM/Loc_ai/LAi_fightparams.c": "5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",
    "PROGRAM/Loc_ai/LAi_monsters.c": "b1b957d11540d120e48c2fc1a5591d7ec24481fc8d3b7f363c68e8be1bb6ab11",
    "PROGRAM/Loc_ai/LAi_utilites.c": "b76b427775e1ad0555865bb6c05b3e04307b694f03d0812bc3c876b6ae5a0449",
    "PROGRAM/battle_interface/landinterface.c": "c398d4657d77ef129ba472623c7cb5cd98c2b949274b8ec498c26f0005821bef",
    "PROGRAM/battle_interface/loginterface.c": "88a187aec46bf600ff6c239eb2d358b8155775a006f70d085917fa7bc79a849c",
    "PROGRAM/characters/characterUtilite.c": "104e5a88131306aba33bfd07a11a79b71ddb945a1252a55579cf7c870f11d1dd",
    "PROGRAM/dialog.c": "1f0f4f918f1f35134478e1ba2c98c607492f045242e8e6c30cccdfb95a6b57ce",
    "PROGRAM/dialogs/russian/Common_Prison.c": "32bda5780e11dbb4f3db86ab0e6b3b846244f89b806df8d0322663d10ec30d58",
    "PROGRAM/dialogs/russian/Common_ItemTrader.c": "ccd0009666c9d65c0613d34022fa7df1b23036d7dee5c19d745143242b56b874",
    "PROGRAM/dialogs/russian/Common_Soldier.c": "def79074f7fabeae124fad723e464de70bd0f2af122a52502390c8626c39f22c",
    "PROGRAM/dialogs/russian/Common_Store.c": "b4d1dd473489ff0506e04466dbcdd61d235d891d61701e34232fff2837c01ab2",
    "PROGRAM/dialogs/russian/Common_Tavern.c": "d247238cead8cf02870e750010875dbf08ed21f219a6cd5f764ea3a40ddd1f76",
    "PROGRAM/dialogs/russian/Enc_Patrol.c": "a41e16383d66227e96d836722fbbadc60dcd45df7aa54d095cd482f0897e1e51",
    "PROGRAM/dialogs/russian/Hunter_dialog.c": "cc35537952d48ee0b713e49907ca2f5746d13c6faaf3a5d878404f21373446b5",
    "PROGRAM/interface/hirecrew.c": "13f66bb3ac70fa7d7a9edfa33235ec1941e9c824a1e1debad1ddd4efdee9eedb",
    "PROGRAM/interface/salary.c": "125a9604e6237fc2332dbfdd8c0ecdee9a22568ce0b288c82f484e7bf64abbc5",
    "PROGRAM/interface/ship.c": "c198c1df317cc9f2c179291fac71c7bf0c88d21207468ed871f41ec188aabe34",
    "PROGRAM/locations/locations_loader.c": "3d08e2cd3444e2547300df875e8bbdec52e132d767561c4e0a93fce4d71918e4",
    "PROGRAM/quests/quests.c": "4e3bbd97bfad571c631c3dcf17b0e6783279d652c78915b4a6f9149ec4654bdc",
    "PROGRAM/quests/quests_reaction.c": "b435b2fbc99346d06a7376b3f02de5fec306fa65da096099a0f092b971414514",
    "PROGRAM/scripts/GoldFleet.c": "1ea9fc5a3d95983fc6ac4b664de90541ae1324b60bca187666441ed134631c3e",
    "PROGRAM/scripts/Crew.c": "072f41ae931d3c19946dab7aca5da9fa7f85c76e14aeebd8836c102a9230b490",
    "PROGRAM/scripts/bountyhunters.c": "90e1ec28ea2ef5455f67a8097e33ce7b894713220f2d44a104ff1fed41235e8d",
    "PROGRAM/scripts/colony.c": "c3225c680846a904fd7a8228887aea9879f5fa4482772596b222ee2ee0380c3a",
    "PROGRAM/scripts/islandships.c": "bc03feaf048837187e22bad6fc7671b860de0fe565cd7dace027b76bc96cca20",
    "PROGRAM/scripts/time_events.c": "9da55205414ac74922b3ae9fb0d1cde6de7257566c5f84536da2bc0921566e43",
    "PROGRAM/sea_ai/AISeaGoods.c": "39050a04396461aaf2cb05d59d8d1269f67e48de2b75af5ccebcb76f8b44e162",
    "PROGRAM/sea_ai/AIShip.c": "96b1eaef43f818f1b5d5e25f89c2fae3ef74286d7d37102d65c3ea4057cfe187",
    "PROGRAM/sea_ai/Cabin.c": "9334dc64ffcc17ebe991a82a64f2c69e63b288791ea69b16ad6964e8d1df2271",
    "PROGRAM/sea_ai/sea.c": "107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1",
    "PROGRAM/seadogs.c": "22a0d8a04ed84aeaeba157a0106379d0ea1e76389d6e4d2a9a78536e1e1bd31a",
    "PROGRAM/scripts/custody.c": "bb4d9a6f3a393ee8683fbca333947d675383dff3c1692aab2323744378920b3f",
    "RESOURCE/INI/interfaces/ship.ini": "e5822481464d182b80c2e89318e02e6183ec8e3d934bfd3aa0b3e89148cc7b4a",
}

CREATED_FILES = {"PROGRAM/scripts/custody.c"}
LEGACY_SHOP_PATH = "PROGRAM/scripts/shop_rotation.c"
LEGACY_SHOP_SHA256 = "c4af5f1e97719a8010aebbf2b33a58c20f038a77d7b9cde3258387f5831cdccf"
SHOP_SEGMENT_SHA256 = "a97d9206f3e6c05226492ae7d71b62c9f679087720cfad1ce98d7f08ec248666"
PREVIOUS_SHA256 = {
    "PROGRAM/quests/quests_reaction.c": "99934a1d95f959714280dc366d611140fdb6f3d5ca90d19983e8a4ccab1176ec",
    "PROGRAM/scripts/colony.c": "ad03ea0623a4c367ed1a9698e15a01cd39078ab36ee8f0a69c5a750ea7b8f799",
    "PROGRAM/seadogs.c": "50248f0e85736c3e8ff35211db110dbb96fb206a7bad507106c6af89c7d15841",
    "PROGRAM/scripts/custody.c": "c75b9a3b3a605d6fa310ff5d632ad5ac57bfcb5c2402e32addf019a9651e7c3e",
}


def providers() -> tuple[tuple[str, tuple[FilePatch, ...]], ...]:
    """Return providers in one stable dependency and composition order."""
    from patch_cabin_sleep_weather import PATCH as cabin_patch
    from patch_crime_debt_suite import patch_sets as crime_debt_patch_sets
    from patch_land_encounter_combat_lock import FILES as land_files
    from patch_prison_surrender import FILES as prison_files

    result = [
        (label, tuple(patch_set.files))
        for label, patch_set in crime_debt_patch_sets()
    ]
    result.extend(
        (
            ("cabin sleep weather", tuple(cabin_patch.files)),
            ("land encounter combat lock", tuple(land_files)),
            ("prison surrender", tuple(prison_files)),
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
    bases = base_hashes()
    rows: list[tuple[str, str, str]] = []
    states: set[str] = set()
    for relative_path in sorted(COMPOSITE_SHA256):
        path = root / relative_path
        digest = sha256(path.read_bytes()) if path.is_file() else "-"
        if relative_path in CREATED_FILES and not path.exists():
            state = "original"
        elif relative_path in CREATED_FILES and digest == COMPOSITE_SHA256[relative_path]:
            state = "patched"
        elif relative_path in CREATED_FILES:
            state = "unsupported"
        elif digest == bases[relative_path]:
            state = "original"
        elif digest == COMPOSITE_SHA256[relative_path]:
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
        if all(digest == PREVIOUS_SHA256.get(path, COMPOSITE_SHA256[path]) for path, _, digest in rows):
            aggregate = "upgrade"
    return aggregate, rows


def _engine_hash(root: Path) -> str:
    engine = root / "engine.exe"
    return sha256(engine.read_bytes()) if engine.is_file() else "missing"


def _require_engine(root: Path) -> None:
    actual = _engine_hash(root)
    if actual != SUPPORTED_ENGINE_SHA256:
        raise RuntimeError(
            f"installed engine does not support the cumulative gameplay suite: {actual}"
        )


def print_status(root: Path) -> int:
    state, rows = classify(root)
    engine = _engine_hash(root)
    engine_state = "supported" if engine == SUPPORTED_ENGINE_SHA256 else "unsupported"
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
            path = root / relative_path
        else:
            snapshot = SNAPSHOT_ROOT / relative_path
            path = snapshot if snapshot.is_file() else root / relative_path
        if not path.is_file():
            raise RuntimeError(f"missing original source: {path}")
        data = path.read_bytes()
        if sha256(data) != bases[relative_path]:
            raise RuntimeError(f"original hash mismatch: {path}")
        originals[relative_path] = data
    return originals


def _build_outputs(originals: dict[str, bytes]) -> dict[str, bytes]:
    from patch_prison_surrender import CUSTODY_SEGMENT_BYTES, CUSTODY_SEGMENT_PATH

    outputs = {
        path: expected_bytes(path, originals[path], fragments)
        for path, fragments in file_fragments().items()
    }
    outputs[CUSTODY_SEGMENT_PATH] = CUSTODY_SEGMENT_BYTES
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
    from patch_dialog_ambush import _verify_dialog, _verify_utilities
    from patch_prison_surrender import _sentence, _standing

    _verify_dialog(outputs["PROGRAM/dialog.c"].decode("utf-8"))
    _verify_utilities(outputs["PROGRAM/Loc_ai/LAi_utilites.c"].decode("utf-8"))
    verify_debt_fixture()
    if _sentence(-5, False) != 3 or _sentence(100, True) != 11:
        raise RuntimeError("custody sentence fixture failed")
    if _standing(5, -18) != 0 or _standing(95, 14) != 100:
        raise RuntimeError("custody standing fixture failed")

    required = {
        "PROGRAM/quests/quests_reaction.c": (
            "if (bCabinStarted) bCabinSleepWeatherPending = true;",
            "LAi_LockFightMode(pchar, false);",
            'case "LandEnc_RaidersBegin":',
            'case "LandEnc_RapersTalk":',
            'case "LandEnc_PatrolBegin":',
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
            'pchar.quest.CustodyReleaseReservation.function = "Custody_ClearReleaseReservation";',
            'CheckAttribute(pchar, "GenQuest.CaptainComission")',
            'SetNationRelation2MainCharacter(nation, RELATION_NEUTRAL);',
            "bool Custody_TryCombatSurrender()",
            'captor.Dialog.CurrentNode = "Custody_CombatArrest";',
            'FindNearCharacters(pchar, 60.0, -1.0, -1.0, 0.01, false, true)',
            'CharactersVisibleTest(pchar, candidate)',
        ),
        "PROGRAM/seadogs.c": (
            '#include "scripts\\shop_rotation.c"',
            '#include "scripts\\custody.c"',
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
    label = "supported" if engine == SUPPORTED_ENGINE_SHA256 else "unsupported for apply"
    print(f"engine: {label} {engine}")
    return 0


def apply_suite(root: Path) -> str:
    if root.resolve() != TARGET_ROOT.resolve():
        raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
    with lifecycle_lock():
        require_idle()
        state, _ = classify(root)
        if state == "patched":
            return "already patched"
        if state not in {"original", "legacy", "upgrade"}:
            raise RuntimeError(f"refusing apply: aggregate state is {state}")
        _require_engine(root)
        originals = _read_originals(root, state)
        outputs = _build_outputs(originals)
        _verify_cross_feature_contracts(outputs)
        _snapshot(originals)

        written: list[str] = []
        legacy_shop = None
        try:
            for relative_path in sorted(outputs):
                atomic_write(root / relative_path, outputs[relative_path])
                written.append(relative_path)
            if state == "legacy":
                shop_path = root / LEGACY_SHOP_PATH
                legacy_shop = shop_path.read_bytes()
                clean_shop = SNAPSHOT_ROOT / LEGACY_SHOP_PATH
                clean_bytes = clean_shop.read_bytes()
                if sha256(clean_bytes) != SHOP_SEGMENT_SHA256:
                    raise RuntimeError("clean shop segment snapshot hash mismatch")
                atomic_write(shop_path, clean_bytes)
            final, _ = classify(root)
            if final != "patched":
                raise RuntimeError(f"post-apply state is {final}")
        except BaseException:
            for relative_path in reversed(written):
                if relative_path in CREATED_FILES:
                    (root / relative_path).unlink(missing_ok=True)
                else:
                    atomic_write(root / relative_path, originals[relative_path])
            if legacy_shop is not None:
                atomic_write(root / LEGACY_SHOP_PATH, legacy_shop)
            raise
    return "patched"


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
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
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
