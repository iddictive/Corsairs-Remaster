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
CURRENT_RESOLUTION = len(RESOLUTIONS)
DESKTOP_RESOLUTION = CURRENT_RESOLUTION + 1
# Keep the old name as a source/probe compatibility alias.  Index 4 means the
# saved current/custom dimensions; index 5 is the new active-display mode.
CUSTOM_RESOLUTION = CURRENT_RESOLUTION
DEFAULT_RESOLUTION_WIDTH = 1920
DEFAULT_RESOLUTION_HEIGHT = 1080
MIN_RESOLUTION_WIDTH = 320
MIN_RESOLUTION_HEIGHT = 200
MAX_RESOLUTION_WIDTH = 16384
MAX_RESOLUTION_HEIGHT = 16384

DEFAULTS = {
    "dynamic_lighting": True,
    "shadow_quality": 1,
    "modern_water": True,
    "modern_lighting": True,
    "cinematic": True,
    "light_shafts": True,
    "dynamic_sky": True,
    "antialiasing": False,
    "full_screen": True,
    "resolution": 2,
    "resolution_width": DEFAULT_RESOLUTION_WIDTH,
    "resolution_height": DEFAULT_RESOLUTION_HEIGHT,
}

DISPLAY_MODE_CUSTOM = 0
DISPLAY_MODE_DESKTOP = 1

ENV_KEYS = {
    "dynamic_lighting": "STORM_METAL_DYNAMIC_LIGHTING",
    "modern_water": "STORM_METAL_MODERN_WATER",
    "modern_lighting": "STORM_METAL_MODERN_LIGHTING",
    "cinematic": "STORM_METAL_CINEMATIC",
    "light_shafts": "STORM_METAL_LIGHT_SHAFTS",
    "dynamic_sky": "STORM_METAL_DYNAMIC_SKY",
    "antialiasing": "STORM_METAL_ANTIALIASING",
}

INTEGER_ENV_KEYS = {
    "shadow_quality": "STORM_METAL_SHADOW_QUALITY",
}

SHADOW_QUALITY_MIN = 0
SHADOW_QUALITY_MAX = 2


@dataclass(frozen=True)
class FilePatch:
    relative_path: str
    original_sha256: str
    patched_sha256: str
    replacements: tuple[tuple[str, str], ...]


