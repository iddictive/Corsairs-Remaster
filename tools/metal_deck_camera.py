"""Metal-only camera HUD layer over the frozen shared deck gameplay package."""
import hashlib

PATH = "PROGRAM/sea_ai/AICameras.c"
BASE = "3541f7a2e0a304240e067037d7daeffb1b29465f42c48415db7b4f08ce998889"
EDITS = (
    (b"Crosshair.OutsideCamera = SeaCameras_isCameraOutside();",
     b'Crosshair.OutsideCamera = SeaCameras_isCameraOutside() || (SeaCameras.Camera == "SeaDeckCamera" && sti(SeaDeckCamera.ThirdPerson) != 0);'),
    (b'SeaDeckCamera.ThirdPerson = 1;\r\n\t\t\t\tSeaCameras.Camera = "SeaDeckCamera";\r\n\t\t\t\tCrosshair.OutsideCamera = false;',
     b'SeaDeckCamera.ThirdPerson = 1;\r\n\t\t\t\tSeaCameras.Camera = "SeaDeckCamera";\r\n\t\t\t\tCrosshair.OutsideCamera = true;'),
    (b'SeaDeckCamera.ThirdPerson = 0;\r\n\t\t\t\tbreak;',
     b'SeaDeckCamera.ThirdPerson = 0;\r\n\t\t\t\tCrosshair.OutsideCamera = false;\r\n\t\t\t\tbreak;'),
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def strip(data):
    if digest(data) == BASE:
        return data
    restored = data
    for before, after in reversed(EDITS):
        if restored.count(after) != 1:
            raise RuntimeError("unrecognized Metal deck camera revision")
        restored = restored.replace(after, before)
    if digest(restored) != BASE:
        raise RuntimeError("unrecognized Metal deck camera source")
    return restored


def prepare(data):
    result = strip(data)
    for before, after in EDITS:
        if result.count(before) != 1:
            raise RuntimeError("ambiguous Metal deck camera anchor")
        result = result.replace(before, after)
    return result
