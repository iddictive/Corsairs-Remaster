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
    "display_width": DEFAULT_RESOLUTION_WIDTH,
    "display_height": DEFAULT_RESOLUTION_HEIGHT,
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

# The compact picker is a fifth hash-pinned layer. Resolution is one bounded
# draft value with explicit previous/next controls; the renderer never sees a
# radio-node event index or an aspect-derived pseudo-preset.
_RESOLUTION_PICKER_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        (
            '\tif( sNodName == "METAL_GRAPHICS_CANCEL" ) {\n'
            '\t\tRestoreMetalGraphicsOptions();\n'
            '\t\tCloseMetalGraphicsWindow();\n'
            '\t\treturn;\n'
            '\t}\n',
            '\tif( sNodName == "METAL_GRAPHICS_CANCEL" ) {\n'
            '\t\tRestoreMetalGraphicsOptions();\n'
            '\t\tCloseMetalGraphicsWindow();\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif( sNodName == "METAL_RESOLUTION_PREV" ) {\n'
            '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) - 1;\n'
            '\t\tif(sti(InterfaceStates.MetalResolution) < 0) InterfaceStates.MetalResolution = 5;\n'
            '\t\tUpdateMetalResolutionLabel();\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif( sNodName == "METAL_RESOLUTION_NEXT" ) {\n'
            '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) + 1;\n'
            '\t\tif(sti(InterfaceStates.MetalResolution) > 5) InterfaceStates.MetalResolution = 0;\n'
            '\t\tUpdateMetalResolutionLabel();\n'
            '\t\treturn;\n'
            '\t}\n',
        ),
        (
            '\tif( sNodName == "METAL_RESOLUTION_RADIO" && bBtnState ) {\n'
            '\t\tInterfaceStates.MetalResolution = nBtnIndex - 1;\n'
            '\t\tUpdateMetalResolutionLabels();\n'
            '\t}\n',
            '\t// Resolution is owned by the bounded previous/next picker.\n',
        ),
        (
            '\tint nWidth;\n'
            '\tint nHeight;\n'
            '\tint desktopMode = 0;\n'
            '\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }\n',
            '\tint nWidth;\n'
            '\tint nHeight;\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tint desktopMode = 0;\n'
            '\tif(nBaseWidth < 320 || nBaseHeight < 200 || nBaseWidth > 16384 || nBaseHeight > 16384) {\n'
            '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif(nResolution < 0 || nResolution > 5) {\n'
            '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));\n'
            '\t\treturn;\n'
            '\t}\n',
        ),
        (
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(sti(InterfaceStates.MetalResolution)==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(sti(InterfaceStates.MetalResolution)==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;\n',
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;\n',
        ),
        (
            '\tSendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 2, sti(InterfaceStates.MetalResolution)+1, true);\n'
            '\tUpdateMetalResolutionLabels();\n'
            '}\n'
            '\n'
            'void UpdateMetalResolutionLabels()\n',
            '\tUpdateMetalResolutionLabel();\n'
            '}\n'
            '\n'
            'void UpdateMetalResolutionLabels()\n',
        ),
        (
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
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 5, XI_ConvertString("Resolution Current") + ": " + nBaseWidth + " x " + nBaseHeight);\n'
            '\tSendMessage(&GameInterface,"lslls",MSG_INTERFACE_MSG_TO_NODE,"METAL_RESOLUTION_RADIO", 1, 6, XI_ConvertString("Resolution Desktop"));\n'
            '}\n',
            'void UpdateMetalResolutionLabel()\n'
            '{\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
            '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
            '\tint nWidth;\n'
            '\tint nHeight;\n'
            '\tstring sResolution;\n'
            '\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; InterfaceStates.MetalResolution = 2; }\n'
            '\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }\n'
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution>=0 && nResolution<=3) sResolution = "" + nWidth + " x " + nHeight;\n'
            '\tif(nResolution==4) sResolution = XI_ConvertString("Resolution Current") + ": " + sti(InterfaceStates.MetalResolutionWidth) + " x " + sti(InterfaceStates.MetalResolutionHeight);\n'
            '\tif(nResolution==5) sResolution = XI_ConvertString("Resolution Desktop");\n'
            '\tSetFormatedText("METAL_RESOLUTION_VALUE", sResolution);\n'
            '}\n',
        ),
    ),
    "RESOURCE/INI/interfaces/option_screen.ini": (
        (
            'item = 105,RADIOBUTTON,METAL_RESOLUTION_RADIO\n',
            'item = 105,BUTTON,METAL_RESOLUTION_PREV\n'
            'item = 105,FORMATEDTEXT,METAL_RESOLUTION_VALUE\n'
            'item = 105,BUTTON,METAL_RESOLUTION_NEXT\n',
        ),
        (
            'nodelist = METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_SHADOW_QUALITY_RADIO,METAL_RESOLUTION_RADIO\n',
            'nodelist = METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_SHADOW_QUALITY_RADIO\n'
            'nodelist = METAL_RESOLUTION_PREV,METAL_RESOLUTION_VALUE,METAL_RESOLUTION_NEXT\n',
        ),
        (
            '[METAL_RESOLUTION_RADIO]\n'
            'command = click\n'
            'command = upstep\n'
            'command = downstep\n'
            'command = rightstep\n'
            'command = leftstep\n'
            'position = 170,320,630,410\n'
            'alignment = left\n'
            'lineheight = 16\n'
            'rect_textoffset = 22,0,0,0\n'
            'iconsize = 16,16\n'
            'section1 = 0,0,Resolution 1280x720\n'
            'section2 = 0,0,Resolution 1600x900\n'
            'section3 = 0,0,Resolution 1920x1080\n'
            'section4 = 0,0,Resolution 2560x1440\n'
            'section5 = 0,0,Resolution Current\n'
            'section6 = 0,0,Resolution Desktop\n'
            'fontScale = 0.85\n'
            'bShowGlowCursor = 0\n'
            'individualpos = 1\n'
            'pos1 = 0,0\n'
            'pos2 = 240,0\n'
            'pos3 = 0,30\n'
            'pos4 = 240,30\n'
            'pos5 = 0,60\n'
            'pos6 = 240,60\n',
            '[METAL_RESOLUTION_PREV]\n'
            'command = click,event:eventBtnAction\n'
            'command = activate,event:eventBtnAction\n'
            'command = rightstep,select:METAL_RESOLUTION_NEXT\n'
            'position = 180,330,220,370\n'
            'group = ARROWS\n'
            'picture = left\n'
            'shadowOffset = 3,2\n'
            '\n'
            '[METAL_RESOLUTION_VALUE]\n'
            'position = 230,330,570,370\n'
            'fontscale = 0.9\n'
            'lineSpace = 16\n'
            'alignment = center\n'
            'valignment = 1\n'
            '\n'
            '[METAL_RESOLUTION_NEXT]\n'
            'command = click,event:eventBtnAction\n'
            'command = activate,event:eventBtnAction\n'
            'command = leftstep,select:METAL_RESOLUTION_PREV\n'
            'position = 580,330,620,370\n'
            'group = ARROWS\n'
            'picture = right\n'
            'shadowOffset = 3,2\n',
        ),
    ),
}


def _build_resolution_picker_files() -> tuple[FilePatch, ...]:
    patched_hashes = {
        "PROGRAM/interface/option_sl.c": "bf564e35c6f035e6229e4ec4782f37dc588422ebd1836a8087d1edf8cd04f489",
        "PROGRAM/interface/option_screen.c": "809d8c152687b83b3fb869afb93c9b5e943cc86c99163b3dd06f50befeeadf44",
        "RESOURCE/INI/interfaces/option_screen.ini": "69496485a05a6731bfcccd61806cc330b3677acfa7fccb5b4b75bbff3364aeb7",
        "RESOURCE/INI/texts/russian/common.ini": "3f3069bed1b5c5f10347171a731ebcfaaca0f3c6cb13b75530725599816fcf9a",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.patched_sha256,
            patched_hashes[spec.relative_path],
            _RESOLUTION_PICKER_EXTRAS.get(spec.relative_path, ()),
        )
        for spec in LIVE_APPLY_FILES
    )


RESOLUTION_PICKER_FILES = _build_resolution_picker_files()

# The sixth layer makes Desktop an actual display query, shows dimensions and
# aspect for every choice, and gives the compact picker a stable labeled row.
_DESKTOP_PICKER_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        (
            "bool metalGraphicsEditing = false;\n",
            "bool metalGraphicsEditing = false;\n"
            "int metalGraphicsDesktopWidth = 1920;\n"
            "int metalGraphicsDesktopHeight = 1080;\n",
        ),
        (
            "void GetMetalGraphicsData()\n"
            "{\n",
            "void RefreshMetalDesktopSize()\n"
            "{\n"
            "\tint fallbackWidth = sti(InterfaceStates.MetalResolutionWidth);\n"
            "\tint fallbackHeight = sti(InterfaceStates.MetalResolutionHeight);\n"
            "\tif(fallbackWidth < 320 || fallbackHeight < 200) { fallbackWidth = 1920; fallbackHeight = 1080; }\n"
            "\tmetalGraphicsDesktopWidth = fallbackWidth;\n"
            "\tmetalGraphicsDesktopHeight = fallbackHeight;\n"
            "\tint displayWidth = SendMessage(&GameInterface,\"l\",45063);\n"
            "\tint displayHeight = SendMessage(&GameInterface,\"l\",45064);\n"
            "\tif(displayWidth >= 320 && displayHeight >= 200) {\n"
            "\t\tmetalGraphicsDesktopWidth = displayWidth;\n"
            "\t\tmetalGraphicsDesktopHeight = displayHeight;\n"
            "\t}\n"
            "}\n"
            "\n"
            "void GetMetalGraphicsData()\n"
            "{\n"
            "\tRefreshMetalDesktopSize();\n",
        ),
        (
            "\tif(nResolution==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;\n",
            "\tif(nResolution==5 && sti(InterfaceStates.MetalFullScreen)) {\n"
            "\t\tnWidth = metalGraphicsDesktopWidth;\n"
            "\t\tnHeight = metalGraphicsDesktopHeight;\n"
            "\t\tdesktopMode = 1;\n"
            "\t}\n",
        ),
        (
            "void UpdateMetalResolutionLabel()\n"
            "{\n"
            "\tint nResolution = sti(InterfaceStates.MetalResolution);\n"
            "\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n"
            "\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n"
            "\tint nWidth;\n"
            "\tint nHeight;\n"
            "\tstring sResolution;\n"
            "\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; InterfaceStates.MetalResolution = 2; }\n"
            "\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }\n"
            "\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n"
            "\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution>=0 && nResolution<=3) sResolution = \"\" + nWidth + \" x \" + nHeight;\n"
            "\tif(nResolution==4) sResolution = XI_ConvertString(\"Resolution Current\") + \": \" + sti(InterfaceStates.MetalResolutionWidth) + \" x \" + sti(InterfaceStates.MetalResolutionHeight);\n"
            "\tif(nResolution==5) sResolution = XI_ConvertString(\"Resolution Desktop\");\n"
            "\tSetFormatedText(\"METAL_RESOLUTION_VALUE\", sResolution);\n"
            "}\n",
            "string FormatMetalAspect(int nWidth, int nHeight)\n"
            "{\n"
            "\tint nAspect = makeint((nWidth * 100 + nHeight / 2) / nHeight);\n"
            "\tint nWhole = makeint(nAspect / 100);\n"
            "\tint nFraction = nAspect % 100;\n"
            "\tstring sAspect = \"\" + nWhole + \".\";\n"
            "\tif(nFraction < 10) sAspect = sAspect + \"0\";\n"
            "\treturn sAspect + nFraction + \":1\";\n"
            "}\n"
            "\n"
            "void UpdateMetalResolutionLabel()\n"
            "{\n"
            "\tint nResolution = sti(InterfaceStates.MetalResolution);\n"
            "\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n"
            "\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n"
            "\tint nWidth;\n"
            "\tint nHeight;\n"
            "\tstring sPrefix = \"\";\n"
            "\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; InterfaceStates.MetalResolution = 2; }\n"
            "\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }\n"
            "\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n"
            "\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n"
            "\tif(nResolution==4) sPrefix = XI_ConvertString(\"Resolution Current\") + \": \";\n"
            "\tif(nResolution==5) { sPrefix = XI_ConvertString(\"Resolution Desktop\") + \": \"; nWidth = metalGraphicsDesktopWidth; nHeight = metalGraphicsDesktopHeight; }\n"
            "\tSetFormatedText(\"METAL_RESOLUTION_VALUE\", sPrefix + nWidth + \" x \" + nHeight + \"  (\" + FormatMetalAspect(nWidth,nHeight) + \")\");\n"
            "}\n",
        ),
    ),
    "RESOURCE/INI/interfaces/option_screen.ini": (
        (
            "item = 105,BUTTON,METAL_RESOLUTION_PREV\n",
            "item = 105,FORMATEDTEXT,METAL_RESOLUTION_LABEL\n"
            "item = 105,BUTTON,METAL_RESOLUTION_PREV\n",
        ),
        (
            "nodelist = METAL_RESOLUTION_PREV,METAL_RESOLUTION_VALUE,METAL_RESOLUTION_NEXT\n",
            "nodelist = METAL_RESOLUTION_LABEL,METAL_RESOLUTION_PREV,METAL_RESOLUTION_VALUE,METAL_RESOLUTION_NEXT\n",
        ),
        (
            "[METAL_RESOLUTION_PREV]\n",
            "[METAL_RESOLUTION_LABEL]\n"
            "position = 170,315,630,335\n"
            "fontscale = 0.8\n"
            "lineSpace = 14\n"
            "alignment = center\n"
            "valignment = 1\n"
            "string = Resolution\n"
            "\n"
            "[METAL_RESOLUTION_PREV]\n",
        ),
        ("position = 180,330,220,370\n", "position = 180,340,220,380\n"),
        ("position = 230,330,570,370\n", "position = 225,340,575,380\n"),
        ("position = 580,330,620,370\n", "position = 580,340,620,380\n"),
    ),
    "RESOURCE/INI/texts/russian/common.ini": (
        (
            'string = Resolution Desktop,"Рабочий стол"\n',
            'string = Resolution Desktop,"Рабочий стол"\n'
            'string = Resolution,"Разрешение"\n',
        ),
    ),
}


