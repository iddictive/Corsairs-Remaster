#!/usr/bin/env python3
"""Apply or revert the verified ReCon world-map/sea-AI runtime fix set."""

from __future__ import annotations

import argparse
import hashlib
import os
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


TARGET_ROOT = Path(
    "/REQUIRED_EXTERNAL_INPUT/Library/Application Support/CrossOver/Bottles/GAMES/drive_c/Games/"
    "KVL-Codex-Test"
)
SNAPSHOT_ROOT = TARGET_ROOT / ".codex-runtime-fixes-snapshots" / "20260913-worldmap-audio-v1"
LEGACY_PATCHED_SHA256 = {
    "PROGRAM/worldmap/worldmap_encgen.c":
        "c12f2a7c84ecbddbace3ed450f4994af9cc1b5ec284bc2cafc75e34db2316cc1",
}


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]
    add_final_newline: bool = False


FILES = (
    FilePatch(
        "PROGRAM/worldmap/worldmap_globals.c",
        "30783f9c535a5bbfab5ac8a0bfbbb89adde18f2100e5a33436e12fbb02c1716c",
        "184ce121f96c9347ae8ea6c7304af77dee74cf15b117891c3f0e0b04f3ac6885",
        (
            (
                """void Map_ReleaseQuestEncounter(string characterID)
{
	//Просматриваем список создаваемых
	aref encs, at;
	string aname;
	makearef(encs, worldMap.addQuestEncounters);
	int num = GetAttributesNum(encs);
	for(int cnt = 0; cnt < num; cnt++)
	{
		for(int i = 0; i < num; i++)
		{
			at = GetAttributeN(encs, i);
			aname = GetAttributeName(at);
			aname = "addQuestEncounters." + aname + ".characterID";
			if(worldMap.(aname) == characterID)
			{
				DeleteAttribute(&worldMap, aname);
				break;
			}
		}
		if(i >= num)
		{
			break;
		}
	}
	//Просматриваем список созданных
	makearef(encs, worldMap.encounters);
	num = GetAttributesNum(encs);
	for(i = 0; i < num; i++)
	{
		aref enc = GetAttributeN(encs, i);
		if(CheckAttribute(enc, "quest.chrID") == 0) continue;
		if(enc.quest.chrID == characterID)
		{
			wdmDeleteLoginEncounter(GetAttributeName(enc));
		}
        num--; //fix
        i--;
	}
	worldMap.deleteUpdate = "";
}
""",
                """void Map_ReleaseQuestEncounter(string characterID)
{
	//Просматриваем список создаваемых
	aref encs, at;
	string aname;
	makearef(encs, worldMap.addQuestEncounters);
	int num = GetAttributesNum(encs);
	for(int i = num - 1; i >= 0; i--)
	{
		at = GetAttributeN(encs, i);
		aname = GetAttributeName(at);
		if(CheckAttribute(at, "characterID") && at.characterID == characterID)
		{
			DeleteAttribute(&worldMap, "addQuestEncounters." + aname);
		}
	}
	//Просматриваем список созданных
	makearef(encs, worldMap.encounters);
	num = GetAttributesNum(encs);
	for(i = num - 1; i >= 0; i--)
	{
		aref enc = GetAttributeN(encs, i);
		if(CheckAttribute(enc, "quest.chrID") == 0) continue;
		if(enc.quest.chrID == characterID)
		{
			wdmDeleteLoginEncounter(GetAttributeName(enc));
		}
	}
	worldMap.deleteUpdate = "";
	wdmReconcileQuestShipCounter();
}
""",
            ),
            ("\t// тест торнадор\n\tisTornado = 1;\n\t// конец теста\n", ""),
        ),
        True,
    ),
    FilePatch(
        "PROGRAM/worldmap/worldmap_encgen.c",
        "3552d387de3ef77680ba3fcfed81ac980602d33671da0bed97b7fda1e4135481",
        "f6615ecded6b52649a99e94d955c1a2fb53a29b5812a1aa32b67ffd8e05f5615",
        (
            (
                """//Random ships
void wdmShipEncounter(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)
{
	int numShips = wdmGetNumberShipEncounters();
	if( CheckAttribute(pchar,"worldmap.shipcounter") ) {
		numShips = numShips - sti(pchar.worldmap.shipcounter);
	}
	if( numShips < 0 )
	{
		trace("Warning! World map ship quantity < 0 : numShips = " + numShips);
		trace("pchar.worldmap.shipcounter = " + pchar.worldmap.shipcounter);
		numShips = 0;
	}
""",
                """//Random ships
int wdmReconcileQuestShipCounter()
{
	int questShips = 0;
	if(CheckAttribute(&worldMap, "encounters"))
	{
		aref encs;
		makearef(encs, worldMap.encounters);
		int num = GetAttributesNum(encs);
		for(int i = 0; i < num; i++)
		{
			aref enc = GetAttributeN(encs, i);
			if(CheckAttribute(enc, "quest")) questShips++;
		}
	}
	pchar.worldmap.shipcounter = questShips;
	return questShips;
}

void wdmShipEncounter(float dltTime, float playerShipX, float playerShipZ, float playerShipAY)
{
	int numShips = wdmGetNumberShipEncounters();
	int questShips = wdmReconcileQuestShipCounter();
	numShips = numShips - questShips;
	if( numShips < 0 )
	{
		numShips = 0;
	}
""",
            ),
            ("\tif(numShips < 8)\n", "\tif(numShips < 12)\n"),
        ),
        True,
    ),
    FilePatch(
        "PROGRAM/worldmap/worldmap.c",
        "1984c34053d6a716ff876ce80362ae84dfa710f55ed834db5d59d95ff33b6055",
        "bc1095c0209ad8b597edcd8754fb2e0f7116a6f178b5707765dcac7df9aa4e65",
        (
            (
                "\nvoid wdmSetNationFlag(int iNation)\n",
                """
void wdmPurgeStaleNationEncounters()
{
	if(!IsEntity(worldMap) || !CheckAttribute(&worldMap, "encounters")) return;

	aref encs;
	makearef(encs, worldMap.encounters);
	int num = GetAttributesNum(encs);
	bool needDeleteUpdate = false;
	for(int i = 0; i < num; i++)
	{
		aref enc = GetAttributeN(encs, i);
		if(CheckAttribute(enc, "quest")) continue;
		if(!CheckAttribute(enc, "type") || !CheckAttribute(enc, "nation") || !CheckAttribute(enc, "isEnemy")) continue;
		if(enc.type != "Follow" && enc.type != "Merchant") continue;

		bool shouldBeEnemy = GetNationRelation2MainCharacter(sti(enc.nation)) == RELATION_ENEMY;
		bool wasEnemy = sti(enc.isEnemy) != 0;
		if(shouldBeEnemy != wasEnemy)
		{
			wdmDeleteLoginEncounter(GetAttributeName(enc));
			needDeleteUpdate = true;
		}
	}
	if(needDeleteUpdate) worldMap.deleteUpdate = "";
}

void wdmSetNationFlag(int iNation)
""",
            ),
            (
                '\t\tSendMessage(&worldMap, "ll", MSG_WORLDMAP_SET_NATION_FLAG, Nation);\n',
                '\t\tSendMessage(&worldMap, "ll", MSG_WORLDMAP_SET_NATION_FLAG, Nation);\n\t\twdmPurgeStaleNationEncounters();\n',
            ),
        ),
        True,
    ),
    FilePatch(
        "PROGRAM/sea_ai/AIShip.c",
        "08d298143777d48888da6966ce16b0f761d21a0c671e5583a1d2025e6cf4fb31",
        "68bf42bb7974a43d436b8570aa0558f11f4cc07dd019e0f03ffd91e96a4f1f14",
        (
            (
                "\t        ref rCannon = GetCannonByType(sti(rCharacter.Ship.Cannons.Type));\n\t\t    float range = stf(rCannon.FireRange);\n",
                "\t        ref rCannon = GetCannonByType(sti(rCharacter.Ship.Cannons.Type));\n\t\t    float range = 550.0;\n\t\t    if(CheckAttribute(rCannon, \"FireRange\")) range = stf(rCannon.FireRange);\n",
            ),
            (
                """\tref\trCannon = GetCannonByType(sti(rCharacter.Ship.Cannons.Type));
\tfloat range = stf(rCannon.FireRange);
\tif (checkAttribute(rCharacter, "SeaAI.task.target") && rCharacter.SeaAI.task.target != "") //fix
\t{
\t\tint enemy = sti(rCharacter.SeaAI.task.target);
\t\tref rBaseShip = GetRealShip(GetCharacterShipType(rCharacter));
\t\t
\t\trCannon = GetCannonByType(sti(characters[enemy].Ship.Cannons.Type));
\t\tfloat TargetRange = stf(rCannon.FireRange);
""",
                """\tint iCannonType = sti(rCharacter.Ship.Cannons.Type);
\tif(iCannonType == CANNON_TYPE_NONECANNON) return -1;
\tref\trCannon = GetCannonByType(iCannonType);
\tfloat range = 550.0;
\tif(CheckAttribute(rCannon, "FireRange")) range = stf(rCannon.FireRange);
\tif (checkAttribute(rCharacter, "SeaAI.task.target") && rCharacter.SeaAI.task.target != "") //fix
\t{
\t\tint enemy = sti(rCharacter.SeaAI.task.target);
\t\tref rBaseShip = GetRealShip(GetCharacterShipType(rCharacter));
""",
            ),
        ),
    ),
    FilePatch(
        "RESOURCE/INI/aliases/sound_alias.ini",
        "9b76417a1c3ddf18fc335c5fcd56579f00334c1ebbe766e4adefd10dcd5683f9",
        "a1784ffe64748e5dd0aecffeb47eb519e907920c1428b02af9ccbe71bb211b63",
        (
            (
                """[shipbow]
minDistance = 0.0
maxDistance = 0.0
volume = 0.0
name = PEOPLE\\clothes1.wav

[female-citizen]
""",
                """[shipbow]
minDistance = 0.0
maxDistance = 0.0
volume = 0.0
name = PEOPLE\\clothes1.wav

[ship_bow]
minDistance = 0.0
maxDistance = 0.0
volume = 0.0
name = PEOPLE\\clothes1.wav

[female-citizen]
""",
            ),
        ),
    ),
)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def classify(path: Path, spec: FilePatch) -> str:
    if not path.is_file():
        return "missing"
    digest = sha256_bytes(path.read_bytes())
    if digest == spec.original_sha256:
        return "original"
    if digest == spec.patched_sha256:
        return "patched"
    if digest == LEGACY_PATCHED_SHA256.get(spec.relative_path):
        return "legacy-patched"
    return "unsupported"


