#!/usr/bin/env python3
"""Own the native Metal graphics options record and its source layer.

The game UI serializes this record as ``metal_graphics`` in the global userdata
root.  This adapter deliberately does not read the legacy profile options file:
graphics and device settings must survive profile changes and a normal restart.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_RUNTIME = PROJECT / "experiments/native-metal/.cache/runtime"
METAL_OPTIONS = "metal_graphics"
RESOLUTIONS = ((1280, 720), (1600, 900), (1920, 1080), (2560, 1440))

DEFAULTS = {
    "dynamic_lighting": True,
    "modern_water": True,
    "modern_lighting": True,
    "cinematic": True,
    "light_shafts": True,
    "dynamic_sky": True,
    "antialiasing": False,
    "full_screen": True,
    "resolution": 2,
}

ENV_KEYS = {
    "dynamic_lighting": "STORM_METAL_DYNAMIC_LIGHTING",
    "modern_water": "STORM_METAL_MODERN_WATER",
    "modern_lighting": "STORM_METAL_MODERN_LIGHTING",
    "cinematic": "STORM_METAL_CINEMATIC",
    "light_shafts": "STORM_METAL_LIGHT_SHAFTS",
    "dynamic_sky": "STORM_METAL_DYNAMIC_SKY",
    "antialiasing": "STORM_METAL_ANTIALIASING",
}


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


# Filled after the first source transform.  The source hashes are pinned so a
# future Storm update cannot silently consume this layer.
FILES = (
    FilePatch(
        "PROGRAM/interface/option_sl.c",
        "e34dcee24757681a00e544fe643fb530ee70e869e211c613e2905d219ab46bf2",
        "e336a51d1e0abec5085af7521bd3bf6bcd3aa01f88cb1441b2b47d6b022b22d7",
        (
            (
                "\tSaveSavedOptions(&gopt);\n}",
                "\tSaveSavedOptions(&gopt);\n\tSaveMetalGraphicsOptions();\n}",
            ),
            (
                "\tSetCurentOptions(&gopt);\n}",
                "\tSetCurentOptions(&gopt);\n\tReadMetalGraphicsOptions();\n}\n\n"
                "void PrepareDefaultMetalGraphics(ref optref)\n{\n"
                "\toptref.dynamic_lighting = true;\n\toptref.modern_water = true;\n"
                "\toptref.modern_lighting = true;\n\toptref.cinematic = true;\n"
                "\toptref.light_shafts = true;\n\toptref.dynamic_sky = true;\n"
                "\toptref.antialiasing = false;\n\toptref.full_screen = true;\n"
                "\toptref.resolution = 2;\n}\n\n"
                "void ReadMetalGraphicsOptions()\n{\n\tobject gopt;\n"
                "\tPrepareDefaultMetalGraphics(&gopt);\n"
                "\tSendMessage(&GameInterface, \"lsa\", MSG_INTERFACE_LOADOPTIONS, \"metal_graphics\", gopt);\n"
                "\tSetMetalGraphicsOptions(&gopt);\n}\n\n"
                "void SaveMetalGraphicsOptions()\n{\n\tobject gopt;\n"
                "\tPrepareDefaultMetalGraphics(&gopt);\n\tGetMetalGraphicsOptions(&gopt);\n"
                "\tSendMessage(&GameInterface, \"lsa\", MSG_INTERFACE_SAVEOPTIONS, \"metal_graphics\", gopt);\n}\n\n"
                "void GetMetalGraphicsOptions(ref optref)\n{\n"
                "\toptref.dynamic_lighting = InterfaceStates.MetalDynamicLighting;\n"
                "\toptref.modern_water = InterfaceStates.MetalModernWater;\n"
                "\toptref.modern_lighting = InterfaceStates.MetalModernLighting;\n"
                "\toptref.cinematic = InterfaceStates.MetalCinematic;\n"
                "\toptref.light_shafts = InterfaceStates.MetalLightShafts;\n"
                "\toptref.dynamic_sky = InterfaceStates.MetalDynamicSky;\n"
                "\toptref.antialiasing = InterfaceStates.MetalAntialiasing;\n"
                "\toptref.full_screen = InterfaceStates.MetalFullScreen;\n"
                "\toptref.resolution = InterfaceStates.MetalResolution;\n}\n\n"
                "void SetMetalGraphicsOptions(ref optref)\n{\n"
                "\tInterfaceStates.MetalDynamicLighting = CheckAttribute(&optref, \"dynamic_lighting\") ? sti(optref.dynamic_lighting) : true;\n"
                "\tInterfaceStates.MetalModernWater = CheckAttribute(&optref, \"modern_water\") ? sti(optref.modern_water) : true;\n"
                "\tInterfaceStates.MetalModernLighting = CheckAttribute(&optref, \"modern_lighting\") ? sti(optref.modern_lighting) : true;\n"
                "\tInterfaceStates.MetalCinematic = CheckAttribute(&optref, \"cinematic\") ? sti(optref.cinematic) : true;\n"
                "\tInterfaceStates.MetalLightShafts = CheckAttribute(&optref, \"light_shafts\") ? sti(optref.light_shafts) : true;\n"
                "\tInterfaceStates.MetalDynamicSky = CheckAttribute(&optref, \"dynamic_sky\") ? sti(optref.dynamic_sky) : true;\n"
                "\tInterfaceStates.MetalAntialiasing = CheckAttribute(&optref, \"antialiasing\") ? sti(optref.antialiasing) : false;\n"
                "\tInterfaceStates.MetalFullScreen = CheckAttribute(&optref, \"full_screen\") ? sti(optref.full_screen) : true;\n"
                "\tInterfaceStates.MetalResolution = CheckAttribute(&optref, \"resolution\") ? sti(optref.resolution) : 2;\n"
                "\tif(sti(InterfaceStates.MetalResolution)<0 || sti(InterfaceStates.MetalResolution)>3) InterfaceStates.MetalResolution = 2;\n}\n",
            ),
        ),
    ),
    FilePatch(
        "PROGRAM/interface/option_screen.c",
        "6ac2f588a58ea9cbbbdf4b1e500c8aa46f0cded1a41934b0816de540e57edd12",
        "299a6048a17a814535aff9940af6720eef5013081a7bbb4c41ac91da9ec8ae0c",
        (
            ("\tGetControlsStatesData();\n}", "\tGetControlsStatesData();\n\tGetMetalGraphicsData();\n}"),
            (
                "\tif( sNodName == \"BTN_CONTROLS_DEFAULT\" ) {\n\t\tRestoreDefaultKeys();\n\t\treturn;\n\t}\n",
                "\tif( sNodName == \"BTN_CONTROLS_DEFAULT\" ) {\n\t\tRestoreDefaultKeys();\n\t\treturn;\n\t}\n"
                "\tif( sNodName == \"METAL_GRAPHICS_BTN\" ) {\n\t\tXI_WindowDisable(\"MAIN_WINDOW\", true);\n"
                "\t\tXI_WindowShow(\"METAL_GRAPHICS_WINDOW\", true);\n\t\tSetCurrentNode(\"METAL_DYNAMIC_LIGHTING_CHECKBOX\");\n\t\treturn;\n\t}\n"
                "\tif( sNodName == \"METAL_GRAPHICS_CLOSE\" ) {\n\t\tXI_WindowShow(\"METAL_GRAPHICS_WINDOW\", false);\n"
                "\t\tXI_WindowDisable(\"MAIN_WINDOW\", false);\n\t\tSetCurrentNode(\"METAL_GRAPHICS_BTN\");\n\t\treturn;\n\t}\n",
            ),
            (
                "\t\tInterfaceStates.EnabledAutoSaveMode = bBtnState;\n\t\t}\n\t}\n}\n",
                "\t\tInterfaceStates.EnabledAutoSaveMode = bBtnState;\n\t\t}\n\t}\n\n"
                "\tif( sNodName == \"METAL_DYNAMIC_LIGHTING_CHECKBOX\" ) InterfaceStates.MetalDynamicLighting = bBtnState;\n"
                "\tif( sNodName == \"METAL_MODERN_WATER_CHECKBOX\" ) InterfaceStates.MetalModernWater = bBtnState;\n"
                "\tif( sNodName == \"METAL_MODERN_LIGHTING_CHECKBOX\" ) InterfaceStates.MetalModernLighting = bBtnState;\n"
                "\tif( sNodName == \"METAL_CINEMATIC_CHECKBOX\" ) InterfaceStates.MetalCinematic = bBtnState;\n"
                "\tif( sNodName == \"METAL_LIGHT_SHAFTS_CHECKBOX\" ) InterfaceStates.MetalLightShafts = bBtnState;\n"
                "\tif( sNodName == \"METAL_DYNAMIC_SKY_CHECKBOX\" ) InterfaceStates.MetalDynamicSky = bBtnState;\n"
                "\tif( sNodName == \"METAL_ANTIALIASING_CHECKBOX\" ) InterfaceStates.MetalAntialiasing = bBtnState;\n"
                "\tif( sNodName == \"METAL_FULLSCREEN_CHECKBOX\" ) InterfaceStates.MetalFullScreen = bBtnState;\n"
                "\tif( sNodName == \"METAL_RESOLUTION_RADIO\" && bBtnState ) InterfaceStates.MetalResolution = nBtnIndex - 1;\n}\n",
            ),
            (
                "void SetAlwaysRun(bool bRun)",
                "void GetMetalGraphicsData()\n{\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_DYNAMIC_LIGHTING_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalDynamicLighting));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_MODERN_WATER_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalModernWater));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_MODERN_LIGHTING_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalModernLighting));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_CINEMATIC_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalCinematic));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_LIGHT_SHAFTS_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalLightShafts));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_DYNAMIC_SKY_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalDynamicSky));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_ANTIALIASING_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalAntialiasing));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_FULLSCREEN_CHECKBOX\", 2, 1, sti(InterfaceStates.MetalFullScreen));\n"
                "\tSendMessage(&GameInterface,\"lslll\",MSG_INTERFACE_MSG_TO_NODE,\"METAL_RESOLUTION_RADIO\", 2, sti(InterfaceStates.MetalResolution)+1, true);\n}\n\n"
                "void SetAlwaysRun(bool bRun)",
            ),
        ),
    ),
    FilePatch(
        "RESOURCE/INI/interfaces/option_screen.ini",
        "28ec5c6417518342f061d67b0aed7fcd7ff66dcb78008f6385eb8a3888e175ad",
        "5f293f8b214386bdd3949acedcf4a0a21505ec1fb17461e4740e0526221f84a0",
        (
            ("item = 55,TEXTBUTTON2,BTN_CANCEL\n", "item = 55,TEXTBUTTON2,BTN_CANCEL\nitem = 55,TEXTBUTTON2,METAL_GRAPHICS_BTN\n"),
            ("item = WINDOW,CHANGEKEY_WINDOW\n", "item = WINDOW,CHANGEKEY_WINDOW\nitem = 100,FRAME,METAL_GRAPHICS_FRAME\nitem = 105,FORMATEDTEXT,METAL_GRAPHICS_TITLE\nitem = 105,FORMATEDTEXT,METAL_GRAPHICS_NOTE\nitem = 105,TEXTBUTTON2,METAL_GRAPHICS_CLOSE\nitem = 105,CHECKBUTTON,METAL_DYNAMIC_LIGHTING_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_MODERN_WATER_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_MODERN_LIGHTING_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_CINEMATIC_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_LIGHT_SHAFTS_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_DYNAMIC_SKY_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_ANTIALIASING_CHECKBOX\nitem = 105,CHECKBUTTON,METAL_FULLSCREEN_CHECKBOX\nitem = 105,RADIOBUTTON,METAL_RESOLUTION_RADIO\nitem = WINDOW,METAL_GRAPHICS_WINDOW\n"),
            ("nodelist = SCROLL_CONTROLS,CONTROLS_LIST,TABBTN_SAILING_1ST,TABBTN_PRIMARY_LAND,TABBTN_SAILING_3RD,TABBTN_FIGHT_MODE\n", "nodelist = SCROLL_CONTROLS,CONTROLS_LIST,TABBTN_SAILING_1ST,TABBTN_PRIMARY_LAND,TABBTN_SAILING_3RD,TABBTN_FIGHT_MODE\nnodelist = METAL_GRAPHICS_BTN\n"),
            ("shadowColor = 0,0,0,0\n", "shadowColor = 0,0,0,0\n\n[METAL_GRAPHICS_BTN]\ncommand = click,event:eventBtnAction\ncommand = activate,event:eventBtnAction\nposition = 650,61,780,80\nstring = Metal Graphics\nglowoffset = 0,0\npressPictureOffset = 2,2\n"),
            ("nodelist = CHANGEKEY_FRAME,CHANGEKEY_TEXT,KEY_CHOOSER\n", "nodelist = CHANGEKEY_FRAME,CHANGEKEY_TEXT,KEY_CHOOSER\n\n[METAL_GRAPHICS_WINDOW]\nshow = 0\nnodelist = METAL_GRAPHICS_FRAME,METAL_GRAPHICS_TITLE,METAL_GRAPHICS_NOTE,METAL_GRAPHICS_CLOSE\nnodelist = METAL_DYNAMIC_LIGHTING_CHECKBOX,METAL_MODERN_WATER_CHECKBOX,METAL_MODERN_LIGHTING_CHECKBOX,METAL_CINEMATIC_CHECKBOX\nnodelist = METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_RESOLUTION_RADIO\n\n[METAL_GRAPHICS_FRAME]\nposition = 120,90,680,500\nbackimage = piclist=SITH_PAPIRUS, picname=papirus, color=(235,128,128,128)\n\n[METAL_GRAPHICS_TITLE]\nposition = 170,110,630,140\nfontscale = 1.0\nlineSpace = 16\nvalignment = 1\nstring = Metal Graphics\n\n[METAL_GRAPHICS_NOTE]\nposition = 170,140,630,175\nfontscale = 0.8\nlineSpace = 14\nvalignment = 1\nstring = Metal Graphics Restart Note\n\n[METAL_DYNAMIC_LIGHTING_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_DYNAMIC_LIGHTING_CHECKBOX\nposition = 170,190,390,210\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Dynamic Shadows\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_MODERN_WATER_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_MODERN_WATER_CHECKBOX\nposition = 410,190,630,210\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Modern Water\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_MODERN_LIGHTING_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_MODERN_LIGHTING_CHECKBOX\nposition = 170,220,390,240\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Modern Lighting\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_CINEMATIC_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_CINEMATIC_CHECKBOX\nposition = 410,220,630,240\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Cinematic Interior\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_LIGHT_SHAFTS_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_LIGHT_SHAFTS_CHECKBOX\nposition = 170,250,390,270\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Light Shafts\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_DYNAMIC_SKY_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_DYNAMIC_SKY_CHECKBOX\nposition = 410,250,630,270\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Dynamic Sky\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_ANTIALIASING_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_ANTIALIASING_CHECKBOX\nposition = 170,280,390,300\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal FXAA\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_FULLSCREEN_CHECKBOX]\ncommand = click\ncommand = rclick,event:ShowInfo,select:METAL_FULLSCREEN_CHECKBOX\nposition = 410,280,630,300\nalignment = left\niconsize = 15,15\nindividualpos = 1\nrect_textoffset = 25,0,0,0\nsection1 = 0,0,Metal Fullscreen\nfontScale = 0.85\nbShowGlowCursor = 0\n\n[METAL_RESOLUTION_RADIO]\ncommand = click\ncommand = upstep\ncommand = downstep\ncommand = rightstep\ncommand = leftstep\nposition = 170,330,630,370\nalignment = left\nlineheight = 8\niconsize = 16,16\nsection1 = 0,0,Resolution 1280x720\nsection2 = 0,0,Resolution 1600x900\nsection3 = 0,0,Resolution 1920x1080\nsection4 = 0,0,Resolution 2560x1440\nfontScale = 0.85\nbShowGlowCursor = 0\nindividualpos = 1\npos1 = 0,0\npos2 = 115,0\npos3 = 230,0\npos4 = 345,0\n\n[METAL_GRAPHICS_CLOSE]\ncommand = click,event:eventBtnAction\ncommand = activate,event:eventBtnAction\nposition = 510,450,630,480\nstring = Close\nglowoffset = 0,0\npressPictureOffset = 2,2\n"),
        ),
    ),
    FilePatch(
        "RESOURCE/INI/texts/russian/common.ini",
        "a84822877ba63cdfe6ceb0beb5fef8daa158a8fdbd9da73d0a650ff577260562",
        "71c6991a8dceeae5923c494c16c00a8b684efedea2434063025f0c3b579e4e2b",
        (("string = Glow_descr,\"Изменяет эффект свечения выводимого изображения.\"\n", "string = Glow_descr,\"Изменяет эффект свечения выводимого изображения.\"\n\nstring = Metal Graphics,\"Графика Metal\"\nstring = Metal Graphics Restart Note,\"Настройки применяются после перезапуска. Разрешение работает в оконном режиме; в полноэкранном режиме используется размер рабочего стола.\"\nstring = Metal Dynamic Shadows,\"Динамические тени\"\nstring = Metal Modern Water,\"Современная вода\"\nstring = Metal Modern Lighting,\"Современное освещение и AO\"\nstring = Metal Cinematic Interior,\"Кинематографический свет в интерьерах\"\nstring = Metal Light Shafts,\"Световые лучи\"\nstring = Metal Dynamic Sky,\"Динамическое небо\"\nstring = Metal FXAA,\"Сглаживание FXAA\"\nstring = Metal Fullscreen,\"Полноэкранный режим\"\nstring = Resolution 1280x720,\"1280 × 720\"\nstring = Resolution 1600x900,\"1600 × 900\"\nstring = Resolution 1920x1080,\"1920 × 1080\"\nstring = Resolution 2560x1440,\"2560 × 1440\"\nstring = Close,\"Закрыть\"\n"),),
    ),
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_record(path: Path) -> dict[str, str]:
    """Parse the engine's line-oriented key=value record without evaluation."""
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    for line_number, raw in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        if "=" not in line:
            raise ValueError(f"{path}: malformed line {line_number}")
        key, value = (part.strip() for part in line.split("=", 1))
        if not re.fullmatch(r"[a-z][a-z0-9_]*", key):
            raise ValueError(f"{path}: unsupported key on line {line_number}")
        if not re.fullmatch(r"(?:0|1|true|false|-?[0-9]+)", value, re.IGNORECASE):
            raise ValueError(f"{path}: unsupported value on line {line_number}")
        values[key] = value.lower()
    return values