def _build_desktop_picker_files() -> tuple[FilePatch, ...]:
    patched_hashes = {
        "PROGRAM/interface/option_sl.c": "bf564e35c6f035e6229e4ec4782f37dc588422ebd1836a8087d1edf8cd04f489",
        "PROGRAM/interface/option_screen.c": "7dc31fe0feaa3c0121026ed94adf64115fef47322489d9c2b6e29200fba258bc",
        "RESOURCE/INI/interfaces/option_screen.ini": "b620b6ceaed1fc78891c922191999c9a4e1e2674d7d90a2e0e53bb938bee78d1",
        "RESOURCE/INI/texts/russian/common.ini": "5cfe93d5c6b618c531deb4951b02477489e305dc38144dd60127284ef89294e0",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.patched_sha256,
            patched_hashes[spec.relative_path],
            _DESKTOP_PICKER_EXTRAS.get(spec.relative_path, ()),
        )
        for spec in RESOLUTION_PICKER_FILES
    )


DESKTOP_PICKER_FILES = _build_desktop_picker_files()

# The grouped settings layer owns the final scrollable graphics panel and keeps
# display geometry separate from the saved Current/custom resolution.
_GROUPED_SETTINGS_EXTRAS = {'PROGRAM/interface/option_sl.c': (('\toptref.resolution_width = 1920;\n\toptref.resolution_height = 1080;\n',
                                    '\toptref.resolution_width = 1920;\n'
                                    '\toptref.resolution_height = 1080;\n'
                                    '\toptref.display_width = 1920;\n'
                                    '\toptref.display_height = 1080;\n'),
                                   ('\toptref.resolution_width = InterfaceStates.MetalResolutionWidth;\n'
                                    '\toptref.resolution_height = InterfaceStates.MetalResolutionHeight;\n',
                                    '\toptref.resolution_width = InterfaceStates.MetalResolutionWidth;\n'
                                    '\toptref.resolution_height = InterfaceStates.MetalResolutionHeight;\n'
                                    '\toptref.display_width = InterfaceStates.MetalDisplayWidth;\n'
                                    '\toptref.display_height = InterfaceStates.MetalDisplayHeight;\n'),
                                   ('\tInterfaceStates.MetalResolutionWidth = sti(optref.resolution_width);\n'
                                    '\tInterfaceStates.MetalResolutionHeight = sti(optref.resolution_height);\n'
                                    '\tif(sti(InterfaceStates.MetalResolution)<0 || '
                                    'sti(InterfaceStates.MetalResolution)>5) InterfaceStates.MetalResolution = 2;\n'
                                    '\tif(sti(InterfaceStates.MetalResolutionWidth)<320 || '
                                    'sti(InterfaceStates.MetalResolutionHeight)<200) {\n'
                                    '\t\tInterfaceStates.MetalResolutionWidth = 1920;\n'
                                    '\t\tInterfaceStates.MetalResolutionHeight = 1080;\n'
                                    '\t}\n',
                                    '\tInterfaceStates.MetalResolutionWidth = sti(optref.resolution_width);\n'
                                    '\tInterfaceStates.MetalResolutionHeight = sti(optref.resolution_height);\n'
                                    '\tInterfaceStates.MetalDisplayWidth = sti(optref.display_width);\n'
                                    '\tInterfaceStates.MetalDisplayHeight = sti(optref.display_height);\n'
                                    '\tif(sti(InterfaceStates.MetalResolution)<0 || '
                                    'sti(InterfaceStates.MetalResolution)>5) InterfaceStates.MetalResolution = 2;\n'
                                    '\tif(sti(InterfaceStates.MetalResolutionWidth)<320 || '
                                    'sti(InterfaceStates.MetalResolutionHeight)<200) {\n'
                                    '\t\tInterfaceStates.MetalResolutionWidth = 1920;\n'
                                    '\t\tInterfaceStates.MetalResolutionHeight = 1080;\n'
                                    '\t}\n'
                                    '\tif(sti(InterfaceStates.MetalDisplayWidth)<320 || '
                                    'sti(InterfaceStates.MetalDisplayHeight)<200) {\n'
                                    '\t\tInterfaceStates.MetalDisplayWidth = '
                                    'sti(InterfaceStates.MetalResolutionWidth);\n'
                                    '\t\tInterfaceStates.MetalDisplayHeight = '
                                    'sti(InterfaceStates.MetalResolutionHeight);\n'
                                    '\t}\n'),
                                   ('void SaveMetalGraphicsOptions()\n'
                                    '{\n'
                                    '\tobject gopt;\n'
                                    '\tPrepareDefaultMetalGraphics(&gopt);\n'
                                    '\tGetMetalGraphicsOptions(&gopt);\n'
                                    '\tSendMessage(&GameInterface, "lsa", MSG_INTERFACE_SAVEOPTIONS, "metal_graphics", '
                                    'gopt);\n'
                                    '}\n',
                                    'bool SaveMetalGraphicsOptions()\n'
                                    '{\n'
                                    '\tobject gopt;\n'
                                    '\tPrepareDefaultMetalGraphics(&gopt);\n'
                                    '\tGetMetalGraphicsOptions(&gopt);\n'
                                    '\treturn SendMessage(&GameInterface, "lsa", MSG_INTERFACE_SAVEOPTIONS, '
                                    '"metal_graphics", gopt);\n'
                                    '}\n')),
 'PROGRAM/interface/option_screen.c': (('\tSetEventHandler("evFaderFrame","FaderFrame",0);\n',
                                        '\tSetEventHandler("evFaderFrame","FaderFrame",0);\n'
                                        '\tSetEventHandler("ievnt_command","ProcessMetalGraphicsCommand",0);\n'
                                        '\tSetEventHandler("OnTableClick","ClickMetalGraphicsRow",0);\n'
                                        '\tSetEventHandler("TableActivate","ActivateMetalGraphicsRow",0);\n'),
                                       ('\tDelEventHandler("evFaderFrame","FaderFrame");\n'
                                        '\tDelEventHandler("InterfaceBreak","ProcessCancelExit");  // boal\n',
                                        '\tDelEventHandler("evFaderFrame","FaderFrame");\n'
                                        '\tDelEventHandler("ievnt_command","ProcessMetalGraphicsCommand");\n'
                                        '\tDelEventHandler("OnTableClick","ClickMetalGraphicsRow");\n'
                                        '\tDelEventHandler("TableActivate","ActivateMetalGraphicsRow");\n'
                                        '\tDelEventHandler("InterfaceBreak","ProcessCancelExit");  // boal\n'),
                                       ('SetCurrentNode("METAL_DYNAMIC_LIGHTING_CHECKBOX");',
                                        'SetCurrentNode("METAL_GRAPHICS_LIST");'),
                                       ('\tif( sNodName == "METAL_RESOLUTION_PREV" ) {\n'
                                        '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) - '
                                        '1;\n'
                                        '\t\tif(sti(InterfaceStates.MetalResolution) < 0) '
                                        'InterfaceStates.MetalResolution = 5;\n'
                                        '\t\tUpdateMetalResolutionLabel();\n'
                                        '\t\treturn;\n'
                                        '\t}\n'
                                        '\tif( sNodName == "METAL_RESOLUTION_NEXT" ) {\n'
                                        '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) + '
                                        '1;\n'
                                        '\t\tif(sti(InterfaceStates.MetalResolution) > 5) '
                                        'InterfaceStates.MetalResolution = 0;\n'
                                        '\t\tUpdateMetalResolutionLabel();\n'
                                        '\t\treturn;\n'
                                        '\t}\n',
                                        ''),
                                       ('void ApplyMetalGraphicsOptions()\n'
                                        '{\n'
                                        '\tint flagsMask = 0;\n'
                                        '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
                                        '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
                                        '\tint nWidth;\n'
                                        '\tint nHeight;\n'
                                        '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
                                        '\tint desktopMode = 0;\n'
                                        '\tif(nBaseWidth < 320 || nBaseHeight < 200 || nBaseWidth > 16384 || '
                                        'nBaseHeight > 16384) {\n'
                                        '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics '
                                        'Apply Error"));\n'
                                        '\t\treturn;\n'
                                        '\t}\n'
                                        '\tif(nResolution < 0 || nResolution > 5) {\n'
                                        '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics '
                                        'Apply Error"));\n'
                                        '\t\treturn;\n'
                                        '\t}\n'
                                        '\tif(sti(InterfaceStates.MetalDynamicLighting)) flagsMask = flagsMask + 1;\n'
                                        '\tif(sti(InterfaceStates.MetalModernWater)) flagsMask = flagsMask + 2;\n'
                                        '\tif(sti(InterfaceStates.MetalModernLighting)) flagsMask = flagsMask + 4;\n'
                                        '\tif(sti(InterfaceStates.MetalCinematic)) flagsMask = flagsMask + 8;\n'
                                        '\tif(sti(InterfaceStates.MetalLightShafts)) flagsMask = flagsMask + 16;\n'
                                        '\tif(sti(InterfaceStates.MetalDynamicSky)) flagsMask = flagsMask + 32;\n'
                                        '\tif(sti(InterfaceStates.MetalAntialiasing)) flagsMask = flagsMask + 64;\n'
                                        '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
                                        '\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==5 && sti(InterfaceStates.MetalFullScreen)) {\n'
                                        '\t\tnWidth = metalGraphicsDesktopWidth;\n'
                                        '\t\tnHeight = metalGraphicsDesktopHeight;\n'
                                        '\t\tdesktopMode = 1;\n'
                                        '\t}\n'
                                        '\t'
                                        'if(SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),nWidth,nHeight,desktopMode)) '
                                        '{\n'
                                        '\t\tSaveMetalGraphicsOptions();\n'
                                        '\t\tCloseMetalGraphicsWindow();\n'
                                        '\t\treturn;\n'
                                        '\t}\n'
                                        '\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics '
                                        'Apply Error"));\n'
                                        '}\n'
                                        '\n'
                                        'void RefreshMetalDesktopSize()\n'
                                        '{\n'
                                        '\tint fallbackWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
                                        '\tint fallbackHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
                                        '\tif(fallbackWidth < 320 || fallbackHeight < 200) { fallbackWidth = 1920; '
                                        'fallbackHeight = 1080; }\n'
                                        '\tmetalGraphicsDesktopWidth = fallbackWidth;\n'
                                        '\tmetalGraphicsDesktopHeight = fallbackHeight;\n'
                                        '\tint displayWidth = SendMessage(&GameInterface,"l",45063);\n'
                                        '\tint displayHeight = SendMessage(&GameInterface,"l",45064);\n'
                                        '\tif(displayWidth >= 320 && displayHeight >= 200) {\n'
                                        '\t\tmetalGraphicsDesktopWidth = displayWidth;\n'
                                        '\t\tmetalGraphicsDesktopHeight = displayHeight;\n'
                                        '\t}\n'
                                        '}\n'
                                        '\n'
                                        'void GetMetalGraphicsData()\n'
                                        '{\n'
                                        '\tRefreshMetalDesktopSize();\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_DYNAMIC_LIGHTING_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalDynamicLighting));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_SHADOW_QUALITY_RADIO", '
                                        '2, sti(InterfaceStates.MetalShadowQuality)+1, true);\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_MODERN_WATER_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalModernWater));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_MODERN_LIGHTING_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalModernLighting));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_CINEMATIC_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalCinematic));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_LIGHT_SHAFTS_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalLightShafts));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_DYNAMIC_SKY_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalDynamicSky));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_ANTIALIASING_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalAntialiasing));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lslll",MSG_INTERFACE_MSG_TO_NODE,"METAL_FULLSCREEN_CHECKBOX", '
                                        '2, 1, sti(InterfaceStates.MetalFullScreen));\n'
                                        '\tUpdateMetalResolutionLabel();\n'
                                        '}\n'
                                        '\n'
                                        'string FormatMetalAspect(int nWidth, int nHeight)\n'
                                        '{\n'
                                        '\tint nAspect = makeint((nWidth * 100 + nHeight / 2) / nHeight);\n'
                                        '\tint nWhole = makeint(nAspect / 100);\n'
                                        '\tint nFraction = nAspect % 100;\n'
                                        '\tstring sAspect = "" + nWhole + ".";\n'
                                        '\tif(nFraction < 10) sAspect = sAspect + "0";\n'
                                        '\treturn sAspect + nFraction + ":1";\n'
                                        '}\n'
                                        '\n'
                                        'void UpdateMetalResolutionLabel()\n'
                                        '{\n'
                                        '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
                                        '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
                                        '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
                                        '\tint nWidth;\n'
                                        '\tint nHeight;\n'
                                        '\tstring sPrefix = "";\n'
                                        '\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; '
                                        'InterfaceStates.MetalResolution = 2; }\n'
                                        '\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight '
                                        '= 1080; }\n'
                                        '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
                                        '\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight '
                                        '+ nBaseWidth / 2) / nBaseWidth); }\n'
                                        '\tif(nResolution==4) sPrefix = XI_ConvertString("Resolution Current") + ": '
                                        '";\n'
                                        '\tif(nResolution==5) { sPrefix = XI_ConvertString("Resolution Desktop") + ": '
                                        '"; nWidth = metalGraphicsDesktopWidth; nHeight = metalGraphicsDesktopHeight; '
                                        '}\n'
                                        '\tSetFormatedText("METAL_RESOLUTION_VALUE", sPrefix + nWidth + " x " + '
                                        'nHeight + "  (" + FormatMetalAspect(nWidth,nHeight) + ")");\n'
                                        '}\n'
                                        '\n',
                                        'void ResolveMetalResolution(ref result, int nResolution)\n'
                                        '{\n'
                                        '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
                                        '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
                                        '\tint nDisplayWidth = metalGraphicsDesktopWidth;\n'
                                        '\tint nDisplayHeight = metalGraphicsDesktopHeight;\n'
                                        '\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight '
                                        '= 1080; }\n'
                                        '\tif(nDisplayWidth < 320 || nDisplayHeight < 200) { nDisplayWidth = '
                                        'nBaseWidth; nDisplayHeight = nBaseHeight; }\n'
                                        '\tresult.width = nBaseWidth;\n'
                                        '\tresult.height = nBaseHeight;\n'
                                        '\tresult.desktop = 0;\n'
                                        '\tif(nResolution==0) result.width = 1280;\n'
                                        '\tif(nResolution==1) result.width = 1600;\n'
                                        '\tif(nResolution==2) result.width = 1920;\n'
                                        '\tif(nResolution==3) result.width = 2560;\n'
                                        '\tif(nResolution>=0 && nResolution<=3) result.height = '
                                        'makeint((sti(result.width) * nDisplayHeight + nDisplayWidth / 2) / '
                                        'nDisplayWidth);\n'
                                        '\tif(nResolution==5) {\n'
                                        '\t\tresult.width = nDisplayWidth;\n'
                                        '\t\tresult.height = nDisplayHeight;\n'
                                        '\t\tif(sti(InterfaceStates.MetalFullScreen)) result.desktop = 1;\n'
                                        '\t}\n'
                                        '}\n'
                                        '\n'
                                        'string FormatMetalAspect(int nWidth, int nHeight)\n'
                                        '{\n'
                                        '\tint nA = nWidth;\n'
                                        '\tint nB = nHeight;\n'
                                        '\tint nRemainder;\n'
                                        '\twhile(nB > 0) {\n'
                                        '\t\tnRemainder = nA % nB;\n'
                                        '\t\tnA = nB;\n'
                                        '\t\tnB = nRemainder;\n'
                                        '\t}\n'
                                        '\tif(nA > 0 && nWidth / nA <= 32 && nHeight / nA <= 32) return "" + nWidth / '
                                        'nA + ":" + nHeight / nA;\n'
                                        '\tint nAspect = makeint((nWidth * 100 + nHeight / 2) / nHeight);\n'
                                        '\tint nWhole = makeint(nAspect / 100);\n'
                                        '\tint nFraction = nAspect % 100;\n'
                                        '\tstring sAspect = "" + nWhole + ".";\n'
                                        '\tif(nFraction < 10) sAspect = sAspect + "0";\n'
                                        '\treturn sAspect + nFraction + ":1";\n'
                                        '}\n'
                                        '\n'
                                        'string FormatMetalResolution()\n'
                                        '{\n'
                                        '\tobject resolved;\n'
                                        '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
                                        '\tstring sPrefix = "";\n'
                                        '\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; '
                                        'InterfaceStates.MetalResolution = 2; }\n'
                                        '\tResolveMetalResolution(&resolved, nResolution);\n'
                                        '\tif(nResolution==4) sPrefix = XI_ConvertString("Resolution Current") + ": '
                                        '";\n'
                                        '\tif(nResolution==5) sPrefix = XI_ConvertString("Resolution Desktop") + ": '
                                        '";\n'
                                        '\treturn sPrefix + resolved.width + " x " + resolved.height + "  (" + '
                                        'FormatMetalAspect(sti(resolved.width),sti(resolved.height)) + ")";\n'
                                        '}\n'
                                        '\n'
                                        'string MetalGraphicsToggleLabel(int nEnabled)\n'
                                        '{\n'
                                        '\tif(nEnabled) return XI_ConvertString("Yes");\n'
                                        '\treturn XI_ConvertString("No");\n'
                                        '}\n'
                                        '\n'
                                        'string MetalGraphicsShadowLabel()\n'
                                        '{\n'
                                        '\tint nQuality = sti(InterfaceStates.MetalShadowQuality);\n'
                                        '\tif(nQuality==0) return XI_ConvertString("Shadow Quality Low");\n'
                                        '\tif(nQuality==2) return XI_ConvertString("Shadow Quality High");\n'
                                        '\treturn XI_ConvertString("Shadow Quality Medium");\n'
                                        '}\n'
                                        '\n'
                                        'void SetMetalGraphicsRow(int nRow, string sGroup, string sName, string '
                                        'sValue)\n'
                                        '{\n'
                                        '\tstring sRow = "tr" + nRow;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td1.str = sGroup;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td2.str = sName;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td3.str = "<  " + sValue + "  >";\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td1.scale = 0.78;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td2.scale = 0.82;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td3.scale = 0.82;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.(sRow).td3.align = "center";\n'
                                        '}\n'
                                        '\n'
                                        'void GetMetalGraphicsData()\n'
                                        '{\n'
                                        '\tint nSelected = 1;\n'
                                        '\tint nTop = 0;\n'
                                        '\tRefreshMetalDesktopSize();\n'
                                        '\tif(CheckAttribute(&GameInterface,"METAL_GRAPHICS_LIST.select")) nSelected = '
                                        'sti(GameInterface.METAL_GRAPHICS_LIST.select);\n'
                                        '\tif(CheckAttribute(&GameInterface,"METAL_GRAPHICS_LIST.top")) nTop = '
                                        'sti(GameInterface.METAL_GRAPHICS_LIST.top);\n'
                                        '\tDeleteAttribute(&GameInterface,"METAL_GRAPHICS_LIST");\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.select = nSelected;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.top = nTop;\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.hr.td1.str = XI_ConvertString("Metal '
                                        'Graphics Group");\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.hr.td2.str = XI_ConvertString("Metal '
                                        'Graphics Setting");\n'
                                        '\tGameInterface.METAL_GRAPHICS_LIST.hr.td3.str = XI_ConvertString("Metal '
                                        'Graphics Value");\n'
                                        '\tSetMetalGraphicsRow(1, XI_ConvertString("Metal Display Group"), '
                                        'XI_ConvertString("Metal Fullscreen"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalFullScreen)));\n'
                                        '\tSetMetalGraphicsRow(2, "", XI_ConvertString("Resolution"), '
                                        'FormatMetalResolution());\n'
                                        '\tSetMetalGraphicsRow(3, XI_ConvertString("Metal Lighting Group"), '
                                        'XI_ConvertString("Metal Dynamic Shadows"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalDynamicLighting)));\n'
                                        '\tSetMetalGraphicsRow(4, "", XI_ConvertString("Metal Shadow Quality"), '
                                        'MetalGraphicsShadowLabel());\n'
                                        '\tSetMetalGraphicsRow(5, "", XI_ConvertString("Metal Modern Lighting"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalModernLighting)));\n'
                                        '\tSetMetalGraphicsRow(6, "", XI_ConvertString("Metal Light Shafts"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalLightShafts)));\n'
                                        '\tSetMetalGraphicsRow(7, "", XI_ConvertString("Metal Dynamic Sky"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalDynamicSky)));\n'
                                        '\tSetMetalGraphicsRow(8, "", XI_ConvertString("Metal Cinematic Interior"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalCinematic)));\n'
                                        '\tSetMetalGraphicsRow(9, XI_ConvertString("Metal Effects Group"), '
                                        'XI_ConvertString("Metal Modern Water"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalModernWater)));\n'
                                        '\tSetMetalGraphicsRow(10, "", XI_ConvertString("Metal FXAA"), '
                                        'MetalGraphicsToggleLabel(sti(InterfaceStates.MetalAntialiasing)));\n'
                                        '\t'
                                        'SendMessage(&GameInterface,"lsl",MSG_INTERFACE_MSG_TO_NODE,"METAL_GRAPHICS_LIST",0);\n'
                                        '}\n'
                                        '\n'
                                        'void ChangeMetalGraphicsRow(int nRow, int nDirection)\n'
                                        '{\n'
                                        '\tif(nDirection==0) nDirection = 1;\n'
                                        '\tif(nRow==1) {\n'
                                        '\t\tInterfaceStates.MetalFullScreen = !sti(InterfaceStates.MetalFullScreen);\n'
                                        '\t\tif(!sti(InterfaceStates.MetalFullScreen) && '
                                        'sti(InterfaceStates.MetalResolution)==5) InterfaceStates.MetalResolution = '
                                        '4;\n'
                                        '\t}\n'
                                        '\tif(nRow==2) {\n'
                                        '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) + '
                                        'nDirection;\n'
                                        '\t\tif(sti(InterfaceStates.MetalFullScreen)) {\n'
                                        '\t\t\tif(sti(InterfaceStates.MetalResolution)<0) '
                                        'InterfaceStates.MetalResolution = 5;\n'
                                        '\t\t\tif(sti(InterfaceStates.MetalResolution)>5) '
                                        'InterfaceStates.MetalResolution = 0;\n'
                                        '\t\t} else {\n'
                                        '\t\t\tif(sti(InterfaceStates.MetalResolution)<0) '
                                        'InterfaceStates.MetalResolution = 4;\n'
                                        '\t\t\tif(sti(InterfaceStates.MetalResolution)>4) '
                                        'InterfaceStates.MetalResolution = 0;\n'
                                        '\t\t}\n'
                                        '\t}\n'
                                        '\tif(nRow==3) InterfaceStates.MetalDynamicLighting = '
                                        '!sti(InterfaceStates.MetalDynamicLighting);\n'
                                        '\tif(nRow==4) {\n'
                                        '\t\tInterfaceStates.MetalShadowQuality = '
                                        'sti(InterfaceStates.MetalShadowQuality) + nDirection;\n'
                                        '\t\tif(sti(InterfaceStates.MetalShadowQuality)<0) '
                                        'InterfaceStates.MetalShadowQuality = 2;\n'
                                        '\t\tif(sti(InterfaceStates.MetalShadowQuality)>2) '
                                        'InterfaceStates.MetalShadowQuality = 0;\n'
                                        '\t}\n'
                                        '\tif(nRow==5) InterfaceStates.MetalModernLighting = '
                                        '!sti(InterfaceStates.MetalModernLighting);\n'
                                        '\tif(nRow==6) InterfaceStates.MetalLightShafts = '
                                        '!sti(InterfaceStates.MetalLightShafts);\n'
                                        '\tif(nRow==7) InterfaceStates.MetalDynamicSky = '
                                        '!sti(InterfaceStates.MetalDynamicSky);\n'
                                        '\tif(nRow==8) InterfaceStates.MetalCinematic = '
                                        '!sti(InterfaceStates.MetalCinematic);\n'
                                        '\tif(nRow==9) InterfaceStates.MetalModernWater = '
                                        '!sti(InterfaceStates.MetalModernWater);\n'
                                        '\tif(nRow==10) InterfaceStates.MetalAntialiasing = '
                                        '!sti(InterfaceStates.MetalAntialiasing);\n'
                                        '\tGetMetalGraphicsData();\n'
                                        '}\n'
                                        '\n'
                                        'void ProcessMetalGraphicsCommand()\n'
                                        '{\n'
                                        '\tstring sCommand = GetEventData();\n'
                                        '\tstring sNode = GetEventData();\n'
                                        '\tif(sNode != "METAL_GRAPHICS_LIST") return;\n'
                                        '\tif(sCommand=="leftstep" || sCommand=="speedleft") '
                                        'ChangeMetalGraphicsRow(sti(GameInterface.METAL_GRAPHICS_LIST.select),-1);\n'
                                        '\tif(sCommand=="rightstep" || sCommand=="speedright") '
                                        'ChangeMetalGraphicsRow(sti(GameInterface.METAL_GRAPHICS_LIST.select),1);\n'
                                        '}\n'
                                        '\n'
                                        'void ClickMetalGraphicsRow()\n'
                                        '{\n'
                                        '\tstring sNode = GetEventData();\n'
                                        '\tint nRow = GetEventData();\n'
                                        '\tint nColumn = GetEventData();\n'
                                        '\tif(sNode=="METAL_GRAPHICS_LIST") ChangeMetalGraphicsRow(nRow+1,1);\n'
                                        '}\n'
                                        '\n'
                                        'void ActivateMetalGraphicsRow()\n'
                                        '{\n'
                                        '\tstring sNode = GetEventData();\n'
                                        '\tint nRow = GetEventData();\n'
                                        '\tif(sNode=="METAL_GRAPHICS_LIST") ChangeMetalGraphicsRow(nRow+1,1);\n'
                                        '}\n'
                                        '\n'
                                        'void ApplyMetalGraphicsOptions()\n'
                                        '{\n'
                                        '\tobject resolved;\n'
                                        '\tint flagsMask = 0;\n'
                                        '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
                                        '\tif(nResolution < 0 || nResolution > 5) {\n'
                                        '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics '
                                        'Apply Error"));\n'
                                        '\t\treturn;\n'
                                        '\t}\n'
                                        '\tif(!sti(InterfaceStates.MetalFullScreen) && nResolution==5) {\n'
                                        '\t\tnResolution = 4;\n'
                                        '\t\tInterfaceStates.MetalResolution = 4;\n'
                                        '\t}\n'
                                        '\tif(sti(InterfaceStates.MetalDynamicLighting)) flagsMask = flagsMask + 1;\n'
                                        '\tif(sti(InterfaceStates.MetalModernWater)) flagsMask = flagsMask + 2;\n'
                                        '\tif(sti(InterfaceStates.MetalModernLighting)) flagsMask = flagsMask + 4;\n'
                                        '\tif(sti(InterfaceStates.MetalCinematic)) flagsMask = flagsMask + 8;\n'
                                        '\tif(sti(InterfaceStates.MetalLightShafts)) flagsMask = flagsMask + 16;\n'
                                        '\tif(sti(InterfaceStates.MetalDynamicSky)) flagsMask = flagsMask + 32;\n'
                                        '\tif(sti(InterfaceStates.MetalAntialiasing)) flagsMask = flagsMask + 64;\n'
                                        '\tResolveMetalResolution(&resolved,nResolution);\n'
                                        '\t'
                                        'if(SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),sti(resolved.width),sti(resolved.height),sti(resolved.desktop))) '
                                        '{\n'
                                        '\t\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
                                        '\t\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
                                        '\t\tSaveMetalGraphicsOptions();\n'
                                        '\t\tCloseMetalGraphicsWindow();\n'
                                        '\t\treturn;\n'
                                        '\t}\n'
                                        '\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics '
                                        'Apply Error"));\n'
                                        '}\n'
                                        '\n'
                                        'void RefreshMetalDesktopSize()\n'
                                        '{\n'
                                        '\tint fallbackWidth = sti(InterfaceStates.MetalDisplayWidth);\n'
                                        '\tint fallbackHeight = sti(InterfaceStates.MetalDisplayHeight);\n'
                                        '\tif(fallbackWidth < 320 || fallbackHeight < 200) {\n'
                                        '\t\tfallbackWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
                                        '\t\tfallbackHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
                                        '\t}\n'
                                        '\tif(fallbackWidth < 320 || fallbackHeight < 200) { fallbackWidth = 1920; '
                                        'fallbackHeight = 1080; }\n'
                                        '\tmetalGraphicsDesktopWidth = fallbackWidth;\n'
                                        '\tmetalGraphicsDesktopHeight = fallbackHeight;\n'
                                        '\tint displayWidth = SendMessage(&GameInterface,"l",45063);\n'
                                        '\tint displayHeight = SendMessage(&GameInterface,"l",45064);\n'
                                        '\tif(displayWidth >= 320 && displayHeight >= 200) {\n'
                                        '\t\tmetalGraphicsDesktopWidth = displayWidth;\n'
                                        '\t\tmetalGraphicsDesktopHeight = displayHeight;\n'
                                        '\t}\n'
                                        '}\n'
                                        '\n'),
                                       ('\t\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
                                        '\t\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
                                        '\t\tSaveMetalGraphicsOptions();\n'
                                        '\t\tCloseMetalGraphicsWindow();\n'
                                        '\t\treturn;\n',
                                        '\t\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
                                        '\t\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
                                        '\t\tif(SaveMetalGraphicsOptions()) {\n'
                                        '\t\t\tCloseMetalGraphicsWindow();\n'
                                        '\t\t\treturn;\n'
                                        '\t\t}\n')),
 'RESOURCE/INI/interfaces/option_screen.ini': (('item = 105,CHECKBUTTON,METAL_DYNAMIC_LIGHTING_CHECKBOX\n'
                                                'item = 105,RADIOBUTTON,METAL_SHADOW_QUALITY_RADIO\n'
                                                'item = 105,CHECKBUTTON,METAL_MODERN_WATER_CHECKBOX\n'
                                                'item = 105,CHECKBUTTON,METAL_MODERN_LIGHTING_CHECKBOX\n'
                                                'item = 105,CHECKBUTTON,METAL_CINEMATIC_CHECKBOX\n'
                                                'item = 105,CHECKBUTTON,METAL_LIGHT_SHAFTS_CHECKBOX\n'
                                                'item = 105,CHECKBUTTON,METAL_DYNAMIC_SKY_CHECKBOX\n'
                                                'item = 105,CHECKBUTTON,METAL_ANTIALIASING_CHECKBOX\n'
                                                'item = 105,CHECKBUTTON,METAL_FULLSCREEN_CHECKBOX\n'
                                                'item = 105,FORMATEDTEXT,METAL_RESOLUTION_LABEL\n'
                                                'item = 105,BUTTON,METAL_RESOLUTION_PREV\n'
                                                'item = 105,FORMATEDTEXT,METAL_RESOLUTION_VALUE\n'
                                                'item = 105,BUTTON,METAL_RESOLUTION_NEXT\n',
                                                'item = 105,TABLE,METAL_GRAPHICS_LIST\n'
                                                'item = 106,SCROLLER,METAL_GRAPHICS_SCROLL\n'),
                                               ('nodelist = '
                                                'METAL_GRAPHICS_FRAME,METAL_GRAPHICS_TITLE,METAL_GRAPHICS_ERROR,METAL_GRAPHICS_APPLY,METAL_GRAPHICS_CANCEL\n'
                                                'nodelist = '
                                                'METAL_DYNAMIC_LIGHTING_CHECKBOX,METAL_MODERN_WATER_CHECKBOX,METAL_MODERN_LIGHTING_CHECKBOX,METAL_CINEMATIC_CHECKBOX\n'
                                                'nodelist = '
                                                'METAL_LIGHT_SHAFTS_CHECKBOX,METAL_DYNAMIC_SKY_CHECKBOX,METAL_ANTIALIASING_CHECKBOX,METAL_FULLSCREEN_CHECKBOX,METAL_SHADOW_QUALITY_RADIO\n'
                                                'nodelist = '
                                                'METAL_RESOLUTION_LABEL,METAL_RESOLUTION_PREV,METAL_RESOLUTION_VALUE,METAL_RESOLUTION_NEXT\n',
                                                'nodelist = '
                                                'METAL_GRAPHICS_FRAME,METAL_GRAPHICS_TITLE,METAL_GRAPHICS_ERROR,METAL_GRAPHICS_APPLY,METAL_GRAPHICS_CANCEL\n'
                                                'nodelist = METAL_GRAPHICS_LIST,METAL_GRAPHICS_SCROLL\n'),
                                               ('[METAL_DYNAMIC_LIGHTING_CHECKBOX]\n'
                                                'command = click\n'
                                                'command = '
                                                'rclick,event:ShowInfo,select:METAL_DYNAMIC_LIGHTING_CHECKBOX\n'
                                                'position = 170,170,390,190\n'
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
                                                'position = 410,170,630,190\n'
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
                                                'command = '
                                                'rclick,event:ShowInfo,select:METAL_MODERN_LIGHTING_CHECKBOX\n'
                                                'position = 170,200,390,220\n'
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
                                                'position = 410,200,630,220\n'
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
                                                'position = 170,230,390,250\n'
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
                                                'position = 410,230,630,250\n'
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
                                                'position = 170,260,390,280\n'
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
                                                'position = 410,260,630,280\n'
                                                'alignment = left\n'
                                                'iconsize = 15,15\n'
                                                'individualpos = 1\n'
                                                'rect_textoffset = 25,0,0,0\n'
                                                'section1 = 0,0,Metal Fullscreen\n'
                                                'fontScale = 0.85\n'
                                                'bShowGlowCursor = 0\n'
                                                '\n'
                                                '[METAL_SHADOW_QUALITY_RADIO]\n'
                                                'command = click\n'
                                                'command = upstep\n'
                                                'command = downstep\n'
                                                'command = rightstep\n'
                                                'command = leftstep\n'
                                                'position = 170,290,630,310\n'
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
                                                '[METAL_RESOLUTION_LABEL]\n'
                                                'position = 170,315,630,335\n'
                                                'fontscale = 0.8\n'
                                                'lineSpace = 14\n'
                                                'alignment = center\n'
                                                'valignment = 1\n'
                                                'string = Resolution\n'
                                                '\n'
                                                '[METAL_RESOLUTION_PREV]\n'
                                                'command = click,event:eventBtnAction\n'
                                                'command = activate,event:eventBtnAction\n'
                                                'command = rightstep,select:METAL_RESOLUTION_NEXT\n'
                                                'position = 180,340,220,380\n'
                                                'group = ARROWS\n'
                                                'picture = left\n'
                                                'shadowOffset = 3,2\n'
                                                '\n'
                                                '[METAL_RESOLUTION_VALUE]\n'
                                                'position = 225,340,575,380\n'
                                                'fontscale = 0.9\n'
                                                'lineSpace = 16\n'
                                                'alignment = center\n'
                                                'valignment = 1\n'
                                                '\n'
                                                '[METAL_RESOLUTION_NEXT]\n'
                                                'command = click,event:eventBtnAction\n'
                                                'command = activate,event:eventBtnAction\n'
                                                'command = leftstep,select:METAL_RESOLUTION_PREV\n'
                                                'position = 580,340,620,380\n'
                                                'group = ARROWS\n'
                                                'picture = right\n'
                                                'shadowOffset = 3,2\n'
                                                '\n',
                                                '[METAL_GRAPHICS_LIST]\n'
                                                'command = click,select:METAL_GRAPHICS_LIST\n'
                                                'command = dblclick\n'
                                                'command = activate\n'
                                                'command = upstep\n'
                                                'command = downstep\n'
                                                'command = speedup\n'
                                                'command = speeddown\n'
                                                'command = leftstep\n'
                                                'command = rightstep\n'
                                                'command = speedleft\n'
                                                'command = speedright\n'
                                                'position = 150,165,640,405\n'
                                                'scroller = METAL_GRAPHICS_SCROLL\n'
                                                'rowquantity = 8\n'
                                                'colquantity = 3\n'
                                                'vlinewidth = 0\n'
                                                'hlineheight = 1\n'
                                                'headerlineheight = 1\n'
                                                'borderwidth = 1\n'
                                                'bordercornersize = 0,0\n'
                                                'colswidth = 90,205,195\n'
                                                'rowsheight = 24,30,30,30,30,30,30,30\n'
                                                'fontcellalignment = left\n'
                                                'fontcellvalignment = center\n'
                                                'cellspacesize = 5,3\n'
                                                'fontcellscale = 0.82\n'
                                                'fonttitlescale = 0.78\n'
                                                'fonttitlealignment = left\n'
                                                '\n'
                                                '[METAL_GRAPHICS_SCROLL]\n'
                                                'command = click,select:METAL_GRAPHICS_LIST\n'
                                                'command = upstep\n'
                                                'command = downstep\n'
                                                'position = 640,165,652,405\n'
                                                'bUseWheel = 1\n'
                                                'ownedControl = METAL_GRAPHICS_LIST\n'
                                                '\n')),
 'RESOURCE/INI/texts/russian/common.ini': (('string = Resolution,"Разрешение"\n',
                                            'string = Resolution,"Разрешение"\n'
                                            'string = Metal Graphics Group,"Раздел"\n'
                                            'string = Metal Graphics Setting,"Параметр"\n'
                                            'string = Metal Graphics Value,"Значение"\n'
                                            'string = Metal Display Group,"Экран"\n'
                                            'string = Metal Lighting Group,"Освещение"\n'
                                            'string = Metal Effects Group,"Эффекты"\n'
                                            'string = Metal Shadow Quality,"Качество теней"\n'),)}


