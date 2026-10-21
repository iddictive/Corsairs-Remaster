#!/usr/bin/env python3
"""Own the native Metal graphics options record and its source layer.

The game UI serializes this record as ``metal_graphics`` in the Metal runtime
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
        'PROGRAM/interface/option_sl.c',
        'e34dcee24757681a00e544fe643fb530ee70e869e211c613e2905d219ab46bf2',
        '45933dde2fa45e50254a509af1d228e0f59fe955199c84b3085bec94b02b6af8',
        (('\tSaveSavedOptions(&gopt);\n}', '\tSaveSavedOptions(&gopt);\n\tSaveMetalGraphicsOptions();\n}'),
         ('\tSetCurentOptions(&gopt);\n}',
          '\tSetCurentOptions(&gopt);\n'
          '\tReadMetalGraphicsOptions();\n'
          '}\n'
          '\n'
          'void PrepareDefaultMetalGraphics(ref optref)\n'
          '{\n'
          '\toptref.dynamic_lighting = true;\n'
          '\toptref.modern_water = true;\n'
          '\toptref.modern_lighting = true;\n'
          '\toptref.cinematic = true;\n'
          '\toptref.light_shafts = true;\n'
          '\toptref.dynamic_sky = true;\n'
          '\toptref.antialiasing = false;\n'
          '\toptref.full_screen = true;\n'
          '\toptref.resolution = 2;\n'
          '}\n'
          '\n'
          'void ReadMetalGraphicsOptions()\n'
          '{\n'
          '\tobject gopt;\n'
          '\tPrepareDefaultMetalGraphics(&gopt);\n'
          '\tSendMessage(&GameInterface, "lsa", MSG_INTERFACE_LOADOPTIONS, "metal_graphics", gopt);\n'
          '\tSetMetalGraphicsOptions(&gopt);\n'
          '}\n'
          '\n'
          'void SaveMetalGraphicsOptions()\n'
          '{\n'
          '\tobject gopt;\n'
          '\tPrepareDefaultMetalGraphics(&gopt);\n'
          '\tGetMetalGraphicsOptions(&gopt);\n'
          '\tSendMessage(&GameInterface, "lsa", MSG_INTERFACE_SAVEOPTIONS, "metal_graphics", gopt);\n'
          '}\n'
          '\n'
          'void GetMetalGraphicsOptions(ref optref)\n'
          '{\n'
          '\toptref.dynamic_lighting = InterfaceStates.MetalDynamicLighting;\n'
          '\toptref.modern_water = InterfaceStates.MetalModernWater;\n'
          '\toptref.modern_lighting = InterfaceStates.MetalModernLighting;\n'
          '\toptref.cinematic = InterfaceStates.MetalCinematic;\n'
          '\toptref.light_shafts = InterfaceStates.MetalLightShafts;\n'
          '\toptref.dynamic_sky = InterfaceStates.MetalDynamicSky;\n'
          '\toptref.antialiasing = InterfaceStates.MetalAntialiasing;\n'
          '\toptref.full_screen = InterfaceStates.MetalFullScreen;\n'
          '\toptref.resolution = InterfaceStates.MetalResolution;\n'
          '}\n'
          '\n'
          'void SetMetalGraphicsOptions(ref optref)\n'
          '{\n'
          '\tInterfaceStates.MetalDynamicLighting = sti(optref.dynamic_lighting);\n'
          '\tInterfaceStates.MetalModernWater = sti(optref.modern_water);\n'
          '\tInterfaceStates.MetalModernLighting = sti(optref.modern_lighting);\n'
          '\tInterfaceStates.MetalCinematic = sti(optref.cinematic);\n'
          '\tInterfaceStates.MetalLightShafts = sti(optref.light_shafts);\n'
          '\tInterfaceStates.MetalDynamicSky = sti(optref.dynamic_sky);\n'
          '\tInterfaceStates.MetalAntialiasing = sti(optref.antialiasing);\n'
          '\tInterfaceStates.MetalFullScreen = sti(optref.full_screen);\n'
          '\tInterfaceStates.MetalResolution = sti(optref.resolution);\n'
          '\tif(sti(InterfaceStates.MetalResolution)<0 || sti(InterfaceStates.MetalResolution)>3) '
          'InterfaceStates.MetalResolution = 2;\n'
          '}\n')),
    ),
    FilePatch(
        'PROGRAM/interface/option_screen.c',
        '6ac2f588a58ea9cbbbdf4b1e500c8aa46f0cded1a41934b0816de540e57edd12',
        '4d90a9e34f449281318301f3c0ad12e20de8af312e3561d47ff1d5171e84ba27',
        (('void IReadVariableAfterInit()\n{\n', 'void IReadVariableAfterInit()\n{\n\tGetMetalGraphicsData();\n'),
         ('\tif( sNodName == "BTN_CONTROLS_DEFAULT" ) {\n\t\tRestoreDefaultKeys();\n\t\treturn;\n\t}\n',
          '\tif( sNodName == "BTN_CONTROLS_DEFAULT" ) {\n'
          '\t\tRestoreDefaultKeys();\n'
          '\t\treturn;\n'
          '\t}\n'
          '\tif( sNodName == "METAL_GRAPHICS_BTN" ) {\n'
          '\t\tXI_WindowDisable("MAIN_WINDOW", true);\n'
          '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", true);\n'
          '\t\tSetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");\n'
          '\t\treturn;\n'
          '\t}\n'
          '\tif( sNodName == "METAL_GRAPHICS_CLOSE" ) {\n'
          '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", false);\n'
          '\t\tXI_WindowDisable("MAIN_WINDOW", false);\n'
          '\t\tSetCurrentNode("METAL_GRAPHICS_BTN");\n'
          '\t\treturn;\n'
          '\t}\n'),
         ('}\n\nvoid procSlideChange()',
          '\tif( sNodName == "METAL_DYNAMIC_LIGHTING_CHECKBOX" ) InterfaceStates.MetalDynamicLighting = bBtnState;\n'
          '\tif( sNodName == "METAL_MODERN_WATER_CHECKBOX" ) InterfaceStates.MetalModernWater = bBtnState;\n'
          '\tif( sNodName == "METAL_MODERN_LIGHTING_CHECKBOX" ) InterfaceStates.MetalModernLighting = bBtnState;\n'
          '\tif( sNodName == "METAL_CINEMATIC_CHECKBOX" ) InterfaceStates.MetalCinematic = bBtnState;\n'
          '\tif( sNodName == "METAL_LIGHT_SHAFTS_CHECKBOX" ) InterfaceStates.MetalLightShafts = bBtnState;\n'
          '\tif( sNodName == "METAL_DYNAMIC_SKY_CHECKBOX" ) InterfaceStates.MetalDynamicSky = bBtnState;\n'
          '\tif( sNodName == "METAL_ANTIALIASING_CHECKBOX" ) InterfaceStates.MetalAntialiasing = bBtnState;\n'
          '\tif( sNodName == "METAL_FULLSCREEN_CHECKBOX" ) InterfaceStates.MetalFullScreen = bBtnState;\n'
          '\tif( sNodName == "METAL_RESOLUTION_RADIO" && bBtnState ) InterfaceStates.MetalResolution = nBtnIndex - '
          '1;\n'
          '}\n'
          '\n'
          'void procSlideChange()'),
         ('void SetAlwaysRun(bool bRun)',
          'void GetMetalGraphicsData()\n'
          '{\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_DYNAMIC_LIGHTING_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalDynamicLighting));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_MODERN_WATER_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalModernWater));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_MODERN_LIGHTING_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalModernLighting));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_CINEMATIC_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalCinematic));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_LIGHT_SHAFTS_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalLightShafts));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_DYNAMIC_SKY_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalDynamicSky));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_ANTIALIASING_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalAntialiasing));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_FULLSCREEN_CHECKBOX", 2, 1, '
          'sti(InterfaceStates.MetalFullScreen));\n'
          '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 2, '
          'sti(InterfaceStates.MetalResolution)+1, true);\n'
          '}\n'
          '\n'
          'void SetAlwaysRun(bool bRun)')),
    ),
    FilePatch(
        'RESOURCE/INI/interfaces/option_screen.ini',
        '28ec5c6417518342f061d67b0aed7fcd7ff66dcb78008f6385eb8a3888e175ad',
        '96cdc40c797aacb8dafa237f1158fc98dbebf69f0a2cc29727e1dee6328cc5e9',
        (('item = 55,TEXTBUTTON2,BTN_CANCEL\n',
          'item = 55,TEXTBUTTON2,BTN_CANCEL\nitem = 55,TEXTBUTTON2,METAL_GRAPHICS_BTN\n'),
         ('item = WINDOW,CHANGEKEY_WINDOW\n',
          'item = WINDOW,CHANGEKEY_WINDOW\n'
          'item = 100,FRAME,METAL_GRAPHICS_FRAME\n'
          'item = 105,FORMATEDTEXT,METAL_GRAPHICS_TITLE\n'
          'item = 105,FORMATEDTEXT,METAL_GRAPHICS_NOTE\n'
          'item = 105,TEXTBUTTON2,METAL_GRAPHICS_CLOSE\n'
          'item = 105,CHECKBUTTON,METAL_DYNAMIC_LIGHTING_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_MODERN_WATER_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_MODERN_LIGHTING_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_CINEMATIC_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_LIGHT_SHAFTS_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_DYNAMIC_SKY_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_ANTIALIASING_CHECKBOX\n'
          'item = 105,CHECKBUTTON,METAL_FULLSCREEN_CHECKBOX\n'
          'item = 105,RADIOBUTTON,METAL_RESOLUTION_RADIO\n'
          'item = WINDOW,METAL_GRAPHICS_WINDOW\n'),
         ('nodelist = '
          'SCROLL_CONTROLS,CONTROLS_LIST,TABBTN_SAILING_1ST,TABBTN_PRIMARY_LAND,TABBTN_SAILING_3RD,TABBTN_FIGHT_MODE\n',
          'nodelist = '
          'SCROLL_CONTROLS,CONTROLS_LIST,TABBTN_SAILING_1ST,TABBTN_PRIMARY_LAND,TABBTN_SAILING_3RD,TABBTN_FIGHT_MODE\n'
          'nodelist = METAL_GRAPHICS_BTN\n'),
         ('shadowColor = 0,0,0,0\n',
          'shadowColor = 0,0,0,0\n'
          '\n'
          '[METAL_GRAPHICS_BTN]\n'
          'command = click,event:eventBtnAction\n'
          'command = activate,event:eventBtnAction\n'
          'position = 650,61,780,80\n'
          'string = Metal Graphics\n'
          'glowoffset = 0,0\n'
          'pressPictureOffset = 2,2\n'),
         ('nodelist = CHANGEKEY_FRAME,CHANGEKEY_TEXT,KEY_CHOOSER\n',
          'nodelist = CHANGEKEY_FRAME,CHANGEKEY_TEXT,KEY_CHOOSER\n'
          '\n'
          '[METAL_GRAPHICS_WINDOW]\n'
          'show = 0\n'
          'nodelist = METAL_GRAPHICS_FRAME,METAL_GRAPHICS_TITLE,METAL_GRAPHICS_NOTE,METAL_GRAPHICS_CLOSE\n'
          'nodelist = '
          'METAL_DYNAMIC_LIGHTING_CHECKBOX,METAL_MODERN_WATER_CHECKBOX,METAL_MODERN_LIGHTING_CHECKBOX,METAL_CINEMATIC_CHECKBOX\n'
          'nodelist = '
          'METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_RESOLUTION_RADIO\n'
          '\n'
          '[METAL_GRAPHICS_FRAME]\n'
          'position = 120,90,680,500\n'
          'backimage = piclist=SITH_PAPIRUS, picname=papirus, color=(235,128,128,128)\n'
          '\n'
          '[METAL_GRAPHICS_TITLE]\n'
          'position = 170,110,630,140\n'
          'fontscale = 1.0\n'
          'lineSpace = 16\n'
          'valignment = 1\n'
          'string = Metal Graphics\n'
          '\n'
          '[METAL_GRAPHICS_NOTE]\n'
          'position = 170,140,630,185\n'
          'fontscale = 0.8\n'
          'lineSpace = 14\n'
          'valignment = 1\n'
          'string = Metal Graphics Restart Note\n'
          '\n'
          '[METAL_DYNAMIC_LIGHTING_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_DYNAMIC_LIGHTING_CHECKBOX\n'
          'position = 170,190,390,210\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Dynamic Shadows\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_MODERN_WATER_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_MODERN_WATER_CHECKBOX\n'
          'position = 410,190,630,210\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Modern Water\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_MODERN_LIGHTING_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_MODERN_LIGHTING_CHECKBOX\n'
          'position = 170,220,390,240\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Modern Lighting\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_CINEMATIC_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_CINEMATIC_CHECKBOX\n'
          'position = 410,220,630,240\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Cinematic Interior\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_LIGHT_SHAFTS_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_LIGHT_SHAFTS_CHECKBOX\n'
          'position = 170,250,390,270\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Light Shafts\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_DYNAMIC_SKY_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_DYNAMIC_SKY_CHECKBOX\n'
          'position = 410,250,630,270\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Dynamic Sky\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_ANTIALIASING_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_ANTIALIASING_CHECKBOX\n'
          'position = 170,280,390,300\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal FXAA\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_FULLSCREEN_CHECKBOX]\n'
          'command = click\n'
          'command = rclick,event:ShowInfo,select:METAL_FULLSCREEN_CHECKBOX\n'
          'position = 410,280,630,300\n'
          'alignment = left\n'
          'iconsize = 15,15\n'
          'individualpos = 1\n'
          'rect_textoffset = 25,0,0,0\n'
          'section1 = 0,0,Metal Fullscreen\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          '\n'
          '[METAL_RESOLUTION_RADIO]\n'
          'command = click\n'
          'command = upstep\n'
          'command = downstep\n'
          'command = rightstep\n'
          'command = leftstep\n'
          'position = 170,340,630,400\n'
          'alignment = left\n'
          'lineheight = 16\n'
          'rect_textoffset = 22,0,0,0\n'
          'iconsize = 16,16\n'
          'section1 = 0,0,Resolution 1280x720\n'
          'section2 = 0,0,Resolution 1600x900\n'
          'section3 = 0,0,Resolution 1920x1080\n'
          'section4 = 0,0,Resolution 2560x1440\n'
          'fontScale = 0.85\n'
          'bShowGlowCursor = 0\n'
          'individualpos = 1\n'
          'pos1 = 0,0\n'
          'pos2 = 240,0\n'
          'pos3 = 0,30\n'
          'pos4 = 240,30\n'
          '\n'
          '[METAL_GRAPHICS_CLOSE]\n'
          'command = click,event:eventBtnAction\n'
          'command = activate,event:eventBtnAction\n'
          'position = 510,450,630,480\n'
          'string = Close\n'
          'glowoffset = 0,0\n'
          'pressPictureOffset = 2,2\n')),
    ),
    FilePatch(
        'RESOURCE/INI/texts/russian/common.ini',
        'a84822877ba63cdfe6ceb0beb5fef8daa158a8fdbd9da73d0a650ff577260562',
        'b87972e3901cdcef4307db294da1325dece7e6be266906bc010ad9dde50eaf6a',
        (('string = Glow_descr,"Изменяет эффект свечения выводимого изображения."\n',
          'string = Glow_descr,"Изменяет эффект свечения выводимого изображения."\n'
          '\n'
          'string = Metal Graphics,"Графика"\n'
          'string = Metal Graphics Restart Note,"Сохраните кнопкой OK после закрытия окна. Применение — после '
          'перезапуска. Разрешение — для окна; полный экран — по размеру рабочего стола."\n'
          'string = Metal Dynamic Shadows,"Динамические тени"\n'
          'string = Metal Modern Water,"Современная вода"\n'
          'string = Metal Modern Lighting,"Освещение и затенение"\n'
          'string = Metal Cinematic Interior,"Свечение в помещениях"\n'
          'string = Metal Light Shafts,"Световые лучи"\n'
          'string = Metal Dynamic Sky,"Динамическое небо"\n'
          'string = Metal FXAA,"Сглаживание FXAA"\n'
          'string = Metal Fullscreen,"Полноэкранный режим"\n'
          'string = Resolution 1280x720,"1280 × 720"\n'
          'string = Resolution 1600x900,"1600 × 900"\n'
          'string = Resolution 1920x1080,"1920 × 1080"\n'
          'string = Resolution 2560x1440,"2560 × 1440"\n'
          'string = Close,"Закрыть"\n'),),
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


def initialize_record(runtime: Path) -> None:
    record = runtime / METAL_OPTIONS
    if record.exists():
        graphics_values(runtime)  # Reject malformed persisted input before launch.
        return
    values = dict(DEFAULTS)
    values["full_screen"], values["resolution"] = current_engine(runtime / "engine.ini")
    atomic_write(record, "".join(f"{key}={int(value)}\n" for key, value in values.items()).encode())


def launch_environment(runtime: Path, inherited: dict[str, str]) -> dict[str, str]:
    result = dict(inherited)
    values = graphics_values(runtime)
    for key, env_name in ENV_KEYS.items():
        # An explicit diagnostic override still wins; ordinary launches use UI.
        result.setdefault(env_name, str(int(values[key])))
    return result


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
    parser.add_argument("action", choices=("initialize", "launch"))
    parser.add_argument("runtime", nargs="?", type=Path, default=DEFAULT_RUNTIME)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    initialize_record(args.runtime)
    apply_engine(args.runtime)
    if args.action == "launch":
        if args.binary is None or not args.binary.is_file():
            parser.error("launch requires an existing --binary")
        binary = str(args.binary.resolve())
        os.execve(binary, [binary], launch_environment(args.runtime, dict(os.environ)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