# Filled after the first source transform.  The source hashes are pinned so a
# future Storm update cannot silently consume this layer.
# This is the first reviewed layer. Keep it as the migration source while the
# current layer evolves; native-metal may still contain these exact hashes.
LEGACY_FILES = (
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
        'eadd057e4295837875af85fc2a031d49cc387277402e1805ffe971dccd2929e1',
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
          'position = 20,540,385,570\n'
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

# ``FILES`` is assigned below after the current transform adds the resolution
# upgrade. Keeping the old tuple intact lets strip/prepare migrate only this
# reviewed predecessor and reject every other runtime revision.

_CURRENT_EXTRAS = {
    "PROGRAM/interface/option_sl.c": (
        (
            "\toptref.resolution = 2;\n",
            "\toptref.resolution = 2;\n"
            "\toptref.resolution_width = 1920;\n"
            "\toptref.resolution_height = 1080;\n",
        ),
        (
            "\toptref.resolution = InterfaceStates.MetalResolution;\n",
            "\toptref.resolution = InterfaceStates.MetalResolution;\n"
            "\toptref.resolution_width = InterfaceStates.MetalResolutionWidth;\n"
            "\toptref.resolution_height = InterfaceStates.MetalResolutionHeight;\n",
        ),
        (
            "\tInterfaceStates.MetalResolution = sti(optref.resolution);\n"
            "\tif(sti(InterfaceStates.MetalResolution)<0 || sti(InterfaceStates.MetalResolution)>3) "
            "InterfaceStates.MetalResolution = 2;\n",
            "\tInterfaceStates.MetalResolution = sti(optref.resolution);\n"
            "\tInterfaceStates.MetalResolutionWidth = sti(optref.resolution_width);\n"
            "\tInterfaceStates.MetalResolutionHeight = sti(optref.resolution_height);\n"
            "\tif(sti(InterfaceStates.MetalResolution)<0 || sti(InterfaceStates.MetalResolution)>4) "
            "InterfaceStates.MetalResolution = 2;\n"
            "\tif(sti(InterfaceStates.MetalResolutionWidth)<320 || sti(InterfaceStates.MetalResolutionHeight)<200) {\n"
            "\t\tInterfaceStates.MetalResolutionWidth = 1920;\n"
            "\t\tInterfaceStates.MetalResolutionHeight = 1080;\n"
            "\t}\n",
        ),
    ),
    "PROGRAM/interface/option_screen.c": (
        (
            '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", true);\n'
            '\t\tSetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");',
            '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", true);\n'
            '\t\tXI_WindowDisable("METAL_GRAPHICS_WINDOW", false);\n'
            '\t\tSetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");',
        ),
        (
            '\tif( sNodName == "METAL_RESOLUTION_RADIO" && bBtnState ) InterfaceStates.MetalResolution = nBtnIndex - '
            '1;\n',
            '\tif( sNodName == "METAL_RESOLUTION_RADIO" && bBtnState ) {\n'
            '\t\tInterfaceStates.MetalResolution = nBtnIndex - 1;\n'
            '\t\tUpdateMetalResolutionLabels();\n'
            '\t}\n',
        ),
        (
            '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 2, '
            'sti(InterfaceStates.MetalResolution)+1, true);\n'
            '}\n\nvoid SetAlwaysRun(bool bRun)',
            '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 2, '
            'sti(InterfaceStates.MetalResolution)+1, true);\n'
            '\tUpdateMetalResolutionLabels();\n'
            '}\n\nvoid SetAlwaysRun(bool bRun)',
        ),
        (
            'void SetAlwaysRun(bool bRun)',
            'void UpdateMetalResolutionLabels()\n'
            '{\n'
            '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
            '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
            '\tint nWidth;\n'
            '\tint nHeight;\n'
            '\tif(nBaseWidth < 320 || nBaseHeight < 200) {\n'
            '\t\tnBaseWidth = 1920;\n'
            '\t\tnBaseHeight = 1080;\n'
            '\t}\n'
            '\tnWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 1, "" + nWidth + " x " + nHeight);\n'
            '\tnWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 2, "" + nWidth + " x " + nHeight);\n'
            '\tnWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 3, "" + nWidth + " x " + nHeight);\n'
            '\tnWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 4, "" + nWidth + " x " + nHeight);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 5, XI_ConvertString("Resolution Original") + ": " + nBaseWidth + " x " + nBaseHeight);\n'
            '}\n'
            '\n'
            'void SetAlwaysRun(bool bRun)',
        ),
    ),
    "RESOURCE/INI/interfaces/option_screen.ini": (
        (
            'position = 170,340,630,400\n'
            'alignment = left\n'
            'lineheight = 16\n',
            'position = 170,340,630,430\n'
            'alignment = left\n'
            'lineheight = 16\n',
        ),
        (
            'section4 = 0,0,Resolution 2560x1440\n'
            'fontScale = 0.85\n',
            'section4 = 0,0,Resolution 2560x1440\n'
            'section5 = 0,0,Resolution Original\n'
            'fontScale = 0.85\n',
        ),
        (
            'pos4 = 240,30\n\n[METAL_GRAPHICS_CLOSE]',
            'pos4 = 240,30\n'
            'pos5 = 0,60\n\n[METAL_GRAPHICS_CLOSE]',
        ),
    ),
    "RESOURCE/INI/texts/russian/common.ini": (
        (
            'string = Metal Graphics Restart Note,"Сохраните кнопкой OK после закрытия окна. Применение — после перезапуска. Разрешение — для окна; полный экран — по размеру рабочего стола."\n',
            'string = Metal Graphics Restart Note,"Сохраните кнопкой OK после закрытия окна. Применение — после перезапуска. Разрешение рендеринга; пропорции исходного размера сохраняются."\n',
        ),
        (
            'string = Resolution 2560x1440,"2560 × 1440"\n',
            'string = Resolution 2560x1440,"2560 × 1440"\n'
            'string = Resolution Original,"Исходное"\n',
        ),
    ),
}


def _build_current_files() -> tuple[FilePatch, ...]:
    current_hashes = {
        "PROGRAM/interface/option_sl.c": "64e7d86a0400294bf9fb1493e6d70dcefe90fdac5350d2765328d67640cf6967",
        "PROGRAM/interface/option_screen.c": "f6c99efaa093b7cab27357cc150d1d7757852ac5054135af895c5f35fc45664d",
        "RESOURCE/INI/interfaces/option_screen.ini": "e6b318392500ee3b99381241a98b6485162196a70da4ce5a53baba7c33c3c627",
        "RESOURCE/INI/texts/russian/common.ini": "603ff43f848709c0bc341a6242b7f991379ed97f320eccc55d7da50fa7298b83",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.original_sha256,
            current_hashes[spec.relative_path],
            spec.replacements + _CURRENT_EXTRAS.get(spec.relative_path, ()),
        )
        for spec in LEGACY_FILES
    )


CURRENT_FILES = _build_current_files()