def bool_value(values: dict[str, str], key: str) -> bool:
    raw = values.get(key)
    if raw is None:
        return DEFAULTS[key]
    if raw in {"1", "true"}:
        return True
    if raw in {"0", "false"}:
        return False
    raise ValueError(f"{key}: expected a boolean")


def graphics_values(runtime: Path) -> dict[str, int | bool]:
    values = parse_record(runtime / METAL_OPTIONS)
    result: dict[str, int | bool] = {
        key: bool_value(values, key) for key in ENV_KEYS
    }
    result["full_screen"] = bool_value(values, "full_screen")
    resolution = int(values.get("resolution", DEFAULTS["resolution"]))
    if resolution < 0 or resolution >= len(RESOLUTIONS):
        raise ValueError("resolution: unsupported preset")
    result["resolution"] = resolution
    return result


def current_engine(engine_ini: Path) -> tuple[bool, int]:
    values: dict[str, str] = {}
    if engine_ini.is_file():
        for line in engine_ini.read_text(encoding="utf-8-sig").splitlines():
            if "=" in line and not line.lstrip().startswith(("#", ";")):
                key, value = (part.strip() for part in line.split("=", 1))
                values[key] = value
    fullscreen = values.get("full_screen", "1") in {"1", "true", "True"}
    try:
        current = (int(values.get("screen_x", "1920")), int(values.get("screen_y", "1080")))
        resolution = min(range(len(RESOLUTIONS)), key=lambda i: abs(RESOLUTIONS[i][0] - current[0]) + abs(RESOLUTIONS[i][1] - current[1]))
    except ValueError:
        resolution = DEFAULTS["resolution"]
    return fullscreen, resolution


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = path.stat().st_mode if path.exists() else 0o644
    fd, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def apply_engine(runtime: Path) -> None:
    record = runtime / METAL_OPTIONS
    if not record.exists():
        return
    values = graphics_values(runtime)
    engine = runtime / "engine.ini"
    source = engine.read_text(encoding="utf-8-sig")
    fullscreen = "1" if values["full_screen"] else "0"
    width, height = RESOLUTIONS[int(values["resolution"])]
    replacements = {
        "full_screen": fullscreen,
        "screen_x": str(width),
        "screen_y": str(height),
    }
    for key, value in replacements.items():
        source, count = re.subn(rf"(?m)^{re.escape(key)}\s*=.*$", f"{key} = {value}", source)
        if count != 1:
            raise ValueError(f"{engine}: expected one {key} entry")
    atomic_write(engine, source.encode("utf-8"))