def patched_bytes(source: bytes, spec: FilePatch) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
        new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
        if result.count(old_bytes) != 1:
            raise RuntimeError(f"{spec.relative_path}: expected block is not unique")
        result = result.replace(old_bytes, new_bytes, 1)
    if spec.add_final_newline and not result.endswith(newline):
        result += newline
    if sha256_bytes(result) != spec.patched_sha256:
        raise RuntimeError(f"{spec.relative_path}: generated patched hash does not match")
    return result


def atomic_write(path: Path, data: bytes) -> None:
    mode = path.stat().st_mode
    fd, temp_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temp_name, mode)
        os.replace(temp_name, path)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)


def verified_originals(snapshot_root: Path) -> dict[str, bytes]:
    originals = {}
    for spec in FILES:
        source = snapshot_root / spec.relative_path
        if not source.is_file():
            raise RuntimeError(f"snapshot file missing: {source}")
        data = source.read_bytes()
        if sha256_bytes(data) != spec.original_sha256:
            raise RuntimeError(f"snapshot hash mismatch: {source}")
        originals[spec.relative_path] = data
    return originals


def overall_state(root: Path) -> tuple[str, list[tuple[FilePatch, str]]]:
    rows = [(spec, classify(root / spec.relative_path, spec)) for spec in FILES]
    states = {state for _, state in rows}
    if len(states) == 1:
        overall = states.pop()
    elif all(
        state == ("legacy-patched" if spec.relative_path in LEGACY_PATCHED_SHA256 else "patched")
        for spec, state in rows
    ):
        overall = "legacy-patched"
    else:
        overall = "mixed"
    return overall, rows