# The shadow-quality UI/record change is a second, hash-pinned layer over the
# currently delivered four-file graphics layer.  It is deliberately kept as a
# separate transform so an already patched runtime can be upgraded without
# accepting arbitrary source edits.
_SHADOW_QUALITY_EXTRAS = {
    "PROGRAM/interface/option_sl.c": (
        (
            "\toptref.dynamic_lighting = true;\n",
            "\toptref.dynamic_lighting = true;\n"
            "\toptref.shadow_quality = 1;\n",
        ),
        (
            "\toptref.dynamic_lighting = InterfaceStates.MetalDynamicLighting;\n",
            "\toptref.dynamic_lighting = InterfaceStates.MetalDynamicLighting;\n"
            "\toptref.shadow_quality = InterfaceStates.MetalShadowQuality;\n",
        ),
        (
            "\tInterfaceStates.MetalDynamicLighting = sti(optref.dynamic_lighting);\n",
            "\tInterfaceStates.MetalDynamicLighting = sti(optref.dynamic_lighting);\n"
            "\tInterfaceStates.MetalShadowQuality = sti(optref.shadow_quality);\n"
            "\tif(sti(InterfaceStates.MetalShadowQuality)<0 || sti(InterfaceStates.MetalShadowQuality)>2) "
            "InterfaceStates.MetalShadowQuality = 1;\n",
        ),
    ),
    "PROGRAM/interface/option_screen.c": (
        (
            '\tif( sNodName == "METAL_DYNAMIC_LIGHTING_CHECKBOX" ) InterfaceStates.MetalDynamicLighting = bBtnState;\n',
            '\tif( sNodName == "METAL_DYNAMIC_LIGHTING_CHECKBOX" ) InterfaceStates.MetalDynamicLighting = bBtnState;\n'
            '\tif( sNodName == "METAL_SHADOW_QUALITY_RADIO" && bBtnState ) InterfaceStates.MetalShadowQuality = nBtnIndex - 1;\n',
        ),
        (
            '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_DYNAMIC_LIGHTING_CHECKBOX", 2, 1, '
            'sti(InterfaceStates.MetalDynamicLighting));\n',
            '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_DYNAMIC_LIGHTING_CHECKBOX", 2, 1, '
            'sti(InterfaceStates.MetalDynamicLighting));\n'
            '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_SHADOW_QUALITY_RADIO", 2, '
            'sti(InterfaceStates.MetalShadowQuality)+1, true);\n',
        ),
    ),
    "RESOURCE/INI/interfaces/option_screen.ini": (
        (
            'item = 105,CHECKBUTTON,METAL_DYNAMIC_LIGHTING_CHECKBOX\n',
            'item = 105,CHECKBUTTON,METAL_DYNAMIC_LIGHTING_CHECKBOX\n'
            'item = 105,RADIOBUTTON,METAL_SHADOW_QUALITY_RADIO\n',
        ),
        (
            'nodelist = METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_RESOLUTION_RADIO\n',
            'nodelist = METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_SHADOW_QUALITY_RADIO,METAL_RESOLUTION_RADIO\n',
        ),
        (
            '[METAL_RESOLUTION_RADIO]\n',
            '[METAL_SHADOW_QUALITY_RADIO]\n'
            'command = click\n'
            'command = upstep\n'
            'command = downstep\n'
            'command = rightstep\n'
            'command = leftstep\n'
            'position = 170,310,630,330\n'
            'alignment = left\n'
            'lineheight = 16\n'
            'rect_textoffset = 22,0,0,0\n'
            'iconsize = 16,16\n'
            'section1 = 0,0,Shadow Quality Low\n'
            'section2 = 0,0,Shadow Quality Medium\n'
            'section3 = 0,0,Shadow Quality High\n'
            'fontScale = 0.85\n'
            'bShowGlowCursor = 0\n'
            'individualpos = 1\n'
            'pos1 = 0,0\n'
            'pos2 = 160,0\n'
            'pos3 = 320,0\n'
            '\n'
            '[METAL_RESOLUTION_RADIO]\n',
        ),
    ),
    "RESOURCE/INI/texts/russian/common.ini": (
        (
            'string = Metal Dynamic Shadows,"Динамические тени"\n',
            'string = Metal Dynamic Shadows,"Динамические тени"\n'
            'string = Shadow Quality Low,"Низкое"\n'
            'string = Shadow Quality Medium,"Среднее"\n'
            'string = Shadow Quality High,"Высокое"\n'
        ),
    ),
}


def _build_shadow_quality_files() -> tuple[FilePatch, ...]:
    patched_hashes = {
        "PROGRAM/interface/option_sl.c": "686a905b2e3b605ccb5052eb6f7ce39b6feaa38580db9dd35363040dad99e9dc",
        "PROGRAM/interface/option_screen.c": "c9622f5f3cc663a1c2232c0c4f52283a0300e8d02655c12fc1213a568224fa27",
        "RESOURCE/INI/interfaces/option_screen.ini": "9c0b685e285a481dcd3b62bfabfa709fa0c4ba3972c4b58e8eb8f92ed1b23688",
        "RESOURCE/INI/texts/russian/common.ini": "bb3c65769c717cc9d87d41b9cbe33e6fca86a85fd4d87d58a57d0ed9810bb55d",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.patched_sha256,
            patched_hashes[spec.relative_path],
            _SHADOW_QUALITY_EXTRAS[spec.relative_path],
        )
        for spec in CURRENT_FILES
    )


SHADOW_QUALITY_FILES = _build_shadow_quality_files()

