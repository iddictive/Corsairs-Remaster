#!/usr/bin/env python3
"""Reconcile sea rendering after time advances inside the ship cabin."""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, sha256, transform


SUPPORTED_ENGINE_SHA256 = "dc26f65aa9d0842051d71cafdaf382a7401dd1ce2f5c261f4e99ff4076664a31"


PATCH = PatchSet(
    TARGET_ROOT / ".codex-cabin-sleep-weather" / "20260913-v1",
    (
        FilePatch(
            "PROGRAM/quests/quests_reaction.c",
            "4db866742a88054417252ced1524180091811f7da33652d496c90e4585a0f21e",
            "6fb086c494d96fdeb00c38dc2bd7d9ad82e5ad466d85cce9cc43ac1945e69008",
            (
                (
                    '''\tRecalculateJumpTable();
\tWhr_UpdateWeather();
}''',
                    '''\tRecalculateJumpTable();
\tif (bCabinStarted) bCabinSleepWeatherPending = true;
\tWhr_UpdateWeather();
\tWhr_ChangeDayNight();
\tif (bCabinStarted) Sea_ReconcileCabinSleepEnvironment();
}''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/sea_ai/Cabin.c",
            "29375b8482603bb8871fd42688812664e9802c3a7dcf16c637b17772b9e43983",
            "9334dc64ffcc17ebe991a82a64f2c69e63b288791ea69b16ad6964e8d1df2271",
            (
                (
                    '''bool\tbCabinStarted = false;
bool\tbDeckBoatStarted = false;''',
                    '''bool\tbCabinStarted = false;
bool\tbDeckBoatStarted = false;
bool\tbCabinSleepWeatherPending = false;''',
                ),
                (
                    '''\tbDeckBoatStarted = false;
\tSea.AbordageMode = false;

\tInitBattleInterface();''',
                    '''\tbDeckBoatStarted = false;
\tSea.AbordageMode = false;

\tif (bCabinSleepWeatherPending)
\t{
\t\tbCabinSleepWeatherPending = false;
\t\tSea_ReconcileCabinSleepEnvironment();
\t}

\tInitBattleInterface();''',
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/sea_ai/sea.c",
            "e33333dede98163496e7d52d628ed32ad5d54a1883788ca0e88b56896937d063",
            "107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1",
            (
                (
                    '''int\tiSeaSectionLang = -1;

void DeleteSeaEnvironment()''',
                    '''int\tiSeaSectionLang = -1;

void Sea_ReconcileCabinSleepEnvironment()
{
\tif (!bSeaActive || !IsEntity(&Sea)) return;

\tWhrCreateSeaEnvironment();

\tstring lightPath = GetLightingPath();
\tif (IsEntity(&Island)) Island.LightingPath = lightPath;
\tif (IsEntity(&IslandReflModel))
\t{
\t\tSendMessage(&IslandReflModel, "ls", MSG_MODEL_SET_LIGHT_PATH, lightPath);
\t}
\tfor (int i = 0; i < iNumForts; i++)
\t{
\t\tif (IsEntity(&Forts[i])) SendMessage(&Forts[i], "ls", MSG_MODEL_SET_LIGHT_PATH, lightPath);
\t}

\taref currentWeather = GetCurrentWeather();
\tdoShipLightChange(currentWeather);
}

void DeleteSeaEnvironment()''',
                ),
            ),
        ),
    ),
)


def check(root: Path) -> int:
    state, rows = PATCH.overall_state(root)
    if state not in {"original", "patched"}:
        PATCH.print_status(root)
        return 1

    for spec, item_state in rows:
        data = (root / spec.relative_path).read_bytes()
        candidate = transform(data, spec) if item_state == "original" else data
        if sha256(candidate) != spec.patched_sha256:
            raise RuntimeError(f"{spec.relative_path}: candidate hash mismatch")

    print("cabin sleep weather patch: verified")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("status", "check", "apply", "revert"))
    args = parser.parse_args()

    try:
        if args.action == "status":
            return PATCH.print_status(TARGET_ROOT)
        if args.action == "check":
            return check(TARGET_ROOT)
        if args.action == "apply":
            engine = TARGET_ROOT / "engine.exe"
            engine_sha = hashlib.sha256(engine.read_bytes()).hexdigest() if engine.is_file() else "missing"
            if engine_sha != SUPPORTED_ENGINE_SHA256:
                raise RuntimeError(
                    f"installed engine does not support live lighting rebind: {engine_sha}"
                )
            print(PATCH.apply(TARGET_ROOT))
        else:
            print(PATCH.revert(TARGET_ROOT))
        return PATCH.print_status(TARGET_ROOT)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
