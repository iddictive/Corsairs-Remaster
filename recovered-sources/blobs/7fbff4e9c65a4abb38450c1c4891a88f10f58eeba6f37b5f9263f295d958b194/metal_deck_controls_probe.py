#!/usr/bin/env python3
"""Falsify Metal deck-control ownership, restore and exact-input invariants."""

from pathlib import Path

import metal_deck_controls as layer


PROJECT = Path(__file__).resolve().parents[1]
SOURCE = PROJECT / "experiments/native-storm/.cache/runtime"
CANONICAL = (
    "ChrForward",
    "ChrBackward",
    "ChrStrafeLeft",
    "ChrStrafeRight",
    "ChrRun",
)
LEGACY = (
    "DeckWalk_Forward",
    "DeckWalk_Backward",
    "DeckWalk_Left",
    "DeckWalk_Right",
    "DeckWalk_Run",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    outputs = {}
    for spec in layer.FILES:
        source = (SOURCE / spec.relative_path).read_bytes()
        output = layer.prepare(spec.relative_path, source)
        require(output != source, f"{spec.relative_path}: layer added no delta")
        require(layer.strip(spec.relative_path, output) == source,
                f"{spec.relative_path}: exact layer did not round-trip")
        damaged = source[:-1] + bytes((source[-1] ^ 1,))
        try:
            layer.prepare(spec.relative_path, damaged)
        except RuntimeError as error:
            require("unsupported Metal deck-control input" in str(error),
                    f"{spec.relative_path}: unexpected altered-input error")
        else:
            raise RuntimeError(f"{spec.relative_path}: altered input was accepted")
        outputs[spec.relative_path] = output.decode("utf-8-sig")

    init = outputs["PROGRAM/controls/init_pc.c"]
    restore = outputs["PROGRAM/controls/controls.c"]
    for name in CANONICAL:
        require(init.count(f'MapControlToGroup("{name}","Sailing1Pers");') == 1,
                f"{name}: startup deck mapping missing")
        require(restore.count(f'MapControlToGroup("{name}", "Sailing1Pers");') == 1,
                f"{name}: post-restore deck mapping missing")
    for name in LEGACY:
        require(f'CI_CreateAndSetControls("Sailing1Pers", "{name}"' not in init,
                f"{name}: duplicate control is still created")
        require(restore.count(f'ctrlName == "{name}"') == 1,
                f"{name}: stale saved binding is not skipped exactly once")

    remap_at = restore.index('MapControlToGroup("ChrForward", "Sailing1Pers");')
    require(remap_at > restore.index("RebuildControlsRegistry();"),
            "canonical deck controls must be mapped after registry rebuild")
    require(remap_at < restore.index("RunControlsContainers();", remap_at),
            "canonical deck controls must be mapped before containers run")
    print("PASS: Metal deck uses canonical Chr controls before and after profile restore")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
