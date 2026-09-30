#!/usr/bin/env python3
"""Metal-only canonical deck-control layer over the frozen shared package."""

from __future__ import annotations

import hashlib
from dataclasses import dataclass


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


FILES = (
    FilePatch(
        "PROGRAM/controls/init_pc.c",
        "0dc2d657dd2b729ee7547f161bb8a8bf4dd1a0c1638840d657f4162a8800a159",
        "0df5597dc8376f0c6a6ff281eaca6ea0627fb6949ef9e0932275967ed90e513b",
        (
            (
                '    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Forward", CI_GetKeyCode("KEY_W"), 0, true);\n'
                '    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Backward", CI_GetKeyCode("KEY_S"), 0, true);\n'
                '    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Left", CI_GetKeyCode("KEY_A"), 0, true);\n'
                '    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Right", CI_GetKeyCode("KEY_D"), 0, true);\n'
                '    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Run", CI_GetKeyCode("VK_SHIFT"), USE_AXIS_AS_BUTTON, true);\n'
                '    DeckWalk_DefaultShipKeys();',
                '    DeckWalk_DefaultShipKeys();',
            ),
            *tuple(
                (
                    f'\tMapControlToGroup("{name}","BattleInterfaceControls");',
                    f'\tMapControlToGroup("{name}","BattleInterfaceControls");\n'
                    f'\tMapControlToGroup("{name}","Sailing1Pers");',
                )
                for name in (
                    "ChrForward",
                    "ChrBackward",
                    "ChrStrafeLeft",
                    "ChrStrafeRight",
                    "ChrRun",
                )
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/controls/controls.c",
        "b67d02dc59504d10d3c26b8181eccd3ee196871999556912e57dcb150ccdb90a",
        "5df48b6f34d709550c144f29cc3f3f5a1479baf17810e9f307c056243a677a23",
        (
            (
                '\t\t\tif (grName == "sailing1pers" && (ctrlName == "Ship_TurnLeft" || ctrlName == "Ship_TurnRight" || ctrlName == "Ship_SailUp" || ctrlName == "Ship_SailDown" || ctrlName == "DeckCamera_Forward" || ctrlName == "DeckCamera_Backward" || ctrlName == "ShipCamera_Forward" || ctrlName == "ShipCamera_Backward"))',
                '\t\t\tif (grName == "sailing1pers" && (ctrlName == "Ship_TurnLeft" || ctrlName == "Ship_TurnRight" || ctrlName == "Ship_SailUp" || ctrlName == "Ship_SailDown" || ctrlName == "DeckCamera_Forward" || ctrlName == "DeckCamera_Backward" || ctrlName == "ShipCamera_Forward" || ctrlName == "ShipCamera_Backward" || ctrlName == "DeckWalk_Forward" || ctrlName == "DeckWalk_Backward" || ctrlName == "DeckWalk_Left" || ctrlName == "DeckWalk_Right" || ctrlName == "DeckWalk_Run"))',
            ),
            (
                '\tif (!CheckAttribute(arControlsRoot, "sailing1pers.Ship_TurnLeft1"))\n'
                '\t{\n'
                '\t\tDeckWalk_DefaultShipKeys();\n'
                '\t}\n'
                '\tRunControlsContainers();',
                '\tif (!CheckAttribute(arControlsRoot, "sailing1pers.Ship_TurnLeft1"))\n'
                '\t{\n'
                '\t\tDeckWalk_DefaultShipKeys();\n'
                '\t}\n'
                '\t// The registry was rebuilt above; refresh the deck group\'s numeric IDs from canonical land controls.\n'
                '\tMapControlToGroup("ChrForward", "Sailing1Pers");\n'
                '\tMapControlToGroup("ChrBackward", "Sailing1Pers");\n'
                '\tMapControlToGroup("ChrStrafeLeft", "Sailing1Pers");\n'
                '\tMapControlToGroup("ChrStrafeRight", "Sailing1Pers");\n'
                '\tMapControlToGroup("ChrRun", "Sailing1Pers");\n'
                '\tRunControlsContainers();',
            ),
        ),
    ),
)

PATHS = frozenset(item.relative_path for item in FILES)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _spec(relative_path: str) -> FilePatch:
    spec = next((item for item in FILES if item.relative_path == relative_path), None)
    if spec is None:
        raise RuntimeError(f"unsupported Metal deck-control file: {relative_path}")
    return spec


def _transform(source: bytes, spec: FilePatch, *, reverse: bool = False) -> bytes:
    newline = "\r\n" if b"\r\n" in source else "\n"
    pairs = reversed(spec.replacements) if reverse else spec.replacements
    result = source
    for before, after in pairs:
        if reverse:
            before, after = after, before
        before_bytes = before.replace("\n", newline).encode("utf-8")
        after_bytes = after.replace("\n", newline).encode("utf-8")
        if result.count(before_bytes) != 1:
            raise RuntimeError(
                f"{spec.relative_path}: Metal deck-control anchor is not unique"
            )
        result = result.replace(before_bytes, after_bytes, 1)
    return result


def prepare(relative_path: str, source: bytes) -> bytes:
    spec = _spec(relative_path)
    current = digest(source)
    if current == spec.patched_sha256:
        return source
    if current != spec.original_sha256:
        raise RuntimeError(
            f"{relative_path}: unsupported Metal deck-control input {current}"
        )
    result = _transform(source, spec)
    if digest(result) != spec.patched_sha256:
        raise RuntimeError(f"{relative_path}: generated Metal deck-control hash changed")
    return result


def strip(relative_path: str, source: bytes) -> bytes:
    spec = _spec(relative_path)
    current = digest(source)
    if current == spec.original_sha256:
        return source
    if current != spec.patched_sha256:
        raise RuntimeError(
            f"{relative_path}: unsupported installed Metal deck-control hash {current}"
        )
    result = _transform(source, spec, reverse=True)
    if digest(result) != spec.original_sha256:
        raise RuntimeError(f"{relative_path}: stripped Metal deck-control hash changed")
    return result