def _build_grouped_settings_files() -> tuple[FilePatch, ...]:
    patched_hashes = {
        "PROGRAM/interface/option_sl.c": "cd30be0b7d892276b4bdfa86fbf0aff48480b8da5d9f63ebfd4f61a5dfccf2a9",
        "PROGRAM/interface/option_screen.c": "acce3bc399a2d8c46a55324df4f42565b636267d19c83f0e015f09386d9f5f92",
        "RESOURCE/INI/interfaces/option_screen.ini": "adb73d53b051b98523d11f642c8d67da757edd89af6d3bdfb8f679a8de045a28",
        "RESOURCE/INI/texts/russian/common.ini": "603ce127c9ec57e4feafd83f22c301f0bf3c7e8f82562036d3c198a0040264ad",
    }
    return tuple(
        FilePatch(
            spec.relative_path,
            spec.patched_sha256,
            patched_hashes[spec.relative_path],
            _GROUPED_SETTINGS_EXTRAS.get(spec.relative_path, ()),
        )
        for spec in DESKTOP_PICKER_FILES
    )


GROUPED_SETTINGS_FILES = _build_grouped_settings_files()

# The first grouped candidate left the removed picker callbacks in procBtnAction.
# Their call to the removed UpdateMetalResolutionLabel function prevented the
# Options script from loading, so accept that exact staged hash for one upgrade.
_SETTINGS_OPEN_FIX = (
    '\tif( sNodName == "METAL_RESOLUTION_PREV" ) {\n'
    '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) - 1;\n'
    '\t\tif(sti(InterfaceStates.MetalResolution) < 0) InterfaceStates.MetalResolution = 5;\n'
    '\t\tUpdateMetalResolutionLabel();\n'
    '\t\treturn;\n'
    '\t}\n'
    '\tif( sNodName == "METAL_RESOLUTION_NEXT" ) {\n'
    '\t\tInterfaceStates.MetalResolution = sti(InterfaceStates.MetalResolution) + 1;\n'
    '\t\tif(sti(InterfaceStates.MetalResolution) > 5) InterfaceStates.MetalResolution = 0;\n'
    '\t\tUpdateMetalResolutionLabel();\n'
    '\t\treturn;\n'
    '\t}\n',
    '',
)

