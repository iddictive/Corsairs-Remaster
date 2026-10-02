"""Metal-only camera HUD layer over the frozen shared deck gameplay package."""
import hashlib

PATH = "PROGRAM/sea_ai/AICameras.c"
BASE = "3541f7a2e0a304240e067037d7daeffb1b29465f42c48415db7b4f08ce998889"
PREVIOUS_EDITS = (
    (b"Crosshair.OutsideCamera = SeaCameras_isCameraOutside();",
     b'Crosshair.OutsideCamera = SeaCameras_isCameraOutside() || (SeaCameras.Camera == "SeaDeckCamera" && sti(SeaDeckCamera.ThirdPerson) != 0);'),
    (b'SeaDeckCamera.ThirdPerson = 1;\r\n\t\t\t\tSeaCameras.Camera = "SeaDeckCamera";\r\n\t\t\t\tCrosshair.OutsideCamera = false;',
     b'SeaDeckCamera.ThirdPerson = 1;\r\n\t\t\t\tSeaCameras.Camera = "SeaDeckCamera";\r\n\t\t\t\tCrosshair.OutsideCamera = true;'),
    (b'SeaDeckCamera.ThirdPerson = 0;\r\n\t\t\t\tbreak;',
     b'SeaDeckCamera.ThirdPerson = 0;\r\n\t\t\t\tCrosshair.OutsideCamera = false;\r\n\t\t\t\tbreak;'),
)

# Sea_Load recreates the entities after restoring script attributes. Preserve the
# saved deck mode; engine DECK_CAMERA::Load does not serialize ThirdPerson.
EDITS = PREVIOUS_EDITS + (
    (b'\r\n\tSeaDeckCamera.ThirdPerson = 1;\r\n',
     b'\r\n\tif (!bSeaLoad || !CheckAttribute(&SeaDeckCamera, "ThirdPerson"))\r\n\t\tSeaDeckCamera.ThirdPerson = 1;\r\n'),
    (b'\tCrosshair.Texture = ',
     b'\tCrosshair.OutsideCamera = SeaCameras_isCameraOutside() || (SeaCameras.Camera == "SeaDeckCamera" && sti(SeaDeckCamera.ThirdPerson) != 0);\r\n\tCrosshair.Texture = '),
)



def digest(data):
    return hashlib.sha256(data).hexdigest()


def strip(data):
    if digest(data) == BASE:
        return data
    # Accept the prior reviewed adapter as well as the current one, so a script
    # update can safely migrate installed runtimes without changing frozen inputs.
    for edits in (EDITS, PREVIOUS_EDITS):
        restored = data
        for before, after in reversed(edits):
            if restored.count(after) != 1:
                break
            restored = restored.replace(after, before)
        else:
            if digest(restored) == BASE:
                return restored
    raise RuntimeError("unrecognized Metal deck camera source")


def prepare(data):
    result = strip(data)
    for before, after in EDITS:
        if result.count(before) != 1:
            raise RuntimeError("ambiguous Metal deck camera anchor")
        result = result.replace(before, after)
    return result
