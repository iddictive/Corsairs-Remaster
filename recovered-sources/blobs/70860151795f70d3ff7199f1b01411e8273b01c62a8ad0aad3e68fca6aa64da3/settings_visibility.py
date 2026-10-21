#!/usr/bin/env python3
"""Prepare the reviewed in-game hotkey visibility patch without touching a runtime."""

from __future__ import annotations

import argparse
import hashlib
import sys
from dataclasses import dataclass
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = PROJECT / "experiments/native-storm/.cache/runtime"


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


FILES = (
    FilePatch(
        "PROGRAM/controls/init_pc.c",
        "5eae6682e53a6fbc0b8737c9321809497df027637fb4eb3492b8bfaa9a88aa87",
        "0dc2d657dd2b729ee7547f161bb8a8bf4dd1a0c1638840d657f4162a8800a159",
        (
            (
                '\tCI_CreateAndSetControls( "", "Map_Best", CI_GetKeyCode("KEY_N"), 0, true); // Отличная карта\n'
                '\tCI_CreateAndSetControls( "", "MapView", CI_GetKeyCode("KEY_M"), 0, true); // Атлас карт',
                '\tCI_CreateAndSetControls( "", "Map_Best", CI_GetKeyCode("KEY_N"), 0, true); // Отличная карта\n'
                '\tMapControlToGroup("Map_Best", "PrimaryLand");\n'
                '\tCI_CreateAndSetControls( "", "MapView", CI_GetKeyCode("KEY_M"), 0, true); // Атлас карт\n'
                '\tMapControlToGroup("MapView", "PrimaryLand");',
            ),
            (
                '    CI_CreateAndSetControls( "", "TimeScaleFaster", CI_GetKeyCode("VK_A_PLUS"), 0, false );',
                '    CI_CreateAndSetControls( "", "TimeScaleFaster", CI_GetKeyCode("VK_A_PLUS"), 0, false );\n'
                '    MapControlToGroup("TimeScaleFaster", "PrimaryLand");',
            ),
            (
                '    CI_CreateAndSetControls( "", "TimeScaleSlower", CI_GetKeyCode("VK_A_MINUS"), 0, false );',
                '    CI_CreateAndSetControls( "", "TimeScaleSlower", CI_GetKeyCode("VK_A_MINUS"), 0, false );\n'
                '    MapControlToGroup("TimeScaleSlower", "PrimaryLand");',
            ),
            (
                '\tCI_CreateAndSetControls( "", "TimeScale", CI_GetKeyCode("KEY_R"), 0, false );',
                '\tCI_CreateAndSetControls( "", "TimeScale", CI_GetKeyCode("KEY_R"), 0, false );\n'
                '\tMapControlToGroup("TimeScale", "PrimaryLand");',
            ),
            (
                '\tCI_CreateAndSetControls( "", "QuickSave", CI_GetKeyCode("VK_F6"), 0, false );\n'
                '\tCI_CreateAndSetControls( "", "QuickLoad", CI_GetKeyCode("VK_F9"), 0, false );',
                '\tCI_CreateAndSetControls( "", "QuickSave", CI_GetKeyCode("VK_F6"), 0, false );\n'
                '\tMapControlToGroup("QuickSave", "PrimaryLand");\n'
                '\tCI_CreateAndSetControls( "", "QuickLoad", CI_GetKeyCode("VK_F9"), 0, false );\n'
                '\tMapControlToGroup("QuickLoad", "PrimaryLand");',
            ),
        ),
    ),
    FilePatch(
        "RESOURCE/INI/texts/russian/ControlsNames.txt",
        "a5fd8cb3e9e8968fa1810561e16b2ab05d9e6640a19dfd3da754edecc87323e2",
        "4698d9e1e68e954760d79d9322c53d0406969eacacbedc466ecedc79bf8a8d4a",
        (
            (
                "<==========================[ Управление на земле ]===========================>\n\n"
                "ChrCamTurnV {Вертикальное перемещение камеры за персонажем}",
                "<==========================[ Управление на земле ]===========================>\n\n"
                "Map_Best {Лучшая карта (если она есть в инвентаре)}\n"
                "MapView {Атлас карт (если он есть в инвентаре)}\n"
                "TimeScaleFaster {Ускорить время}\n"
                "TimeScaleSlower {Замедлить время}\n"
                "TimeScale {Переключить скорость времени}\n"
                "QuickSave {Быстрое сохранение}\n"
                "QuickLoad {Быстрая загрузка}\n"
                "ChrCamTurnV {Вертикальное перемещение камеры за персонажем}",
            ),
        ),
    ),
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform(source: bytes, spec: FilePatch) -> bytes:
    newline = "\r\n" if b"\r\n" in source else "\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline).encode("utf-8")
        new_bytes = new.replace("\n", newline).encode("utf-8")
        count = result.count(old_bytes)
        if count != 1:
            raise RuntimeError(
                f"{spec.relative_path}: expected source block is not unique ({count})"
            )
        result = result.replace(old_bytes, new_bytes, 1)
    if digest(result) != spec.patched_sha256:
        raise RuntimeError(
            f"{spec.relative_path}: generated hash {digest(result)} does not match review"
        )
    return result


def prepare(relative_path: str, source: bytes) -> tuple[bytes, str]:
    spec = next((item for item in FILES if item.relative_path == relative_path), None)
    if spec is None:
        raise RuntimeError(f"unsupported settings file: {relative_path}")
    current = digest(source)
    if current == spec.patched_sha256:
        return source, "patched"
    if current != spec.original_sha256:
        raise RuntimeError(f"{relative_path}: unsupported source hash {current}")
    return transform(source, spec), "original"


def verify(root: Path) -> None:
    for spec in FILES:
        path = root / spec.relative_path
        output, state = prepare(spec.relative_path, path.read_bytes())
        text = output.decode("utf-8-sig")
        if spec.relative_path.endswith("init_pc.c"):
            for control in (
                "Map_Best", "MapView", "TimeScaleFaster", "TimeScaleSlower",
                "TimeScale", "QuickSave", "QuickLoad",
            ):
                marker = f'MapControlToGroup("{control}", "PrimaryLand");'
                if text.count(marker) != 1:
                    raise RuntimeError(f"{control}: expected one visible group mapping")
            for control in ("DeckWalk_Forward", "DeckWalk_Backward", "DeckWalk_Left", "DeckWalk_Right", "DeckWalk_Run"):
                if text.count(f'"{control}"') != 1:
                    raise RuntimeError(f"{control}: deck binding changed or missing")
        else:
            for control in (
                "Map_Best", "MapView", "TimeScaleFaster", "TimeScaleSlower",
                "TimeScale", "QuickSave", "QuickLoad",
            ):
                if text.count(control + " {") != 1:
                    raise RuntimeError(f"{control}: expected one Russian label")
        print(f"{state:8} {digest(output)}  {spec.relative_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=DEFAULT_SOURCE)
    args = parser.parse_args()
    try:
        verify(args.root)
        print("settings visibility source transform passed")
        return 0
    except (OSError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