OPEN_FIX_FILES = (
    FilePatch(
        "PROGRAM/interface/option_screen.c",
        "7fc4d4a8470bc911a55cbb97fe151bfdbbd4e184d7f6c1de592ac6c04a962a89",
        "eec48e24d2943c6a603110dbe73af6425bb35892fe74a2db466cdf7637e847b7",
        (_SETTINGS_OPEN_FIX,),
    ),
)

_PERSISTENCE_CONFIRMATION_EXTRAS = {
    "PROGRAM/interface/option_sl.c": ((
        'void SaveMetalGraphicsOptions()\n{\n\tobject gopt;\n'
        '\tPrepareDefaultMetalGraphics(&gopt);\n\tGetMetalGraphicsOptions(&gopt);\n'
        '\tSendMessage(&GameInterface, "lsa", MSG_INTERFACE_SAVEOPTIONS, "metal_graphics", gopt);\n}\n',
        'bool SaveMetalGraphicsOptions()\n{\n\tobject gopt;\n'
        '\tPrepareDefaultMetalGraphics(&gopt);\n\tGetMetalGraphicsOptions(&gopt);\n'
        '\treturn SendMessage(&GameInterface, "lsa", MSG_INTERFACE_SAVEOPTIONS, "metal_graphics", gopt);\n}\n',
    ),),
    "PROGRAM/interface/option_screen.c": ((
        '\t\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
        '\t\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
        '\t\tSaveMetalGraphicsOptions();\n\t\tCloseMetalGraphicsWindow();\n\t\treturn;\n',
        '\t\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
        '\t\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
        '\t\tif(SaveMetalGraphicsOptions()) {\n\t\t\tCloseMetalGraphicsWindow();\n'
        '\t\t\treturn;\n\t\t}\n',
    ),),
}