def emit_env(runtime: Path) -> None:
    values = graphics_values(runtime)
    for key, env_name in ENV_KEYS.items():
        print(f"export {env_name}={'1' if values[key] else '0'}")


def transform(source: bytes, spec: FilePatch) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    result = source
    for old, new in spec.replacements:
        old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
        new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
        if result.count(old_bytes) != 1:
            raise ValueError(f"{spec.relative_path}: expected source block is not unique")
        result = result.replace(old_bytes, new_bytes, 1)
    if spec.patched_sha256 and digest(result) != spec.patched_sha256:
        raise ValueError(f"{spec.relative_path}: generated hash is not reviewed")
    return result


def prepare(relative_path: str, source: bytes) -> tuple[bytes, str]:
    spec = next(item for item in FILES if item.relative_path == relative_path)
    current = digest(source)
    if current == spec.patched_sha256 and spec.patched_sha256:
        return source, "patched"
    if current != spec.original_sha256:
        raise ValueError(f"{relative_path}: unsupported source hash {current}")
    return transform(source, spec), "original"


def strip(relative_path: str, source: bytes) -> tuple[bytes, str]:
    spec = next(item for item in FILES if item.relative_path == relative_path)
    current = digest(source)
    if current == spec.original_sha256:
        return source, "original"
    if spec.patched_sha256 and current == spec.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(spec.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: reviewed layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != spec.original_sha256:
            raise ValueError(f"{relative_path}: stripped hash is not canonical")
        return result, "patched"
    raise ValueError(f"{relative_path}: unsupported source hash {current}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("apply", "env"))
    parser.add_argument("runtime", nargs="?", type=Path, default=DEFAULT_RUNTIME)
    args = parser.parse_args()
    if args.action == "apply":
        apply_engine(args.runtime)
    else:
        emit_env(args.runtime)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
