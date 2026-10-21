#!/usr/bin/env python3
"""Exact-input native deck controls; check/apply without touching SAVE or other runtimes."""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
RUNTIME = ROOT / '.cache/runtime'
ENGINE_SHA256 = 'fde51b3d7e95961975539a4d57e166ccaff347bb7a2ba4a193c7cfbe203c826f'
PREVIOUS = {
    'PROGRAM/controls/init_pc.c': '0273eec8a3e9b4779d57a01e31d131ce6ca90049e02033c429974975fa1d5888',
    'PROGRAM/controls/controls.c': '55db5aa1eb827e9819b5618aabaa96a9586de987baef1a609ba2a93972f19543',
    'RESOURCE/INI/texts/russian/ControlsNames.txt': '8e38c37fcedc2def1ac39706421126bafc271bedc68f364101b99c90e76e2afb',
}

SHIP_KEYS = '''void DeckWalk_DefaultShipKeys()
{
    CI_CreateAndSetControls("Sailing1Pers", "Ship_TurnLeft1", CI_GetKeyCode("VK_LEFT"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "Ship_TurnRight1", CI_GetKeyCode("VK_RIGHT"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "Ship_SailUp1", CI_GetKeyCode("VK_UP"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "Ship_SailDown1", CI_GetKeyCode("VK_DOWN"), 0, true);
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_TurnLeft");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_TurnRight");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_SailUp");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.Ship_SailDown");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.DeckCamera_Forward");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.DeckCamera_Backward");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.ShipCamera_Forward");
    DeleteAttribute(&objControlsState, "keygroups.Sailing1Pers.ShipCamera_Backward");
}

'''

WALK_KEYS = '''
    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Forward", CI_GetKeyCode("KEY_W"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Backward", CI_GetKeyCode("KEY_S"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Left", CI_GetKeyCode("KEY_A"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Right", CI_GetKeyCode("KEY_D"), 0, true);
    CI_CreateAndSetControls("Sailing1Pers", "DeckWalk_Run", CI_GetKeyCode("VK_SHIFT"), USE_AXIS_AS_BUTTON, true);
    DeckWalk_DefaultShipKeys();
'''