_PERSISTENCE_PREDECESSOR_HASHES = {
    "PROGRAM/interface/option_sl.c": "ab42500bfbaff46001725c4b3baf0adac8ac94882be95ce6dc02944afc2f6c13",
    "PROGRAM/interface/option_screen.c": "eec48e24d2943c6a603110dbe73af6425bb35892fe74a2db466cdf7637e847b7",
}

PERSISTENCE_FIX_FILES = tuple(
    FilePatch(
        spec.relative_path,
        _PERSISTENCE_PREDECESSOR_HASHES.get(spec.relative_path, spec.patched_sha256),
        spec.patched_sha256,
        _PERSISTENCE_CONFIRMATION_EXTRAS.get(spec.relative_path, ()),
    )
    for spec in GROUPED_SETTINGS_FILES
)

_SAFE_SCREEN_APPLY_EXTRAS = {
    "PROGRAM/interface/option_sl.c": ((
        '\tSaveSavedOptions(&gopt);\n\tSaveMetalGraphicsOptions();\n}',
        '\tSaveSavedOptions(&gopt);\n\tif(!SaveMetalGraphicsOptions()) {}\n}',
    ),),
    "PROGRAM/interface/option_screen.c": (
        (
            '\tint flagsMask = 0;\n\tint nResolution = sti(InterfaceStates.MetalResolution);',
            '\tint flagsMask = 0;\n\tint applyResult = 0;\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);',
        ),
        (
            '\tif(SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),sti(resolved.width),sti(resolved.height),sti(resolved.desktop))) {',
            '\tapplyResult = SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),sti(resolved.width),sti(resolved.height),sti(resolved.desktop));\n'
            '\tif(applyResult > 0) {',
        ),
        (
            '\t\tif(SaveMetalGraphicsOptions()) {\n\t\t\tCloseMetalGraphicsWindow();\n'
            '\t\t\treturn;\n\t\t}',
            '\t\tif(SaveMetalGraphicsOptions()) {\n'
            '\t\t\tif(applyResult == 2) {\n'
            '\t\t\t\tSnapshotMetalGraphicsOptions();\n'
            '\t\t\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Restart Required"));\n'
            '\t\t\t\treturn;\n\t\t\t}\n'
            '\t\t\tCloseMetalGraphicsWindow();\n\t\t\treturn;\n\t\t}',
        ),
    ),
    "RESOURCE/INI/texts/russian/common.ini": ((
        'string = Metal Shadow Quality,"Качество теней"\n',
        'string = Metal Shadow Quality,"Качество теней"\n'
        'string = Metal Graphics Restart Required,"Сохранено. Режим экрана применится после перезапуска"\n',
    ),),
}

