#!/usr/bin/env python3
"""Install the low-risk land-camera baseline for the mutable primary KVL runtime."""

from __future__ import annotations

import argparse
import sys

from runtime_script_patch import FilePatch, PatchSet, TARGET_ROOT, transform


PATCH = PatchSet(
    TARGET_ROOT / ".codex-third-person-camera" / "20260914-land-radius-v1",
    (
        FilePatch(
            "PROGRAM/locations/locations_camera.c",
            "85d04795d42b568559168a51938bb77edc0db0d4872d24ea19c6d8134a11c6b3",
            "b088cea1da9746e3f962cbb0e20d02d4276792ab78fe701c4164f9ed8b23ed31",
            (
                (
                    "#define DEFAULT_CAM_RAD 3.0\n"
                    "#define DEFAULT_CAM_RAD_DEN 0.22",
                    "#define DEFAULT_CAM_RAD_MIN 2.7\n"
                    "#define DEFAULT_CAM_RAD_RANGE 1.5",
                ),
                (
                    "float CalcLandRadius()\n"
                    "{\n"
                    "    float fCamRadAdj = 0.0;\n"
                    '    if(CheckAttribute(&InterfaceStates, "RadDetails"))\n'
                    "       fCamRadAdj = stf(InterfaceStates.RadDetails) / DEFAULT_CAM_RAD_DEN;\n"
                    "\n"
                    "    return (DEFAULT_CAM_RAD + fCamRadAdj);\n"
                    "}",
                    "float CalcLandRadius()\n"
                    "{\n"
                    "    float fCamRad = DEFAULT_CAM_RAD_MIN;\n"
                    '    if(CheckAttribute(&InterfaceStates, "RadDetails"))\n'
                    "       fCamRad += stf(InterfaceStates.RadDetails) * DEFAULT_CAM_RAD_RANGE;\n"
                    "\n"
                    "    return fCamRad;\n"
                    "}",
                ),
            ),
        ),
        FilePatch(
            "PROGRAM/interface/option_screen.c",
            "71f77cc7e956ca3e2cc08a672dd87cf59e2291069f98071f7631e92dc979fbfe",
            "6ac2f588a58ea9cbbbdf4b1e500c8aa46f0cded1a41934b0816de540e57edd12",
            (
                (
                    '            string sMsg = "#"+ FloatToString(fCamPersp, 1);\n'
                    '            SendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE, '
                    '"TITLES_STR", 1, 24, sMsg);',
                    '            string sMsg = "#"+ FloatToString(fCamPersp, 1);\n'
                    '            SendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE, '
                    '"TITLES_STR", 1, 24, sMsg);\n'
                    "            if(IsEntity(&locCamera))\n"
                    "            {\n"
                    '                SendMessage(&locCamera, "lf", MSG_CAMERA_SET_RADIUS, fCamPersp);\n'
                    "            }",
                ),
            ),
        ),
    ),
)


def verify() -> None:
    for spec in PATCH.files:
        source = (TARGET_ROOT / spec.relative_path).read_bytes()
        state = PATCH.overall_state(TARGET_ROOT)[0]
        if state == "original":
            output = transform(source, spec)
        elif state == "patched":
            output = source
        else:
            raise RuntimeError(f"cannot verify aggregate state: {state}")

        text = output.decode("utf-8-sig")
        if spec.relative_path.endswith("locations_camera.c"):
            assert "DEFAULT_CAM_RAD_MIN 2.7" in text
            assert "DEFAULT_CAM_RAD_RANGE 1.5" in text
            assert "stf(InterfaceStates.RadDetails) * DEFAULT_CAM_RAD_RANGE" in text
            assert "DEFAULT_CAM_RAD_DEN" not in text
        else:
            block = text[text.index("void ChangeRadDetail()") : text.index("void ChangeSoundSetting()")]
            assert "IsEntity(&locCamera)" in block
            assert 'MSG_CAMERA_SET_RADIUS, fCamPersp' in block


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    args = parser.parse_args()
    try:
        if args.action == "status":
            return PATCH.print_status(TARGET_ROOT)
        if args.action == "verify":
            verify()
            print("third-person camera patch verification passed")
            return 0
        if args.action == "apply":
            print(PATCH.apply(TARGET_ROOT))
            return 0
        print(PATCH.revert(TARGET_ROOT))
        return 0
    except (AssertionError, OSError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