# The desktop/native mode is a third, hash-pinned layer over the delivered
# graphics UI.  The script record keeps the current/custom dimensions; the
# engine bridge resolves the active Metal drawable only after SDL creates the
# window, where Retina backing pixels are available.
_DISPLAY_MODE_EXTRAS = {
    "PROGRAM/interface/option_sl.c": (
        (
            'if(sti(InterfaceStates.MetalResolution)<0 || sti(InterfaceStates.MetalResolution)>4) '
            'InterfaceStates.MetalResolution = 2;\n',
            'if(sti(InterfaceStates.MetalResolution)<0 || sti(InterfaceStates.MetalResolution)>5) '
            'InterfaceStates.MetalResolution = 2;\n',
        ),
    ),
    "PROGRAM/interface/option_screen.c": (
        (
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 5, XI_ConvertString("Resolution Original") + ": " + nBaseWidth + " x " + nBaseHeight);\n',
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 5, XI_ConvertString("Resolution Current") + ": " + nBaseWidth + " x " + nBaseHeight);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 6, XI_ConvertString("Resolution Desktop"));\n',
        ),
    ),
    "RESOURCE/INI/interfaces/option_screen.ini": (
        (
            'section5 = 0,0,Resolution Original\n'
            'fontScale = 0.85\n',
            'section5 = 0,0,Resolution Current\n'
            'section6 = 0,0,Resolution Desktop\n'
            'fontScale = 0.85\n',
        ),
        (
            'pos5 = 0,60\n\n[METAL_GRAPHICS_CLOSE]',
            'pos5 = 0,60\n'
            'pos6 = 240,60\n\n[METAL_GRAPHICS_CLOSE]',
        ),
    ),
    "RESOURCE/INI/texts/russian/common.ini": (
        (
            'string = Resolution Original,"Исходное"\n',
            'string = Resolution Current,"Текущее"\n'
            'string = Resolution Desktop,"Рабочий стол"\n',
        ),
        (
            'string = Metal Graphics Restart Note,"Сохраните кнопкой OK после закрытия окна. Применение — после перезапуска. Разрешение рендеринга; пропорции исходного размера сохраняются."\n',
            'string = Metal Graphics Restart Note,"Сохраните кнопкой OK после закрытия окна. Применение — после перезапуска. Текущее/своё разрешение сохраняется; Рабочий стол использует разрешение экрана в полноэкранном режиме."\n',
        ),
    ),
}


def _build_display_mode_files() -> tuple[FilePatch, ...]:
    # Filled after the first transform; these hashes are generated below from
    # the checked-in source layer and pin the bridge-facing UI revision.
    patched_hashes = {
        "PROGRAM/interface/option_sl.c": "bf564e35c6f035e6229e4ec4782f37dc588422ebd1836a8087d1edf8cd04f489",
        "PROGRAM/interface/option_screen.c": "eeba02ccd95ae24f6a6c5889edea711c5d3c79ae6776d091f5f663721491ae6d",
        "RESOURCE/INI/interfaces/option_screen.ini": "281ae40058f435456bf6c3291a2a8fc29e27764c0aa7dc4dd0a7ce2c49054f06",
        "RESOURCE/INI/texts/russian/common.ini": "1571e464c361e937376866c1692f58c3c7b4742644128ce62a15325b22250be1",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.patched_sha256,
            patched_hashes[spec.relative_path],
            _DISPLAY_MODE_EXTRAS[spec.relative_path],
        )
        for spec in SHADOW_QUALITY_FILES
    )


DISPLAY_MODE_FILES = _build_display_mode_files()