_DEFERRED_APPLY_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        (
            "int metalGraphicsDesktopHeight = 1080;\n",
            "int metalGraphicsDesktopHeight = 1080;\n"
            "int metalGraphicsPendingFlags = 0;\n"
            "int metalGraphicsPendingShadowQuality = 1;\n"
            "int metalGraphicsPendingFullscreen = 1;\n"
            "int metalGraphicsPendingWidth = 1920;\n"
            "int metalGraphicsPendingHeight = 1080;\n"
            "int metalGraphicsPendingDesktop = 0;\n",
        ),
        (
            '\tSetEventHandler("TableActivate","ActivateMetalGraphicsRow",0);\n',
            '\tSetEventHandler("TableActivate","ActivateMetalGraphicsRow",0);\n'
            '\tSetEventHandler("evApplyMetalGraphicsDeferred","ApplyMetalGraphicsDeferred",0);\n'
            '\tSetEventHandler("evPollMetalGraphicsApply","PollMetalGraphicsApply",0);\n',
        ),
        (
            '\tDelEventHandler("TableActivate","ActivateMetalGraphicsRow");\n',
            '\tDelEventHandler("TableActivate","ActivateMetalGraphicsRow");\n'
            '\tDelEventHandler("evApplyMetalGraphicsDeferred","ApplyMetalGraphicsDeferred");\n'
            '\tDelEventHandler("evPollMetalGraphicsApply","PollMetalGraphicsApply");\n',
        ),
        (
            '\tif( sNodName == "METAL_GRAPHICS_BTN" ) {\n'
            '\t\tSnapshotMetalGraphicsOptions();\n',
            '\tif( sNodName == "METAL_GRAPHICS_BTN" ) {\n'
            '\t\tButton_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics"));\n'
            '\t\tSnapshotMetalGraphicsOptions();\n',
        ),
        (
            'void ApplyMetalGraphicsOptions()\n'
            '{\n'
            '\tobject resolved;\n'
            '\tint flagsMask = 0;\n'
            '\tint applyResult = 0;\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tif(nResolution < 0 || nResolution > 5) {\n'
            '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif(!sti(InterfaceStates.MetalFullScreen) && nResolution==5) {\n'
            '\t\tnResolution = 4;\n'
            '\t\tInterfaceStates.MetalResolution = 4;\n'
            '\t}\n'
            '\tif(sti(InterfaceStates.MetalDynamicLighting)) flagsMask = flagsMask + 1;\n'
            '\tif(sti(InterfaceStates.MetalModernWater)) flagsMask = flagsMask + 2;\n'
            '\tif(sti(InterfaceStates.MetalModernLighting)) flagsMask = flagsMask + 4;\n'
            '\tif(sti(InterfaceStates.MetalCinematic)) flagsMask = flagsMask + 8;\n'
            '\tif(sti(InterfaceStates.MetalLightShafts)) flagsMask = flagsMask + 16;\n'
            '\tif(sti(InterfaceStates.MetalDynamicSky)) flagsMask = flagsMask + 32;\n'
            '\tif(sti(InterfaceStates.MetalAntialiasing)) flagsMask = flagsMask + 64;\n'
            '\tResolveMetalResolution(&resolved,nResolution);\n'
            '\tapplyResult = SendMessage(&GameInterface,"lllllll",45062,flagsMask,sti(InterfaceStates.MetalShadowQuality),sti(InterfaceStates.MetalFullScreen),sti(resolved.width),sti(resolved.height),sti(resolved.desktop));\n'
            '\tif(applyResult > 0) {\n'
            '\t\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
            '\t\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
            '\t\tif(SaveMetalGraphicsOptions()) {\n'
            '\t\t\tif(applyResult == 2) {\n'
            '\t\t\t\tSnapshotMetalGraphicsOptions();\n'
            '\t\t\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Restart Required"));\n'
            '\t\t\t\treturn;\n'
            '\t\t\t}\n'
            '\t\t\tCloseMetalGraphicsWindow();\n'
            '\t\t\treturn;\n'
            '\t\t}\n'
            '\t}\n'
            '\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));\n'
            '}\n',
            'void ApplyMetalGraphicsOptions()\n'
            '{\n'
            '\tobject resolved;\n'
            '\tint flagsMask = 0;\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tif(nResolution < 0 || nResolution > 5) {\n'
            '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Apply Error"));\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tif(!sti(InterfaceStates.MetalFullScreen) && nResolution==5) { nResolution = 4; InterfaceStates.MetalResolution = 4; }\n'
            '\tif(sti(InterfaceStates.MetalDynamicLighting)) flagsMask = flagsMask + 1;\n'
            '\tif(sti(InterfaceStates.MetalModernWater)) flagsMask = flagsMask + 2;\n'
            '\tif(sti(InterfaceStates.MetalModernLighting)) flagsMask = flagsMask + 4;\n'
            '\tif(sti(InterfaceStates.MetalCinematic)) flagsMask = flagsMask + 8;\n'
            '\tif(sti(InterfaceStates.MetalLightShafts)) flagsMask = flagsMask + 16;\n'
            '\tif(sti(InterfaceStates.MetalDynamicSky)) flagsMask = flagsMask + 32;\n'
            '\tif(sti(InterfaceStates.MetalAntialiasing)) flagsMask = flagsMask + 64;\n'
            '\tResolveMetalResolution(&resolved,nResolution);\n'
            '\tInterfaceStates.MetalDisplayWidth = metalGraphicsDesktopWidth;\n'
            '\tInterfaceStates.MetalDisplayHeight = metalGraphicsDesktopHeight;\n'
            '\tif(!SaveMetalGraphicsOptions()) {\n'
            '\t\tSetFormatedText("METAL_GRAPHICS_ERROR", XI_ConvertString("Metal Graphics Save Error"));\n'
            '\t\treturn;\n'
            '\t}\n'
            '\tmetalGraphicsPendingFlags = flagsMask;\n'
            '\tmetalGraphicsPendingShadowQuality = sti(InterfaceStates.MetalShadowQuality);\n'
            '\tmetalGraphicsPendingFullscreen = sti(InterfaceStates.MetalFullScreen);\n'
            '\tmetalGraphicsPendingWidth = sti(resolved.width);\n'
            '\tmetalGraphicsPendingHeight = sti(resolved.height);\n'
            '\tmetalGraphicsPendingDesktop = sti(resolved.desktop);\n'
            '\tSnapshotMetalGraphicsOptions();\n'
            '\tCloseMetalGraphicsWindow();\n'
            '\tButton_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Applying"));\n'
            '\tPostEvent("evApplyMetalGraphicsDeferred",1);\n'
            '}\n'
            '\n'
            'void ApplyMetalGraphicsDeferred()\n'
            '{\n'
            '\tint queued = SendMessage(&GameInterface,"lllllll",45062,metalGraphicsPendingFlags,metalGraphicsPendingShadowQuality,metalGraphicsPendingFullscreen,metalGraphicsPendingWidth,metalGraphicsPendingHeight,metalGraphicsPendingDesktop);\n'
            '\tif(queued != 3) { Button_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Apply Failed")); return; }\n'
            '\tPostEvent("evPollMetalGraphicsApply",50);\n'
            '}\n'
            '\n'
            'void PollMetalGraphicsApply()\n'
            '{\n'
            '\tint result = SendMessage(&GameInterface,"l",45065);\n'
            '\tif(result == 0) { PostEvent("evPollMetalGraphicsApply",50); return; }\n'
            '\tif(result == 1) Button_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Applied"));\n'
            '\tif(result == 2) Button_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Restart Required Short"));\n'
            '}\n',
        ),
    ),
    "RESOURCE/INI/texts/russian/common.ini": ((
        'string = Metal Graphics Restart Required,"Сохранено. Режим экрана применится после перезапуска"\n',
        'string = Metal Graphics Restart Required,"Сохранено. Режим экрана применится после перезапуска"\n'
        'string = Metal Graphics Save Error,"Не удалось сохранить настройки"\n'
        'string = Metal Graphics Applying,"Графика: применяется…"\n'
        'string = Metal Graphics Applied,"Графика: применено"\n'
        'string = Metal Graphics Apply Failed,"Графика: не применена"\n'
        'string = Metal Graphics Restart Required Short,"Сохранено — применится после перезапуска"\n',
    ),),
}

_SAFE_SCREEN_APPLY_OLD_HASHES = {
    "PROGRAM/interface/option_sl.c": "cd30be0b7d892276b4bdfa86fbf0aff48480b8da5d9f63ebfd4f61a5dfccf2a9",
    "PROGRAM/interface/option_screen.c": "acce3bc399a2d8c46a55324df4f42565b636267d19c83f0e015f09386d9f5f92",
    "RESOURCE/INI/texts/russian/common.ini": "603ce127c9ec57e4feafd83f22c301f0bf3c7e8f82562036d3c198a0040264ad",
}

_SAFE_SCREEN_APPLY_HASHES = {
    "PROGRAM/interface/option_sl.c": "e71319811cf493a2bc1004070bb18664152ddf190dddfc87f377bc2cf6a9881f",
    "PROGRAM/interface/option_screen.c": "a73d5a8d9d136109bd02339df781b2b45c470574aada3bf59eba6df14b2d0067",
    "RESOURCE/INI/interfaces/option_screen.ini": "adb73d53b051b98523d11f642c8d67da757edd89af6d3bdfb8f679a8de045a28",
    "RESOURCE/INI/texts/russian/common.ini": "53be0bb2aad48f8a780450a1b901e00ad2ab1ebba853b3b705d7a8d5adac996e",
}

SAFE_SCREEN_APPLY_FILES = tuple(
    FilePatch(
        spec.relative_path,
        _SAFE_SCREEN_APPLY_OLD_HASHES.get(spec.relative_path, spec.patched_sha256),
        _SAFE_SCREEN_APPLY_HASHES[spec.relative_path],
        _SAFE_SCREEN_APPLY_EXTRAS.get(spec.relative_path, ()),
    )
    for spec in GROUPED_SETTINGS_FILES
)