# Source hashes bind this transform to the reviewed native ReCon script corpus.
PATCHES = {
    'PROGRAM/controls/init_pc.c': ('7d713be496dbb4dc4766a4397e74727d9eba36459d6aa92c7370f1e635248d24', (
        ('\tMapControlToGroup("Ship_TurnLeft","Sailing1Pers");\n'
         '\tCI_CreateAndSetControls( "Sailing3Pers", "Ship_TurnRight", CI_GetKeyCode("KEY_D"), 0, true );\n'
         '\tMapControlToGroup("Ship_TurnRight","Sailing1Pers");\n'
         '\tCI_CreateAndSetControls( "Sailing3Pers", "Ship_SailUp", CI_GetKeyCode("KEY_W"), 0, true );\n'
         '\tMapControlToGroup("Ship_SailUp","Sailing1Pers");\n'
         '\tCI_CreateAndSetControls( "Sailing3Pers", "Ship_SailDown", CI_GetKeyCode("KEY_S"), 0, true );\n'
         '\tMapControlToGroup("Ship_SailDown","Sailing1Pers");',
         '\t// Keep WASD ship controls in the outside camera group. The deck group owns WASD walking.\n'
         '\tCI_CreateAndSetControls( "Sailing3Pers", "Ship_TurnRight", CI_GetKeyCode("KEY_D"), 0, true );\n'
         '\tCI_CreateAndSetControls( "Sailing3Pers", "Ship_SailUp", CI_GetKeyCode("KEY_W"), 0, true );\n'
         '\tCI_CreateAndSetControls( "Sailing3Pers", "Ship_SailDown", CI_GetKeyCode("KEY_S"), 0, true );'),
        ('\tMapControlToGroup("Sea_CameraSwitch","Sailing1Pers");',
         '\tMapControlToGroup("Sea_CameraSwitch","Sailing1Pers");\n' + WALK_KEYS),
    )),
    'PROGRAM/controls/controls.c': ('2e4bc6bebc678cff2e59d013501153c857fbba14ae0725901be0b49b92ff0cb2', (
        ('void RestoreKeysFromOptions(aref arControlsRoot)',
         SHIP_KEYS + 'void RestoreKeysFromOptions(aref arControlsRoot)'),
        ('\t\t\tCI_CreateAndSetControls( grName, ctrlName, keyCode, state, true );',
         '\t\t\t// Legacy deck ship names share global bindings with the outside view.\n'
         '\t\t\tif (grName == "sailing1pers" && (ctrlName == "Ship_TurnLeft" || '
         'ctrlName == "Ship_TurnRight" || ctrlName == "Ship_SailUp" || ctrlName == "Ship_SailDown" || '
         'ctrlName == "DeckCamera_Forward" || ctrlName == "DeckCamera_Backward" || '
         'ctrlName == "ShipCamera_Forward" || ctrlName == "ShipCamera_Backward"))\n'
         '\t\t\t{\n\t\t\t\tcontinue;\n\t\t\t}\n'
         '\t\t\tCI_CreateAndSetControls( grName, ctrlName, keyCode, state, true );'),
        ('\tRunControlsContainers();\n}',
         '\t// Old profiles used WASD for the ship on deck. Migrate only once.\n'
         '\tif (!CheckAttribute(arControlsRoot, "sailing1pers.Ship_TurnLeft1"))\n'
         '\t{\n\t\tDeckWalk_DefaultShipKeys();\n\t}\n\tRunControlsContainers();\n}'),
    )),
    'PROGRAM/sea_ai/AICameras.c': ('44b291303e34716c0aa655b455e25af88e54b4682212ff00cab08314a543cc3b', (
        ('\tLayerAddObject(SEA_EXECUTE, &SeaDeckCamera, iShipPriorityExecute + 5);',
         '\tLayerAddObject(SEA_EXECUTE, &SeaDeckCamera, iShipPriorityExecute + 5);\n'
         '\tLayerAddObject(SEA_REALIZE, &SeaDeckCamera, 65531);'),
        ('\tSeaDeckCamera.Perspective = 1.285;',
         '\tSeaDeckCamera.Perspective = 1.285;\n'
         '\tSeaDeckCamera.WalkMode = 1;\n\tSeaDeckCamera.ThirdPerson = 1;\n'
         '\tSeaDeckCamera.TelescopeActive = 0;'),
        ('\tint iTelescopeActive = GetEventData();',
         '\tint iTelescopeActive = GetEventData();\n'
         '\tSeaDeckCamera.TelescopeActive = iTelescopeActive;'),
        ('\t\t\t\tSeaCameras.Camera = "SeaDeckCamera";',
         '\t\t\t\tSeaDeckCamera.ThirdPerson = 1;\n'
         '\t\t\t\tSeaCameras.Camera = "SeaDeckCamera";'),
        ('\t\tcase "SeaDeckCamera":\n\t\t\t//SeaCameras.Camera',
         '\t\tcase "SeaDeckCamera":\n'
         '\t\t\tif (sti(SeaDeckCamera.ThirdPerson) != 0)\n'
         '\t\t\t{\n\t\t\t\tSeaDeckCamera.ThirdPerson = 0;\n\t\t\t\tbreak;\n\t\t\t}\n'
         '\t\t\t//SeaCameras.Camera'),
    )),
    'RESOURCE/INI/texts/russian/ControlsNames.txt': ('6fb996b38b3dc0032971cffd90e1367665e6272576339f5c69cc382933391bde', (
        ('DeckCamera_Forward {Вперёд (вид с палубы)}',
         'DeckCamera_Forward {Вперёд (вид с палубы)}\n'
         'DeckWalk_Forward {Шаг вперёд по палубе}\n'
         'DeckWalk_Backward {Шаг назад по палубе}\n'
         'DeckWalk_Left {Шаг влево по палубе}\n'
         'DeckWalk_Right {Шаг вправо по палубе}\n'
         'DeckWalk_Run {Бег по палубе}\n'
         'Ship_TurnLeft1 {Руль влево (на палубе)}\n'
         'Ship_TurnRight1 {Руль вправо (на палубе)}\n'
         'Ship_SailUp1 {Поднять паруса (на палубе)}\n'
         'Ship_SailDown1 {Убрать паруса (на палубе)}'),
    )),
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform(data: bytes, replacements: tuple, reverse: bool = False) -> bytes:
    newline = b'\r\n' if b'\r\n' in data else b'\n'
    pairs = reversed(replacements) if reverse else replacements
    for old, new in pairs:
        if reverse:
            old, new = new, old
        before = old.encode().replace(b'\n', newline)
        after = new.encode().replace(b'\n', newline)
        if data.count(before) != 1:
            raise RuntimeError('expected script block is not unique')
        data = data.replace(before, after, 1)
    return data


def prepare(path: str, source: bytes) -> tuple[bytes, str]:
    original_hash, replacements = PATCHES[path]
    if digest(source) == original_hash:
        return transform(source, replacements), 'original'
    try:
        original = transform(source, replacements, reverse=True)
    except RuntimeError:
        original = None
    if original is not None and digest(original) == original_hash and transform(original, replacements) == source:
        return source, 'installed'
    if digest(source) == PREVIOUS.get(path):
        original = (ROOT / '.cache/deck-walk-baseline' / path).read_bytes()
        if digest(original) != original_hash:
            raise RuntimeError(f'baseline mismatch: {path}')
        return transform(original, replacements), 'previous candidate'
    raise RuntimeError(f'unsupported script bytes: {path}')


def atomic_write(path: Path, data: bytes) -> None:
    fd, name = tempfile.mkstemp(prefix='.' + path.name, dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(name, path.stat().st_mode)
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('check', 'apply'))
    args = parser.parse_args()
    plan = []
    for relative in PATCHES:
        path = RUNTIME / relative
        before = path.read_bytes()
        after, state = prepare(relative, before)
        plan.append((path, before, after))
        print(f'{relative}: {state}')
    if args.action == 'check':
        return
    processes = subprocess.check_output(['ps', '-axo', 'comm='], text=True)
    executable = str(ROOT / '.cache/CorsairsNative.app/Contents/MacOS/native-engine')
    if executable in (line.strip() for line in processes.splitlines()):
        raise RuntimeError('close native-storm before installing deck scripts')
    if digest(Path(executable).read_bytes()) != ENGINE_SHA256:
        raise RuntimeError('install the matching deck-walk engine before applying scripts')
    # Reconcile all inputs before the first write; restore owned writes on error.
    for path, before, _ in plan:
        if path.read_bytes() != before:
            raise RuntimeError(f'concurrent change: {path}')
    written = []
    try:
        for path, before, after in plan:
            if before != after:
                atomic_write(path, after)
                written.append((path, before, after))
    except BaseException:
        for path, before, after in reversed(written):
            if path.read_bytes() == after:
                atomic_write(path, before)
        raise


if __name__ == '__main__':
    main()