# Live apply is a fourth, hash-pinned layer.  The UI keeps its draft entirely
# in InterfaceStates until the native renderer has accepted the complete
# transaction; only then does it serialize the Metal record.
_LIVE_APPLY_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        (
            "int \tnewBase \t= screenscaling;\n",
            "int \tnewBase \t= screenscaling;\n"
            "\n"
            "int metalGraphicsSnapshotDynamicLighting;\n"
            "int metalGraphicsSnapshotShadowQuality;\n"
            "int metalGraphicsSnapshotModernWater;\n"
            "int metalGraphicsSnapshotModernLighting;\n"
            "int metalGraphicsSnapshotCinematic;\n"
            "int metalGraphicsSnapshotLightShafts;\n"
            "int metalGraphicsSnapshotDynamicSky;\n"
            "int metalGraphicsSnapshotAntialiasing;\n"
            "int metalGraphicsSnapshotFullScreen;\n"
            "int metalGraphicsSnapshotResolution;\n"
            "int metalGraphicsSnapshotResolutionWidth;\n"
            "int metalGraphicsSnapshotResolutionHeight;\n"
            "bool metalGraphicsEditing = false;\n",
        ),
        (
            '\tif( sNodName == "METAL_GRAPHICS_BTN" ) {\n'
            '\t\tXI_WindowDisable("MAIN_WINDOW", true);\n'
            '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", true);\n'
            '\t\tXI_WindowDisable("METAL_GRAPHICS_WINDOW", false);\n'
            '\t\tSetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif( sNodName == "METAL_GRAPHICS_CLOSE" ) {\n'
            '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", false);\n'
            '\t\tXI_WindowDisable("MAIN_WINDOW", false);\n'
            '\t\tSetCurrentNode("METAL_GRAPHICS_BTN");\n'
            '\t\treturn;\n'
            '\t}\n',
            '\tif( sNodName == "METAL_GRAPHICS_BTN" ) {\n'
            '\t\tSnapshotMetalGraphicsOptions();\n'
            '\t\tmetalGraphicsEditing = true;\n'
            '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", "");\n'
            '\t\tXI_WindowDisable("MAIN_WINDOW", true);\n'
            '\t\tXI_WindowShow("METAL_GRAPHICS_WINDOW", true);\n'
            '\t\tXI_WindowDisable("METAL_GRAPHICS_WINDOW", false);\n'
            '\t\tSetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif( sNodName == "METAL_GRAPHICS_APPLY" ) {\n'
            '\t\tApplyMetalGraphicsOptions();\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif( sNodName == "METAL_GRAPHICS_CANCEL" ) {\n'
            '\t\tRestoreMetalGraphicsOptions();\n'
            '\t\tCloseMetalGraphicsWindow();\n'
            '\t\treturn;\n'
            '\t}\n',
        ),
        (
            'void ProcessCancelExit()\n'
            '{\n'
            '\tLoadGameOptions();\n'
            '\tProcessExit();\n'
            '}\n',
            'void ProcessCancelExit()\n'
            '{\n'
            '\tif(metalGraphicsEditing) {\n'
            '\t\tRestoreMetalGraphicsOptions();\n'
            '\t\tCloseMetalGraphicsWindow();\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tLoadGameOptions();\n'
            '\tProcessExit();\n'
            '}\n',
        ),
        (
            'void GetMetalGraphicsData()\n',
            'void SnapshotMetalGraphicsOptions()\n'
            '{\n'
            '\tmetalGraphicsSnapshotDynamicLighting = sti(InterfaceStates.MetalDynamicLighting);\n'
            '\tmetalGraphicsSnapshotShadowQuality = sti(InterfaceStates.MetalShadowQuality);\n'
            '\tmetalGraphicsSnapshotModernWater = sti(InterfaceStates.MetalModernWater);\n'
            '\tmetalGraphicsSnapshotModernLighting = sti(InterfaceStates.MetalModernLighting);\n'
            '\tmetalGraphicsSnapshotCinematic = sti(InterfaceStates.MetalCinematic);\n'
            '\tmetalGraphicsSnapshotLightShafts = sti(InterfaceStates.MetalLightShafts);\n'
            '\tmetalGraphicsSnapshotDynamicSky = sti(InterfaceStates.MetalDynamicSky);\n'
            '\tmetalGraphicsSnapshotAntialiasing = sti(InterfaceStates.MetalAntialiasing);\n'
            '\tmetalGraphicsSnapshotFullScreen = sti(InterfaceStates.MetalFullScreen);\n'
            '\tmetalGraphicsSnapshotResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tmetalGraphicsSnapshotResolutionWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
            '\tmetalGraphicsSnapshotResolutionHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
            '}\n'
            '\n'
            'void RestoreMetalGraphicsOptions()\n'
            '{\n'
            '\tInterfaceStates.MetalDynamicLighting = metalGraphicsSnapshotDynamicLighting;\n'
            '\tInterfaceStates.MetalShadowQuality = metalGraphicsSnapshotShadowQuality;\n'
            '\tInterfaceStates.MetalModernWater = metalGraphicsSnapshotModernWater;\n'
            '\tInterfaceStates.MetalModernLighting = metalGraphicsSnapshotModernLighting;\n'
            '\tInterfaceStates.MetalCinematic = metalGraphicsSnapshotCinematic;\n'
            '\tInterfaceStates.MetalLightShafts = metalGraphicsSnapshotLightShafts;\n'
            '\tInterfaceStates.MetalDynamicSky = metalGraphicsSnapshotDynamicSky;\n'
            '\tInterfaceStates.MetalAntialiasing = metalGraphicsSnapshotAntialiasing;\n'
            '\tInterfaceStates.MetalFullScreen = metalGraphicsSnapshotFullScreen;\n'
            '\tInterfaceStates.MetalResolution = metalGraphicsSnapshotResolution;\n'
            '\tInterfaceStates.MetalResolutionWidth = metalGraphicsSnapshotResolutionWidth;\n'
            '\tInterfaceStates.MetalResolutionHeight = metalGraphicsSnapshotResolutionHeight;\n'
            '\tGetMetalGraphicsData();\n'
            '}\n'
            '\n'
            'void CloseMetalGraphicsWindow()\n'
            '{\n'
            '\tmetalGraphicsEditing = false;\n'
            '\tXI_WindowShow("METAL_GRAPHICS_WINDOW", false);\n'
            '\tXI_WindowDisable("MAIN_WINDOW", false);\n'
            '\tSetCurrentNode("METAL_GRAPHICS_BTN");\n'
            '}\n'
            '\n'
            'void ApplyMetalGraphicsOptions()\n'
            '{\n'
            '\tint flagsMask = 0;\n'
            '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
            '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
            '\tint nWidth;\n'
            '\tint nHeight;\n'
            '\tint desktopMode = 0;\n'
            '\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }\n'
            '\tif(sti(InterfaceStates.MetalDynamicLighting)) flagsMask = flagsMask + 1;\n'
            '\tif(sti(InterfaceStates.MetalModernWater)) flagsMask = flagsMask + 2;\n'
            '\tif(sti(InterfaceStates.MetalModernLighting)) flagsMask = flagsMask + 4;\n'
            '\tif(sti(InterfaceStates.MetalCinematic)) flagsMask = flagsMask + 8;\n'
            '\tif(sti(InterfaceStates.MetalLightShafts)) flagsMask = flagsMask + 16;\n'
            '\tif(sti(InterfaceStates.MetalDynamicSky)) flagsMask = flagsMask + 32;\n'
            '\tif(sti(InterfaceStates.MetalAntialiasing)) flagsMask = flagsMask + 64;\n'
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(sti(InterfaceStates.MetalResolution)==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;\n'
            '\tif(SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),nWidth,nHeight,desktopMode)) {\n'
            '\t\tSaveMetalGraphicsOptions();\n'
            '\t\tCloseMetalGraphicsWindow();\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));\n'
            '}\n'
            '\n'
            'void GetMetalGraphicsData()\n',
        ),
    ),
    "RESOURCE/INI/interfaces/option_screen.ini": (
        (
            'item = 105,FORMATEDTEXT,METAL_GRAPHICS_NOTE\n'
            'item = 105,TEXTBUTTON2,METAL_GRAPHICS_CLOSE\n',
            'item = 105,FORMATEDTEXT,METAL_GRAPHICS_ERROR\n'
            'item = 105,TEXTBUTTON2,METAL_GRAPHICS_APPLY\n'
            'item = 105,TEXTBUTTON2,METAL_GRAPHICS_CANCEL\n',
        ),
        (
            'nodelist = METAL_GRAPHICS_FRAME,METAL_GRAPHICS_TITLE,METAL_GRAPHICS_NOTE,METAL_GRAPHICS_CLOSE\n',
            'nodelist = METAL_GRAPHICS_FRAME,METAL_GRAPHICS_TITLE,METAL_GRAPHICS_ERROR,METAL_GRAPHICS_APPLY,METAL_GRAPHICS_CANCEL\n',
        ),
        (
            'position = 120,90,680,500\n',
            'position = 120,90,680,480\n',
        ),
        (
            '[METAL_GRAPHICS_NOTE]\n'
            'position = 170,140,630,185\n'
            'fontscale = 0.8\n'
            'lineSpace = 14\n'
            'valignment = 1\n'
            'string = Metal Graphics Restart Note\n',
            '[METAL_GRAPHICS_ERROR]\n'
            'position = 170,140,630,160\n'
            'fontscale = 0.8\n'
            'lineSpace = 14\n'
            'valignment = 1\n'
            'Color = 255, 220, 100, 100\n',
        ),
        (
            'position = 170,190,390,210\n',
            'position = 170,170,390,190\n',
        ),
        (
            'position = 410,190,630,210\n',
            'position = 410,170,630,190\n',
        ),
        (
            'position = 170,220,390,240\n',
            'position = 170,200,390,220\n',
        ),
        (
            'position = 410,220,630,240\n',
            'position = 410,200,630,220\n',
        ),
        (
            'position = 170,250,390,270\n',
            'position = 170,230,390,250\n',
        ),
        (
            'position = 410,250,630,270\n',
            'position = 410,230,630,250\n',
        ),
        (
            'position = 170,280,390,300\n',
            'position = 170,260,390,280\n',
        ),
        (
            'position = 410,280,630,300\n',
            'position = 410,260,630,280\n',
        ),
        (
            'position = 170,310,630,330\n',
            'position = 170,290,630,310\n',
        ),
        (
            'position = 170,340,630,430\n',
            'position = 170,320,630,410\n',
        ),
        (
            '[METAL_GRAPHICS_CLOSE]\n'
            'command = click,event:eventBtnAction\n'
            'command = activate,event:eventBtnAction\n'
            'position = 510,450,630,480\n'
            'string = Close\n'
            'glowoffset = 0,0\n'
            'pressPictureOffset = 2,2\n',
            '[METAL_GRAPHICS_CANCEL]\n'
            'command = click,event:eventBtnAction\n'
            'command = activate,event:eventBtnAction\n'
            'position = 410,430,500,460\n'
            'string = Cancel\n'
            'glowoffset = 0,0\n'
            'pressPictureOffset = 2,2\n'
            '\n'
            '[METAL_GRAPHICS_APPLY]\n'
            'command = click,event:eventBtnAction\n'
            'command = activate,event:eventBtnAction\n'
            'position = 510,430,630,460\n'
            'string = Apply\n'
            'glowoffset = 0,0\n'
            'pressPictureOffset = 2,2\n',
        ),
    ),
    "RESOURCE/INI/texts/russian/common.ini": (
        (
            'string = Metal Graphics Restart Note,"Сохраните кнопкой OK после закрытия окна. Применение — после перезапуска. Текущее/своё разрешение сохраняется; Рабочий стол использует разрешение экрана в полноэкранном режиме."\n',
            'string = Metal Graphics Apply Error,"Не удалось применить"\n',
        ),
        (
            'string = Close,"Закрыть"\n',
            'string = Close,"Закрыть"\n'
            'string = Apply,"Применить"\n',
        ),
    ),
}