DISPLAY_QUERY_FIX = FilePatch(
    "PROGRAM/interface/option_screen.c",
    "a73d5a8d9d136109bd02339df781b2b45c470574aada3bf59eba6df14b2d0067",
    "99be4d0983356e97491aecfdb033bdddf4c9ce15f11d3c2d4f1d706dfc5b7626",
    ((
        '\tobject desktopSize;\n'
        '\tint fallbackWidth = sti(InterfaceStates.MetalDisplayWidth);\n'
        '\tint fallbackHeight = sti(InterfaceStates.MetalDisplayHeight);\n'
        '\tif(fallbackWidth < 320 || fallbackHeight < 200) {\n'
        '\t\tfallbackWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
        '\t\tfallbackHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
        '\t}\n'
        '\tif(fallbackWidth < 320 || fallbackHeight < 200) { fallbackWidth = 1920; fallbackHeight = 1080; }\n'
        '\tmetalGraphicsDesktopWidth = fallbackWidth;\n'
        '\tmetalGraphicsDesktopHeight = fallbackHeight;\n'
        '\tif(SendMessage(&GameInterface,"la",45063,desktopSize)) {\n'
        '\t\tif(CheckAttribute(&desktopSize,"width") && CheckAttribute(&desktopSize,"height")) {\n'
        '\t\t\tmetalGraphicsDesktopWidth = sti(desktopSize.width);\n'
        '\t\t\tmetalGraphicsDesktopHeight = sti(desktopSize.height);\n'
        '\t\t}\n'
        '\t}\n',
        '\tint fallbackWidth = sti(InterfaceStates.MetalDisplayWidth);\n'
        '\tint fallbackHeight = sti(InterfaceStates.MetalDisplayHeight);\n'
        '\tif(fallbackWidth < 320 || fallbackHeight < 200) {\n'
        '\t\tfallbackWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
        '\t\tfallbackHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
        '\t}\n'
        '\tif(fallbackWidth < 320 || fallbackHeight < 200) { fallbackWidth = 1920; fallbackHeight = 1080; }\n'
        '\tmetalGraphicsDesktopWidth = fallbackWidth;\n'
        '\tmetalGraphicsDesktopHeight = fallbackHeight;\n'
        '\tint displayWidth = SendMessage(&GameInterface,"l",45063);\n'
        '\tint displayHeight = SendMessage(&GameInterface,"l",45064);\n'
        '\tif(displayWidth >= 320 && displayHeight >= 200) {\n'
        '\t\tmetalGraphicsDesktopWidth = displayWidth;\n'
        '\t\tmetalGraphicsDesktopHeight = displayHeight;\n'
        '\t}\n',
    ),),
)

_DEFERRED_APPLY_PREDECESSOR_HASHES = {
    "PROGRAM/interface/option_screen.c": "99be4d0983356e97491aecfdb033bdddf4c9ce15f11d3c2d4f1d706dfc5b7626",
    "RESOURCE/INI/texts/russian/common.ini": "53be0bb2aad48f8a780450a1b901e00ad2ab1ebba853b3b705d7a8d5adac996e",
}

_DEFERRED_APPLY_HASHES = {
    "PROGRAM/interface/option_screen.c": "51860387a69f2a1faf8c7ce085c160a1962f3e347abb45a8afa9d7ea783a576c",
    "RESOURCE/INI/texts/russian/common.ini": "47418b9e307f836966b718a2c44998f14e9ccbdd0953d642b7cb064779deea47",
}

DEFERRED_APPLY_FIXES = tuple(
    FilePatch(relative_path, predecessor, _DEFERRED_APPLY_HASHES[relative_path],
              _DEFERRED_APPLY_EXTRAS[relative_path])
    for relative_path, predecessor in _DEFERRED_APPLY_PREDECESSOR_HASHES.items()
)

# The Options screen stays loaded while option_sl.c is only a temporary segment.
# Load that segment around the save just like interface.c already does for the
# legacy options record, so the VM never calls an invalidated function entry.
_APPLY_SAVE_FIX_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        (
            'void ApplyMetalGraphicsOptions()\n',
            'extern bool SaveMetalGraphicsOptions();\n\n'
            'bool SaveMetalGraphicsOptionsNow()\n'
            '{\n'
            '\tbool saved = false;\n'
            '\tif(LoadSegment("interface\\option_sl.c")) {\n'
            '\t\tsaved = SaveMetalGraphicsOptions();\n'
            '\t\tUnloadSegment("interface\\option_sl.c");\n'
            '\t}\n'
            '\treturn saved;\n'
            '}\n\n'
            'void ApplyMetalGraphicsOptions()\n',
        ),
        (
            '\tif(!SaveMetalGraphicsOptions()) {',
            '\tif(!SaveMetalGraphicsOptionsNow()) {',
        ),
    ),
}

APPLY_SAVE_FIXES = (
    FilePatch(
        "PROGRAM/interface/option_screen.c",
        "51860387a69f2a1faf8c7ce085c160a1962f3e347abb45a8afa9d7ea783a576c",
        "ea92367497b966371bc7d5db2d50685dff199106e02f5ded15b9f140028d1690",
        _APPLY_SAVE_FIX_EXTRAS["PROGRAM/interface/option_screen.c"],
    ),
)

# Renderer feature flags are safe to apply on the next frame.  Screen geometry
# stays launch-owned because a live fullscreen/device transition invalidates
# legacy interface resources.  Keep the existing Graphics button label stable.
_STABLE_APPLY_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        ('\t\tButton_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics"));\n', ''),
        ('\tmetalGraphicsPendingFullscreen = sti(InterfaceStates.MetalFullScreen);',
         '\tmetalGraphicsPendingFullscreen = sti(Render.full_screen);'),
        ('\tmetalGraphicsPendingWidth = sti(resolved.width);',
         '\tmetalGraphicsPendingWidth = sti(Render.screen_x);'),
        ('\tmetalGraphicsPendingHeight = sti(resolved.height);',
         '\tmetalGraphicsPendingHeight = sti(Render.screen_y);'),
        ('\tmetalGraphicsPendingDesktop = sti(resolved.desktop);',
         '\tmetalGraphicsPendingDesktop = 0;'),
        ('\tButton_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Applying"));\n', ''),
        ('\tif(queued != 3) { Button_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Apply Failed")); return; }',
         '\tif(queued != 3) return;'),
        ('\tif(result == 1) Button_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Applied"));\n'
         '\tif(result == 2) Button_SetText("METAL_GRAPHICS_BTN", XI_ConvertString("Metal Graphics Restart Required Short"));\n',
         ''),
    ),
}

STABLE_APPLY_FIXES = (
    FilePatch(
        "PROGRAM/interface/option_screen.c",
        APPLY_SAVE_FIXES[0].patched_sha256,
        "129c11262b9599dd3e89825385f57d70637eac26b317027620aeeeca0d6035e0",
        _STABLE_APPLY_EXTRAS["PROGRAM/interface/option_screen.c"],
    ),
)

# One staged candidate briefly used fixed 16:9 preset heights. Keep its exact
# hash as a migration input so the active runtime can move to Mac-aspect values
# without replacing the PROGRAM tree.
_FIXED_PICKER_UPGRADE_EXTRAS = {
    "PROGRAM/interface/option_screen.c": (
        (
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(nResolution==0) { nWidth = 1280; nHeight = 720; }\n'
            '\tif(nResolution==1) { nWidth = 1600; nHeight = 900; }\n'
            '\tif(nResolution==2) { nWidth = 1920; nHeight = 1080; }\n'
            '\tif(nResolution==3) { nWidth = 2560; nHeight = 1440; }\n'
            '\tif(nResolution==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;\n',
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==5 && sti(InterfaceStates.MetalFullScreen)) desktopMode = 1;\n',
        ),
        (
            'void UpdateMetalResolutionLabel()\n'
            '{\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tstring sResolution;\n'
            '\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; InterfaceStates.MetalResolution = 2; }\n'
            '\tif(nResolution==0) sResolution = XI_ConvertString("Resolution 1280x720");\n'
            '\tif(nResolution==1) sResolution = XI_ConvertString("Resolution 1600x900");\n'
            '\tif(nResolution==2) sResolution = XI_ConvertString("Resolution 1920x1080");\n'
            '\tif(nResolution==3) sResolution = XI_ConvertString("Resolution 2560x1440");\n',
            'void UpdateMetalResolutionLabel()\n'
            '{\n'
            '\tint nResolution = sti(InterfaceStates.MetalResolution);\n'
            '\tint nBaseWidth = sti(InterfaceStates.MetalResolutionWidth);\n'
            '\tint nBaseHeight = sti(InterfaceStates.MetalResolutionHeight);\n'
            '\tint nWidth;\n'
            '\tint nHeight;\n'
            '\tstring sResolution;\n'
            '\tif(nResolution < 0 || nResolution > 5) { nResolution = 2; InterfaceStates.MetalResolution = 2; }\n'
            '\tif(nBaseWidth < 320 || nBaseHeight < 200) { nBaseWidth = 1920; nBaseHeight = 1080; }\n'
            '\tnWidth = nBaseWidth; nHeight = nBaseHeight;\n'
            '\tif(nResolution==0) { nWidth = 1280; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==1) { nWidth = 1600; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==2) { nWidth = 1920; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution==3) { nWidth = 2560; nHeight = makeint((nWidth * nBaseHeight + nBaseWidth / 2) / nBaseWidth); }\n'
            '\tif(nResolution>=0 && nResolution<=3) sResolution = "" + nWidth + " x " + nHeight;\n',
        ),
    ),
}

FIXED_PICKER_FILES = tuple(
    FilePatch(
        spec.relative_path,
        "ef0d4c8492f83d552620622ddf84a5d320643ae43695eca44e8dff218af63495"
        if spec.relative_path == "PROGRAM/interface/option_screen.c" else spec.patched_sha256,
        spec.patched_sha256,
        _FIXED_PICKER_UPGRADE_EXTRAS.get(spec.relative_path, ()),
    )
    for spec in RESOLUTION_PICKER_FILES
)

# FILES is the final delivered layer.  Its original hash remains the archived
# baseline so the normal sync path can prepare a clean source tree in one pass;
# predecessor layers separately recognize and upgrade each reviewed revision.
_FINAL_HASHES = {
    **_SAFE_SCREEN_APPLY_HASHES,
    **_DEFERRED_APPLY_HASHES,
    "PROGRAM/interface/option_screen.c": STABLE_APPLY_FIXES[0].patched_sha256,
}