def mutate(root: Path, action: str, snapshot_root: Path) -> str:
    state, _ = overall_state(root)
    wanted = "patched" if action == "apply" else "original"
    required = "original" if action == "apply" else "patched"
    if state == wanted:
        return f"already {wanted}"
    if state != required and not (action == "apply" and state == "legacy-patched"):
        raise RuntimeError(f"refusing {action}: aggregate state is {state}")

    originals = verified_originals(snapshot_root)
    before = {spec.relative_path: (root / spec.relative_path).read_bytes() for spec in FILES}
    desired = (
        {spec.relative_path: patched_bytes(originals[spec.relative_path], spec) for spec in FILES}
        if action == "apply"
        else originals
    )
    written: list[Path] = []
    try:
        for spec in FILES:
            path = root / spec.relative_path
            atomic_write(path, desired[spec.relative_path])
            written.append(path)
        final, _ = overall_state(root)
        if final != wanted:
            raise RuntimeError(f"post-{action} state is {final}")
    except Exception:
        for path in reversed(written):
            atomic_write(path, before[str(path.relative_to(root))])
        raise
    return wanted


def require_canonical_target(root: Path) -> None:
    if root.resolve() != TARGET_ROOT.resolve():
        raise RuntimeError(f"apply/revert target is hardcoded to {TARGET_ROOT}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            state, rows = overall_state(root)
            print(f"target: {root}")
            print(f"state:  {state}")
            for spec, item_state in rows:
                path = root / spec.relative_path
                digest = sha256_bytes(path.read_bytes()) if path.is_file() else "-"
                print(f"{item_state:11} {digest}  {spec.relative_path}")
            return 0 if state in {"original", "patched"} else 1

        require_canonical_target(root)
        print(mutate(root, args.action, SNAPSHOT_ROOT))
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