def _build_live_apply_files() -> tuple[FilePatch, ...]:
    patched_hashes = {
        "PROGRAM/interface/option_sl.c": "bf564e35c6f035e6229e4ec4782f37dc588422ebd1836a8087d1edf8cd04f489",
        "PROGRAM/interface/option_screen.c": "5fb2508b48e1b723efc41e1bb980727f2e88d5c39786e1d0c2244403425adc03",
        "RESOURCE/INI/interfaces/option_screen.ini": "dc300efe6672753a84cb79c53bcf47bca5fd159271a12d6ae34b9ac4fc94b5df",
        "RESOURCE/INI/texts/russian/common.ini": "3f3069bed1b5c5f10347171a731ebcfaaca0f3c6cb13b75530725599816fcf9a",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.patched_sha256,
            patched_hashes[spec.relative_path],
            _LIVE_APPLY_EXTRAS.get(spec.relative_path, ()),
        )
        for spec in DISPLAY_MODE_FILES
    )


LIVE_APPLY_FILES = _build_live_apply_files()

# FILES is the final delivered layer.  Its original hash remains the archived
# baseline so the normal sync path can prepare a clean source tree in one pass;
# predecessor layers separately recognize and upgrade each reviewed revision.
FILES = tuple(
    FilePatch(
        current.relative_path,
        legacy.original_sha256,
        next(item for item in LIVE_APPLY_FILES if item.relative_path == current.relative_path).patched_sha256,
        legacy.replacements + _CURRENT_EXTRAS.get(current.relative_path, ()) + _SHADOW_QUALITY_EXTRAS[current.relative_path] + _DISPLAY_MODE_EXTRAS[current.relative_path] + _LIVE_APPLY_EXTRAS.get(current.relative_path, ()),
    )
    for current, legacy in zip(CURRENT_FILES, LEGACY_FILES, strict=True)
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


def integer_value(values: dict[str, str], key: str, minimum: int, maximum: int) -> int:
    raw = values.get(key)
    if raw is None:
        return int(DEFAULTS[key])
    if not re.fullmatch(r"-?[0-9]+", raw):
        raise ValueError(f"{key}: expected an integer")
    value = int(raw)
    if value < minimum or value > maximum:
        raise ValueError(f"{key}: unsupported value")
    return value


def _dimension_value(values: dict[str, str], key: str) -> int:
    try:
        value = int(values[key])
    except (KeyError, ValueError) as error:
        raise ValueError(f"{key}: expected an integer") from error
    limit = (
        MIN_RESOLUTION_WIDTH if key == "resolution_width" else MIN_RESOLUTION_HEIGHT,
        MAX_RESOLUTION_WIDTH if key == "resolution_width" else MAX_RESOLUTION_HEIGHT,
    )
    if not limit[0] <= value <= limit[1]:
        raise ValueError(f"{key}: unsupported custom dimension")
    return value


def _stored_base_dimensions(values: dict[str, str]) -> tuple[int, int]:
    present = [key in values for key in ("resolution_width", "resolution_height")]
    if any(present) and not all(present):
        raise ValueError("custom resolution requires both resolution_width and resolution_height")
    if not any(present):
        return DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT
    return (
        _dimension_value(values, "resolution_width"),
        _dimension_value(values, "resolution_height"),
    )


def _render_dimensions(resolution: int, base_width: int, base_height: int) -> tuple[int, int]:
    if resolution in (CURRENT_RESOLUTION, DESKTOP_RESOLUTION):
        return base_width, base_height
    width = RESOLUTIONS[resolution][0]
    height = (width * base_height + base_width // 2) // base_width
    if not MIN_RESOLUTION_HEIGHT <= height <= MAX_RESOLUTION_HEIGHT:
        raise ValueError("resolution: derived preset height is unsupported")
    return width, height


def graphics_values(runtime: Path) -> dict[str, int | bool]:
    values = parse_record(runtime / METAL_OPTIONS)
    result: dict[str, int | bool] = {
        key: bool_value(values, key) for key in ENV_KEYS
    }
    result["shadow_quality"] = integer_value(
        values, "shadow_quality", SHADOW_QUALITY_MIN, SHADOW_QUALITY_MAX
    )
    result["full_screen"] = bool_value(values, "full_screen")
    resolution = int(values.get("resolution", DEFAULTS["resolution"]))
    if resolution < 0 or resolution > DESKTOP_RESOLUTION:
        raise ValueError("resolution: unsupported preset or custom choice")
    if resolution in (CURRENT_RESOLUTION, DESKTOP_RESOLUTION) and not {
        "resolution_width",
        "resolution_height",
    }.issubset(values):
        raise ValueError("current/desktop resolution requires both resolution_width and resolution_height")
    base_width, base_height = _stored_base_dimensions(values)
    result["resolution"] = resolution
    result["resolution_width"] = base_width
    result["resolution_height"] = base_height
    result["render_width"], result["render_height"] = _render_dimensions(
        resolution, base_width, base_height
    )
    return result


def current_engine(engine_ini: Path) -> tuple[bool, int, int, int]:
    values: dict[str, str] = {}
    if engine_ini.is_file():
        for line in engine_ini.read_text(encoding="utf-8-sig").splitlines():
            if "=" in line and not line.lstrip().startswith(("#", ";")):
                key, value = (part.strip() for part in line.split("=", 1))
                values[key] = value
    fullscreen = values.get("full_screen", "1") in {"1", "true", "True"}
    try:
        display_mode = int(values.get("display_mode", str(DISPLAY_MODE_CUSTOM)))
    except ValueError:
        display_mode = DISPLAY_MODE_CUSTOM
    try:
        width = int(values.get("screen_x", str(DEFAULT_RESOLUTION_WIDTH)))
        height = int(values.get("screen_y", str(DEFAULT_RESOLUTION_HEIGHT)))
        _dimension_value({"resolution_width": str(width)}, "resolution_width")
        _dimension_value({"resolution_height": str(height)}, "resolution_height")
    except ValueError:
        width, height = DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT
    if display_mode == DISPLAY_MODE_DESKTOP and fullscreen:
        resolution = DESKTOP_RESOLUTION
    else:
        resolution = next(
            (index for index, dimensions in enumerate(RESOLUTIONS) if dimensions == (width, height)),
            CURRENT_RESOLUTION,
        )
    return fullscreen, resolution, width, height


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
    # Renderer GetInt(nullptr, ...) reads the unnamed INI section. Appending a
    # missing key after [controls] would silently hide Desktop mode from it.
    section = re.search(r"(?m)^\s*\[", source)
    global_end = section.start() if section else len(source)
    global_values, sections = source[:global_end], source[global_end:]
    fullscreen = "1" if values["full_screen"] else "0"
    width = int(values["render_width"])
    height = int(values["render_height"])
    display_mode = DISPLAY_MODE_DESKTOP if int(values["resolution"]) == DESKTOP_RESOLUTION else DISPLAY_MODE_CUSTOM
    replacements = {
        "full_screen": fullscreen,
        "screen_x": str(width),
        "screen_y": str(height),
        "display_mode": str(display_mode),
    }
    for key, value in replacements.items():
        global_values, count = re.subn(rf"(?m)^{re.escape(key)}\s*=.*$", f"{key} = {value}", global_values)
        if count == 0 and key == "display_mode":
            suffix = "" if global_values.endswith("\n") else "\n"
            global_values += f"{suffix}{key} = {value}\n"
            count = 1
        if count != 1:
            raise ValueError(f"{engine}: expected one {key} entry")
    atomic_write(engine, (global_values + sections).encode("utf-8"))


def initialize_record(runtime: Path) -> None:
    record = runtime / METAL_OPTIONS
    if record.exists():
        graphics_values(runtime)  # Reject malformed persisted input before launch.
        return
    values = dict(DEFAULTS)
    (
        values["full_screen"],
        values["resolution"],
        values["resolution_width"],
        values["resolution_height"],
    ) = current_engine(runtime / "engine.ini")
    atomic_write(record, "".join(f"{key}={int(value)}\n" for key, value in values.items()).encode())


def launch_environment(runtime: Path, inherited: dict[str, str]) -> dict[str, str]:
    result = dict(inherited)
    values = graphics_values(runtime)
    for key, env_name in ENV_KEYS.items():
        # An explicit diagnostic override still wins; ordinary launches use UI.
        result.setdefault(env_name, str(int(values[key])))
    for key, env_name in INTEGER_ENV_KEYS.items():
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
    live_apply = next(item for item in LIVE_APPLY_FILES if item.relative_path == relative_path)
    if current == live_apply.original_sha256:
        return transform(source, live_apply), "live-apply-upgrade"
    display = next(item for item in DISPLAY_MODE_FILES if item.relative_path == relative_path)
    if current == display.original_sha256:
        return transform(source, display), "display-mode-upgrade"
    shadow = next(item for item in SHADOW_QUALITY_FILES if item.relative_path == relative_path)
    if current == shadow.original_sha256:
        return transform(source, shadow), "current-upgrade"
    legacy = next(item for item in LEGACY_FILES if item.relative_path == relative_path)
    if current == legacy.patched_sha256:
        canonical, _ = strip(relative_path, source)
        return transform(canonical, spec), "legacy-upgrade"
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
    live_apply = next(item for item in LIVE_APPLY_FILES if item.relative_path == relative_path)
    if current == live_apply.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(live_apply.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: live apply layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != live_apply.original_sha256:
            raise ValueError(f"{relative_path}: stripped live apply layer is not canonical")
        canonical, _ = strip(relative_path, result)
        return canonical, "live-apply-patched"
    shadow_layer = next(item for item in SHADOW_QUALITY_FILES if item.relative_path == relative_path)
    if current == shadow_layer.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(shadow_layer.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: shadow quality layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != shadow_layer.original_sha256:
            raise ValueError(f"{relative_path}: stripped shadow quality layer is not canonical")
        canonical, _ = strip(relative_path, result)
        return canonical, "shadow-quality-patched"
    display_layer = next(item for item in DISPLAY_MODE_FILES if item.relative_path == relative_path)
    if current == display_layer.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(display_layer.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: display mode layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != display_layer.original_sha256:
            raise ValueError(f"{relative_path}: stripped display mode layer is not canonical")
        canonical, _ = strip(relative_path, result)
        return canonical, "display-mode-patched"
    current_layer = next(item for item in CURRENT_FILES if item.relative_path == relative_path)
    if current == current_layer.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(current_layer.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: current layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != current_layer.original_sha256:
            raise ValueError(f"{relative_path}: stripped current layer is not canonical")
        return result, "current-patched"
    legacy = next(item for item in LEGACY_FILES if item.relative_path == relative_path)
    if current == legacy.patched_sha256:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(legacy.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: legacy layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != legacy.original_sha256:
            raise ValueError(f"{relative_path}: stripped legacy hash is not canonical")
        return result, "legacy-patched"
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