FILES = tuple(
    FilePatch(
        current.relative_path,
        legacy.original_sha256,
        _FINAL_HASHES[current.relative_path],
        legacy.replacements + _CURRENT_EXTRAS.get(current.relative_path, ()) + _SHADOW_QUALITY_EXTRAS[current.relative_path] + _DISPLAY_MODE_EXTRAS[current.relative_path] + _LIVE_APPLY_EXTRAS.get(current.relative_path, ()) + _RESOLUTION_PICKER_EXTRAS.get(current.relative_path, ()) + _DESKTOP_PICKER_EXTRAS.get(current.relative_path, ()) + _GROUPED_SETTINGS_EXTRAS.get(current.relative_path, ()) + _SAFE_SCREEN_APPLY_EXTRAS.get(current.relative_path, ()) + _DEFERRED_APPLY_EXTRAS.get(current.relative_path, ()) + _APPLY_SAVE_FIX_EXTRAS.get(current.relative_path, ()) + _STABLE_APPLY_EXTRAS.get(current.relative_path, ()),
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
    is_width = key.endswith("_width")
    limit = (
        MIN_RESOLUTION_WIDTH if is_width else MIN_RESOLUTION_HEIGHT,
        MAX_RESOLUTION_WIDTH if is_width else MAX_RESOLUTION_HEIGHT,
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


def _stored_display_dimensions(
    values: dict[str, str], base_width: int, base_height: int
) -> tuple[int, int]:
    present = [key in values for key in ("display_width", "display_height")]
    if any(present) and not all(present):
        raise ValueError("display geometry requires both display_width and display_height")
    if not any(present):
        return base_width, base_height
    return (
        _dimension_value(values, "display_width"),
        _dimension_value(values, "display_height"),
    )


def _render_dimensions(
    resolution: int,
    base_width: int,
    base_height: int,
    display_width: int,
    display_height: int,
) -> tuple[int, int]:
    if resolution == CURRENT_RESOLUTION:
        return base_width, base_height
    if resolution == DESKTOP_RESOLUTION:
        return display_width, display_height
    width = RESOLUTIONS[resolution][0]
    height = (width * display_height + display_width // 2) // display_width
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
    display_width, display_height = _stored_display_dimensions(values, base_width, base_height)
    if resolution == DESKTOP_RESOLUTION and not result["full_screen"]:
        resolution = CURRENT_RESOLUTION
    result["resolution"] = resolution
    result["resolution_width"] = base_width
    result["resolution_height"] = base_height
    result["display_width"] = display_width
    result["display_height"] = display_height
    result["render_width"], result["render_height"] = _render_dimensions(
        resolution, base_width, base_height, display_width, display_height
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
    display_mode = (
        DISPLAY_MODE_DESKTOP
        if values["full_screen"] and int(values["resolution"]) == DESKTOP_RESOLUTION
        else DISPLAY_MODE_CUSTOM
    )
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
    values["display_width"] = values["resolution_width"]
    values["display_height"] = values["resolution_height"]
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
    stable_apply_fix = next((item for item in STABLE_APPLY_FIXES if item.relative_path == relative_path), None)
    if stable_apply_fix and current == stable_apply_fix.original_sha256:
        return transform(source, stable_apply_fix), "stable-apply-fix"
    apply_save_fix = next((item for item in APPLY_SAVE_FIXES if item.relative_path == relative_path), None)
    if apply_save_fix and current == apply_save_fix.original_sha256:
        return transform(source, apply_save_fix), "apply-save-segment-fix"
    if current == spec.patched_sha256 and spec.patched_sha256:
        return source, "patched"
    deferred_fix = next((item for item in DEFERRED_APPLY_FIXES if item.relative_path == relative_path), None)
    if deferred_fix and current == deferred_fix.original_sha256:
        return transform(source, deferred_fix), "deferred-apply-fix"
    if relative_path == DISPLAY_QUERY_FIX.relative_path and current == DISPLAY_QUERY_FIX.original_sha256:
        display_fixed = transform(source, DISPLAY_QUERY_FIX)
        return transform(display_fixed, deferred_fix), "display-query-and-deferred-apply-fix"
    safe_apply = next(item for item in SAFE_SCREEN_APPLY_FILES if item.relative_path == relative_path)
    if current == safe_apply.original_sha256 and safe_apply.replacements:
        return transform(source, safe_apply), "safe-screen-apply-fix"
    persistence_fix = next(item for item in PERSISTENCE_FIX_FILES if item.relative_path == relative_path)
    if current == persistence_fix.original_sha256 and persistence_fix.replacements:
        return transform(source, persistence_fix), "persistence-confirmation-fix"
    open_fix = next((item for item in OPEN_FIX_FILES if item.relative_path == relative_path), None)
    if open_fix and current == open_fix.original_sha256:
        opened = transform(source, open_fix)
        persistence_fix = next(
            item for item in PERSISTENCE_FIX_FILES if item.relative_path == relative_path
        )
        return transform(opened, persistence_fix), "settings-open-and-persistence-fix"
    grouped = next(item for item in GROUPED_SETTINGS_FILES if item.relative_path == relative_path)
    if current == grouped.original_sha256:
        return transform(source, grouped), "grouped-settings-upgrade"
    desktop_picker = next(item for item in DESKTOP_PICKER_FILES if item.relative_path == relative_path)
    if current == desktop_picker.original_sha256:
        return transform(source, desktop_picker), "desktop-picker-upgrade"
    fixed_picker = next(item for item in FIXED_PICKER_FILES if item.relative_path == relative_path)
    if current == fixed_picker.original_sha256 and fixed_picker.replacements:
        picker_source = transform(source, fixed_picker)
        return transform(picker_source, desktop_picker), "fixed-picker-upgrade"
    picker = next(item for item in RESOLUTION_PICKER_FILES if item.relative_path == relative_path)
    if current == picker.original_sha256:
        picker_source = transform(source, picker)
        return transform(picker_source, desktop_picker), "resolution-picker-upgrade"
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
    inputs = PROJECT / "experiments/native-metal/inputs"
    canonical_root = (inputs / "gameplay" if (inputs / "manifest.json").is_file()
                      else PROJECT / "experiments/native-storm/.cache/runtime")
    canonical_path = canonical_root / relative_path
    canonical = canonical_path.read_bytes()
    if digest(canonical) != spec.original_sha256:
        raise ValueError(f"{relative_path}: archived canonical source changed")
    if current == spec.patched_sha256 and transform(canonical, spec) == source:
        return canonical, "patched"
    stable_apply_fix = next((item for item in STABLE_APPLY_FIXES if item.relative_path == relative_path), None)
    if stable_apply_fix and current == stable_apply_fix.original_sha256:
        upgraded = transform(source, stable_apply_fix)
        stripped, _ = strip(relative_path, upgraded)
        return stripped, "stable-apply-predecessor"
    apply_save_fix = next((item for item in APPLY_SAVE_FIXES if item.relative_path == relative_path), None)
    if apply_save_fix and current == apply_save_fix.original_sha256:
        upgraded = transform(source, apply_save_fix)
        stripped, _ = strip(relative_path, upgraded)
        return stripped, "apply-save-segment-predecessor"
    deferred_fix = next((item for item in DEFERRED_APPLY_FIXES if item.relative_path == relative_path), None)
    if deferred_fix and current == deferred_fix.original_sha256:
        upgraded = transform(source, deferred_fix)
        stripped, _ = strip(relative_path, upgraded)
        return stripped, "deferred-apply-predecessor"
    if relative_path == DISPLAY_QUERY_FIX.relative_path and current == DISPLAY_QUERY_FIX.original_sha256:
        upgraded = transform(source, DISPLAY_QUERY_FIX)
        stripped, _ = strip(relative_path, upgraded)
        return stripped, "display-query-predecessor"
    safe_apply = next(item for item in SAFE_SCREEN_APPLY_FILES if item.relative_path == relative_path)
    if current == safe_apply.original_sha256 and safe_apply.replacements:
        predecessor = FilePatch(
            relative_path,
            spec.original_sha256,
            safe_apply.original_sha256,
            spec.replacements[:-len(safe_apply.replacements)],
        )
        if transform(canonical, predecessor) == source:
            return canonical, "safe-screen-apply-predecessor"
    persistence_fix = next(item for item in PERSISTENCE_FIX_FILES if item.relative_path == relative_path)
    if current == persistence_fix.original_sha256 and persistence_fix.replacements:
        predecessor = FilePatch(
            relative_path,
            spec.original_sha256,
            persistence_fix.original_sha256,
            spec.replacements[:-len(persistence_fix.replacements)],
        )
        if transform(canonical, predecessor) == source:
            return canonical, "persistence-confirmation-predecessor"
    if current == spec.patched_sha256 and spec.patched_sha256:
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
    persistence_fix = next(item for item in PERSISTENCE_FIX_FILES if item.relative_path == relative_path)
    if current == persistence_fix.original_sha256 and persistence_fix.replacements:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        predecessor_replacements = spec.replacements[:-len(persistence_fix.replacements)]
        for old, new in reversed(predecessor_replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: persistence predecessor is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != spec.original_sha256:
            raise ValueError(f"{relative_path}: stripped persistence predecessor is not canonical")
        return result, "persistence-confirmation-predecessor"
    grouped = next(item for item in GROUPED_SETTINGS_FILES if item.relative_path == relative_path)
    if current == grouped.original_sha256 and grouped.replacements:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        predecessor_replacements = spec.replacements[:-len(grouped.replacements)]
        for old, new in reversed(predecessor_replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: predecessor layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != spec.original_sha256:
            raise ValueError(f"{relative_path}: stripped predecessor is not canonical")
        return result, "grouped-settings-predecessor"
    if current == grouped.patched_sha256 and grouped.replacements:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(grouped.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: grouped settings layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != grouped.original_sha256:
            raise ValueError(f"{relative_path}: stripped grouped settings are not canonical")
        canonical, _ = strip(relative_path, result)
        return canonical, "patched"
    desktop_picker = next(item for item in DESKTOP_PICKER_FILES if item.relative_path == relative_path)
    if current == desktop_picker.patched_sha256 and desktop_picker.replacements:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(desktop_picker.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: desktop picker layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != desktop_picker.original_sha256:
            raise ValueError(f"{relative_path}: stripped desktop picker is not canonical")
        canonical, _ = strip(relative_path, result)
        return canonical, "patched"
    picker = next(item for item in RESOLUTION_PICKER_FILES if item.relative_path == relative_path)
    if current == picker.patched_sha256 and picker.replacements:
        result = source
        newline = b"\r\n" if b"\r\n" in source else b"\n"
        for old, new in reversed(picker.replacements):
            old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
            new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
            if result.count(new_bytes) != 1:
                raise ValueError(f"{relative_path}: resolution picker layer is not unique")
            result = result.replace(new_bytes, old_bytes, 1)
        if digest(result) != picker.original_sha256:
            raise ValueError(f"{relative_path}: stripped resolution picker is not canonical")
        canonical, _ = strip(relative_path, result)
        return canonical, "resolution-picker-patched"
    fixed_picker = next(item for item in FIXED_PICKER_FILES if item.relative_path == relative_path)
    if current == fixed_picker.original_sha256 and fixed_picker.replacements:
        upgraded = transform(source, fixed_picker)
        canonical, _ = strip(relative_path, upgraded)
        return canonical, "fixed-picker-patched"
    if spec.patched_sha256 and current == spec.patched_sha256:
        picker = next(item for item in RESOLUTION_PICKER_FILES if item.relative_path == relative_path)
        if picker.replacements:
            result = source
            newline = b"\r\n" if b"\r\n" in source else b"\n"
            for old, new in reversed(picker.replacements):
                old_bytes = old.replace("\n", newline.decode()).encode("utf-8")
                new_bytes = new.replace("\n", newline.decode()).encode("utf-8")
                if result.count(new_bytes) != 1:
                    raise ValueError(f"{relative_path}: resolution picker layer is not unique")
                result = result.replace(new_bytes, old_bytes, 1)
            if digest(result) != picker.original_sha256:
                raise ValueError(f"{relative_path}: stripped resolution picker is not canonical")
            canonical, _ = strip(relative_path, result)
            return canonical, "patched"
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
