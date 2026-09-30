#!/usr/bin/env python3
"""Exact-hash Metal main menu branding layer over original Reconstruction 1.4.1.

This helper owns the main menu branding transform:
- Replaces original Discord/VK with Telegram and Behance text buttons
- Adds bottom clickable banner 'IDDICTIVE REMASTER · macOS Edition'
- Replaces menu logo with new 2.8 aspect IDDICTIVE logo (1024x366 texture)
- Removes legacy QR code window and pointer/subscribe controls
- Preserves all 6 main menu action buttons, background, and content version
"""

from __future__ import annotations

import argparse
import hashlib
from dataclasses import dataclass
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_RUNTIME = PROJECT / "experiments/native-metal/.cache/runtime"

SCRIPT = "PROGRAM/interface/mainmenu.c"
LAYOUT = "RESOURCE/INI/interfaces/mainmenu.ini"
PICTURES = "RESOURCE/INI/interfaces/pictures.ini"

BASE = {
    SCRIPT: "5f73c2b07b34bdf72ba98dc58b469821c4e6c6f376bad73ab8f3ad291d615c0a",
    LAYOUT: "4725f28fb9393fc8be6397263d5bd1021b8708a297d94a81c663d7cbd177d39d",
    PICTURES: "045d5d3e07be77ca6259dc64b826a68145c16fd3114e64c6337578205fea9634",
}

UPDATED = {
    SCRIPT: "c3a2f9374a13de5922f7fbcf53d30dd71addede22934a58b1e5bfd6ea88704a8",
    LAYOUT: "9893484ba53d569653e93860979400b9b4d5ec912255cce8bcdae17bfee2f5e2",
    PICTURES: "8cc35fbd54af83c9bb2fc320c2555ebdabb9daf137c344e847ad09bb849303cb",
}

FILES = (SCRIPT, LAYOUT, PICTURES)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _transform_pictures(data: bytes) -> bytes:
    text = data.decode("utf-8")
    needle = "\r\n[MENU_BUTTONS]\r\n"
    new_group = (
        "\r\n[MENU_IDDICTIVE_LOGO]\r\n"
        "sTextureName\t\t= iddictive_menu_logo.tga\r\n"
        "wTextureWidth\t\t= 1024\r\n"
        "wTextureHeight\t\t= 366\r\n"
        "picture\t\t\t\t= Logo,0,0,1024,366\r\n"
        "\r\n[MENU_BUTTONS]\r\n"
    )
    if needle not in text:
        raise ValueError("missing [MENU_BUTTONS] in pictures.ini")
    return text.replace(needle, new_group, 1).encode("utf-8")


def _strip_pictures(data: bytes) -> bytes:
    text = data.decode("utf-8")
    new_group = (
        "\r\n[MENU_IDDICTIVE_LOGO]\r\n"
        "sTextureName\t\t= iddictive_menu_logo.tga\r\n"
        "wTextureWidth\t\t= 1024\r\n"
        "wTextureHeight\t\t= 366\r\n"
        "picture\t\t\t\t= Logo,0,0,1024,366\r\n"
        "\r\n[MENU_BUTTONS]\r\n"
    )
    needle = "\r\n[MENU_BUTTONS]\r\n"
    if new_group not in text:
        raise ValueError("missing [MENU_IDDICTIVE_LOGO] in pictures.ini")
    return text.replace(new_group, needle, 1).encode("utf-8")


def _transform_ini(data: bytes) -> bytes:
    text = data.decode("utf-8")

    # 1. Main items
    old_main_tail = (
        "item = SMALLBUTTON,BTN_QUIT\r\n"
        "item = SMALLBUTTON,BTN_DISCORD\r\n"
        "item = SMALLBUTTON,BTN_VK\r\n"
        "; CHANGES WINDOW\r\n"
        "item = 100,FRAME,CHANGES_FRAME_WINDOW\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_CAPTION\r\n"
        "item = 110,BUTTON,CHANGES_BTN_EXIT\r\n"
        "item = 110,FRAME,CHANGES_FRAME\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_TEXT\r\n"
        "item = 110,SCROLLER,CHANGES_SCROLL\r\n"
        "item = WINDOW,CHANGES_WINDOW\r\n"
        "; QR CODE WINDOW\r\n"
        "item = 100,PICTURE,QR_ZONE\r\n"
        "item = 110,PICTURE,QR_DISCORD\r\n"
        "item = 110,PICTURE,QR_VK\r\n"
        "item = WINDOW,QR_WINDOW\r\n"
    )
    new_main_tail = (
        "item = SMALLBUTTON,BTN_QUIT\r\n"
        "item = TEXTBUTTON2,BTN_BANNER\r\n"
        "item = TEXTBUTTON2,BTN_TELEGRAM\r\n"
        "item = TEXTBUTTON2,BTN_BEHANCE\r\n"
        "; CHANGES WINDOW\r\n"
        "item = 100,FRAME,CHANGES_FRAME_WINDOW\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_CAPTION\r\n"
        "item = 110,BUTTON,CHANGES_BTN_EXIT\r\n"
        "item = 110,FRAME,CHANGES_FRAME\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_TEXT\r\n"
        "item = 110,SCROLLER,CHANGES_SCROLL\r\n"
        "item = WINDOW,CHANGES_WINDOW\r\n"
    )
    if old_main_tail not in text:
        raise ValueError("missing old main tail in mainmenu.ini")
    text = text.replace(old_main_tail, new_main_tail, 1)

    old_pointer_sub = (
        "item = PICTURE,POINTER\r\n"
        "item = FORMATEDTEXT,SUBSCRIBE\r\n"
    )
    if old_pointer_sub not in text:
        raise ValueError("missing pointer/subscribe in mainmenu.ini")
    text = text.replace(old_pointer_sub, "", 1)

    # 2. LOGO
    old_logo = (
        "[LOGO]\r\n"
        "position = 10,20,350,165\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = Logo\r\n"
    )
    new_logo = (
        "[LOGO]\r\n"
        "position = 10,20,350,142\r\n"
        "groupName = MENU_IDDICTIVE_LOGO\r\n"
        "picName = Logo\r\n"
    )
    if old_logo not in text:
        raise ValueError("missing old logo in mainmenu.ini")
    text = text.replace(old_logo, new_logo, 1)

    # 3. Remove POINTER and SUBSCRIBE sections before [VERSION]
    old_pointer_sec = (
        "[POINTER]\r\n"
        "position = 622,20,645,55\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = Pointer\r\n"
        "\r\n"
        "[SUBSCRIBE]\r\n"
        "position = 643,43,740,53\r\n"
        "font = INTERFACE_NORMAL\r\n"
        "fontScale = 0.65\r\n"
        "lineSpace = 10\r\n"
        "alignment = center\r\n"
        "Color = 255,255,255,255\r\n"
        "\r\n"
    )
    if old_pointer_sec not in text:
        raise ValueError("missing old pointer section in mainmenu.ini")
    text = text.replace(old_pointer_sec, "", 1)

    # 4. Nav steps on main buttons
    for btn, prev_btn, next_btn in [
        ("BTN_NEWGAME", "BTN_BANNER", "BTN_LOAD"),
        ("BTN_LOAD", "BTN_NEWGAME", "BTN_OPTIONS"),
        ("BTN_OPTIONS", "BTN_LOAD", "BTN_CHANGES"),
        ("BTN_CHANGES", "BTN_OPTIONS", "BTN_CREDITS"),
        ("BTN_CREDITS", "BTN_CHANGES", "BTN_QUIT"),
        ("BTN_QUIT", "BTN_CREDITS", "BTN_BANNER"),
    ]:
        old_up = "BTN_QUIT" if btn == "BTN_NEWGAME" else prev_btn
        old_down = "BTN_NEWGAME" if btn == "BTN_QUIT" else next_btn
        old_steps = (
            f"command = upstep,select:{old_up}\r\n"
            f"command = downstep,select:{old_down}\r\n"
            "command = leftstep,select:BTN_VK\r\n"
            "command = rightstep,select:BTN_DISCORD\r\n"
        )
        new_steps = (
            f"command = upstep,select:{prev_btn}\r\n"
            f"command = downstep,select:{next_btn}\r\n"
            "command = leftstep,select:BTN_BEHANCE\r\n"
            "command = rightstep,select:BTN_TELEGRAM\r\n"
        )
        if old_steps not in text:
            raise ValueError(f"missing {old_steps} in {btn}")
        text = text.replace(old_steps, new_steps, 1)

    # 5. Replace BTN_DISCORD, BTN_VK, QR sections
    start_discord = text.index("[BTN_DISCORD]\r\n")
    changes_win = text.index("[CHANGES_WINDOW]\r\n")
    new_social_chunk = (
        "[BTN_BANNER]\r\n"
        "command = click,event:OpenBannerURL\r\n"
        "command = activate,event:OpenBannerURL\r\n"
        "command = deactivate,select:BTN_QUIT\r\n"
        "command = upstep,select:BTN_QUIT\r\n"
        "command = downstep,select:BTN_NEWGAME\r\n"
        "command = leftstep,select:BTN_BEHANCE\r\n"
        "command = rightstep,select:BTN_TELEGRAM\r\n"
        "position = 10,568,340,596\r\n"
        "string = #IDDICTIVE REMASTER · macOS Edition\r\n"
        "font = INTERFACE_NORMAL\r\n"
        "fontScale = 0.9\r\n"
        "strOffset = 7\r\n"
        "glowoffset = 0,0\r\n"
        "pressPictureOffset = 2,2\r\n"
        "\r\n"
        "[BTN_TELEGRAM]\r\n"
        "command = click,event:OpenTelegramURL\r\n"
        "command = activate,event:OpenTelegramURL\r\n"
        "command = deactivate,select:BTN_NEWGAME\r\n"
        "command = leftstep,select:BTN_NEWGAME\r\n"
        "command = rightstep,select:BTN_BEHANCE\r\n"
        "command = downstep,select:BTN_NEWGAME\r\n"
        "position = 580,4,680,32\r\n"
        "string = #Telegram\r\n"
        "fontScale = 0.75\r\n"
        "strOffset = 7\r\n"
        "glowoffset = 0,0\r\n"
        "pressPictureOffset = 2,2\r\n"
        "\r\n"
        "[BTN_BEHANCE]\r\n"
        "command = click,event:OpenBehanceURL\r\n"
        "command = activate,event:OpenBehanceURL\r\n"
        "command = deactivate,select:BTN_NEWGAME\r\n"
        "command = leftstep,select:BTN_TELEGRAM\r\n"
        "command = rightstep,select:BTN_NEWGAME\r\n"
        "command = downstep,select:BTN_NEWGAME\r\n"
        "position = 688,4,788,32\r\n"
        "string = #Behance\r\n"
        "fontScale = 0.75\r\n"
        "strOffset = 7\r\n"
        "glowoffset = 0,0\r\n"
        "pressPictureOffset = 2,2\r\n"
        "\r\n"
    )
    text = text[:start_discord] + new_social_chunk + text[changes_win:]

    # Remove QR_WINDOW and controls at the bottom
    qr_start = text.index("\r\n[QR_WINDOW]\r\n")
    text = text[:qr_start + 2]

    return text.encode("utf-8")


def _strip_ini(data: bytes) -> bytes:
    text = data.decode("utf-8")
    
    # 1. Main items
    old_main_tail = (
        "item = SMALLBUTTON,BTN_QUIT\r\n"
        "item = SMALLBUTTON,BTN_DISCORD\r\n"
        "item = SMALLBUTTON,BTN_VK\r\n"
        "; CHANGES WINDOW\r\n"
        "item = 100,FRAME,CHANGES_FRAME_WINDOW\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_CAPTION\r\n"
        "item = 110,BUTTON,CHANGES_BTN_EXIT\r\n"
        "item = 110,FRAME,CHANGES_FRAME\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_TEXT\r\n"
        "item = 110,SCROLLER,CHANGES_SCROLL\r\n"
        "item = WINDOW,CHANGES_WINDOW\r\n"
        "; QR CODE WINDOW\r\n"
        "item = 100,PICTURE,QR_ZONE\r\n"
        "item = 110,PICTURE,QR_DISCORD\r\n"
        "item = 110,PICTURE,QR_VK\r\n"
        "item = WINDOW,QR_WINDOW\r\n"
    )
    new_main_tail = (
        "item = SMALLBUTTON,BTN_QUIT\r\n"
        "item = TEXTBUTTON2,BTN_BANNER\r\n"
        "item = TEXTBUTTON2,BTN_TELEGRAM\r\n"
        "item = TEXTBUTTON2,BTN_BEHANCE\r\n"
        "; CHANGES WINDOW\r\n"
        "item = 100,FRAME,CHANGES_FRAME_WINDOW\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_CAPTION\r\n"
        "item = 110,BUTTON,CHANGES_BTN_EXIT\r\n"
        "item = 110,FRAME,CHANGES_FRAME\r\n"
        "item = 110,FORMATEDTEXT,CHANGES_TEXT\r\n"
        "item = 110,SCROLLER,CHANGES_SCROLL\r\n"
        "item = WINDOW,CHANGES_WINDOW\r\n"
    )
    if new_main_tail not in text:
        raise ValueError("missing new main tail in mainmenu.ini")
    text = text.replace(new_main_tail, old_main_tail, 1)

    old_pointer_sub = (
        "item = PICTURE,POINTER\r\n"
        "item = FORMATEDTEXT,SUBSCRIBE\r\n"
    )
    logo_needle = "item = PICTURE,LOGO\r\n"
    if logo_needle not in text:
        raise ValueError("missing logo needle in mainmenu.ini")
    text = text.replace(logo_needle, logo_needle + old_pointer_sub, 1)

    # 2. LOGO
    old_logo = (
        "[LOGO]\r\n"
        "position = 10,20,350,165\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = Logo\r\n"
    )
    new_logo = (
        "[LOGO]\r\n"
        "position = 10,20,350,142\r\n"
        "groupName = MENU_IDDICTIVE_LOGO\r\n"
        "picName = Logo\r\n"
    )
    if new_logo not in text:
        raise ValueError("missing new logo in mainmenu.ini")
    text = text.replace(new_logo, old_logo, 1)

    # 3. Restore POINTER and SUBSCRIBE sections before [VERSION]
    old_pointer_sec = (
        "[POINTER]\r\n"
        "position = 622,20,645,55\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = Pointer\r\n"
        "\r\n"
        "[SUBSCRIBE]\r\n"
        "position = 643,43,740,53\r\n"
        "font = INTERFACE_NORMAL\r\n"
        "fontScale = 0.65\r\n"
        "lineSpace = 10\r\n"
        "alignment = center\r\n"
        "Color = 255,255,255,255\r\n"
        "\r\n"
    )
    version_needle = "[VERSION]\r\n"
    if version_needle not in text:
        raise ValueError("missing [VERSION] in mainmenu.ini")
    text = text.replace(version_needle, old_pointer_sec + version_needle, 1)

    # 4. Nav steps on main buttons
    for btn, prev_btn, next_btn in [
        ("BTN_NEWGAME", "BTN_BANNER", "BTN_LOAD"),
        ("BTN_LOAD", "BTN_NEWGAME", "BTN_OPTIONS"),
        ("BTN_OPTIONS", "BTN_LOAD", "BTN_CHANGES"),
        ("BTN_CHANGES", "BTN_OPTIONS", "BTN_CREDITS"),
        ("BTN_CREDITS", "BTN_CHANGES", "BTN_QUIT"),
        ("BTN_QUIT", "BTN_CREDITS", "BTN_BANNER"),
    ]:
        old_up = "BTN_QUIT" if btn == "BTN_NEWGAME" else prev_btn
        old_down = "BTN_NEWGAME" if btn == "BTN_QUIT" else next_btn
        old_steps = (
            f"command = upstep,select:{old_up}\r\n"
            f"command = downstep,select:{old_down}\r\n"
            "command = leftstep,select:BTN_VK\r\n"
            "command = rightstep,select:BTN_DISCORD\r\n"
        )
        new_steps = (
            f"command = upstep,select:{prev_btn}\r\n"
            f"command = downstep,select:{next_btn}\r\n"
            "command = leftstep,select:BTN_BEHANCE\r\n"
            "command = rightstep,select:BTN_TELEGRAM\r\n"
        )
        if new_steps not in text:
            raise ValueError(f"missing {new_steps} in {btn}")
        text = text.replace(new_steps, old_steps, 1)

    # 5. Restore BTN_DISCORD, BTN_VK
    start_banner = text.index("[BTN_BANNER]\r\n")
    changes_win = text.index("[CHANGES_WINDOW]\r\n")
    
    old_social_chunk = (
        "[BTN_DISCORD]\r\n"
        "command = click,event:ShowDiscordQRCodeWindow\r\n"
        "command = rclick,event:ShowDiscordQRCodeWindow\r\n"
        "command = activate,event:ShowDiscordQRCodeWindow\r\n"
        "command = deactivate,event:HideQRCodeWindow\r\n"
        "command = leftstep,select:BTN_NEWGAME\r\n"
        "command = rightstep,select:BTN_VK\r\n"
        "position = 640,0,670,30\r\n"
        "group = MENU_BUTTONS\r\n"
        "buttonMiddle = Discord_unselect\r\n"
        "selectButtonMiddle = Discord_select\r\n"
        "\r\n"
        "[BTN_VK]\r\n"
        "command = click,event:ShowVKQRCodeWindow\r\n"
        "command = rclick,event:ShowVKQRCodeWindow\r\n"
        "command = activate,event:ShowVKQRCodeWindow\r\n"
        "command = deactivate,event:HideQRCodeWindow\r\n"
        "command = leftstep,select:BTN_DISCORD\r\n"
        "command = rightstep,select:BTN_NEWGAME\r\n"
        "position = 670,0,700,30\r\n"
        "group = MENU_BUTTONS\r\n"
        "buttonMiddle = VK_unselect\r\n"
        "selectButtonMiddle = VK_select\r\n"
        "\r\n"
    )
    text = text[:start_banner] + old_social_chunk + text[changes_win:]

    # Restore QR window at bottom
    old_qr_window = (
        "[QR_WINDOW]\r\n"
        "show = 0\r\n"
        "nodelist = QR_ZONE,QR_DISCORD,QR_VK\r\n"
        "\r\n"
        "[QR_ZONE]\r\n"
        "position = 643,58,740,155\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = QRZone\r\n"
        "\r\n"
        "[QR_DISCORD]\r\n"
        "command = click,event:HideQRCodeWindow\r\n"
        "command = rclick,event:HideQRCodeWindow\r\n"
        "command = activate,event:HideQRCodeWindow\r\n"
        "command = deactivate,event:HideQRCodeWindow\r\n"
        "position = 643,58,740,155\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = QR_Discord\r\n"
        "\r\n"
        "[QR_VK]\r\n"
        "command = click,event:HideQRCodeWindow\r\n"
        "command = rclick,event:HideQRCodeWindow\r\n"
        "command = activate,event:HideQRCodeWindow\r\n"
        "command = deactivate,event:HideQRCodeWindow\r\n"
        "position = 643,58,740,155\r\n"
        "groupName = MENU_BUTTONS\r\n"
        "picName = QR_VK\r\n"
    )
    text = text + old_qr_window
    return text.encode("utf-8")


def _transform_c(data: bytes) -> bytes:
    text = data.decode("utf-8")

    # 1. InitInterface
    old_init = (
        '\tSetFormatedText("VERSION", VERSION_NUMBER1 + GetVerNum());\r\n'
        '\t\r\n'
        '\tSetFormatedText("SUBSCRIBE", XI_ConvertString("Subscribe"));\r\n'
        '\t\r\n'
        '\tSetEventHandler("NewGamePress","NewGamePress",0);\r\n'
        '\tSetEventHandler("LoadPress","LoadPress",0);\r\n'
        '\tSetEventHandler("OptionsPress","OptionsPress",0);\r\n'
        '\tSetEventHandler("CreditsPress","CreditsPress",0);\r\n'
        '\tSetEventHandler("QuitPress","QuitPress",0);\r\n'
        '\t\r\n'
        '\tSetEventHandler("ShowChangesWindow","ShowChangesWindow",0);\r\n'
        '\tSetEventHandler("HideChangesWindow","HideChangesWindow",0);\r\n'
        '\tSetEventHandler("ShowDiscordQRCodeWindow","ShowDiscordQRCodeWindow",0);\r\n'
        '\tSetEventHandler("ShowVKQRCodeWindow","ShowVKQRCodeWindow",0);\r\n'
        '\tSetEventHandler("HideQRCodeWindow","HideQRCodeWindow",0);\r\n'
        '\tSetEventHandler("VolumeFader","VolumeFadeIn",0);\r\n'
    )
    new_init = (
        '\tSetFormatedText("VERSION", "IDDICTIVE REMASTER · GPK 1.3.2 AT + ReConstruction 1.4.1");\r\n'
        '\t\r\n'
        '\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_TELEGRAM", 0, "#Telegram");\r\n'
        '\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_BEHANCE", 0, "#Behance");\r\n'
        '\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_BANNER", 0, "#IDDICTIVE REMASTER · macOS Edition");\r\n'
        '\t\r\n'
        '\tSetEventHandler("NewGamePress","NewGamePress",0);\r\n'
        '\tSetEventHandler("LoadPress","LoadPress",0);\r\n'
        '\tSetEventHandler("OptionsPress","OptionsPress",0);\r\n'
        '\tSetEventHandler("CreditsPress","CreditsPress",0);\r\n'
        '\tSetEventHandler("QuitPress","QuitPress",0);\r\n'
        '\t\r\n'
        '\tSetEventHandler("ShowChangesWindow","ShowChangesWindow",0);\r\n'
        '\tSetEventHandler("HideChangesWindow","HideChangesWindow",0);\r\n'
        '\tSetEventHandler("OpenTelegramURL","OpenTelegramURL",0);\r\n'
        '\tSetEventHandler("OpenBehanceURL","OpenBehanceURL",0);\r\n'
        '\tSetEventHandler("OpenBannerURL","OpenBannerURL",0);\r\n'
        '\tSetEventHandler("VolumeFader","VolumeFadeIn",0);\r\n'
    )
    if old_init not in text:
        raise ValueError("missing old init in mainmenu.c")
    text = text.replace(old_init, new_init, 1)

    # 2. IDoExit
    old_exit = (
        '\tDelEventHandler("ShowDiscordQRCodeWindow","ShowDiscordQRCodeWindow");\r\n'
        '\tDelEventHandler("ShowVKQRCodeWindow","ShowVKQRCodeWindow");\r\n'
        '\tDelEventHandler("HideQRCodeWindow","HideQRCodeWindow");\r\n'
    )
    new_exit = (
        '\tDelEventHandler("OpenTelegramURL","OpenTelegramURL");\r\n'
        '\tDelEventHandler("OpenBehanceURL","OpenBehanceURL");\r\n'
        '\tDelEventHandler("OpenBannerURL","OpenBannerURL");\r\n'
    )
    if old_exit not in text:
        raise ValueError("missing old exit in mainmenu.c")
    text = text.replace(old_exit, new_exit, 1)

    # 3. ShowChangesWindow
    old_changes = (
        '\tXI_WindowShow("CHANGES_WINDOW", true);\r\n'
        '\tXI_WindowDisable("CHANGES_WINDOW", false);\r\n'
        '\tHideQRCodeWindow();\r\n'
    )
    new_changes = (
        '\tXI_WindowShow("CHANGES_WINDOW", true);\r\n'
        '\tXI_WindowDisable("CHANGES_WINDOW", false);\r\n'
    )
    if old_changes not in text:
        raise ValueError("missing old ShowChangesWindow in mainmenu.c")
    text = text.replace(old_changes, new_changes, 1)

    # 4. QR handlers
    old_handlers = (
        'void ShowDiscordQRCodeWindow()\r\n'
        '{\r\n'
        '\tHideChangesWindow();\r\n'
        '\t\r\n'
        '\tXI_WindowShow("QR_WINDOW", true);\r\n'
        '\tXI_WindowDisable("QR_WINDOW", false);\r\n'
        '\t\r\n'
        '\tSetNodeUsing("QR_DISCORD", true);\r\n'
        '\tSetNodeUsing("QR_VK", false);\r\n'
        '}\r\n'
        '\r\n'
        'void ShowVKQRCodeWindow()\r\n'
        '{\r\n'
        '\tHideChangesWindow();\r\n'
        '\t\r\n'
        '\tXI_WindowShow("QR_WINDOW", true);\r\n'
        '\tXI_WindowDisable("QR_WINDOW", false);\r\n'
        '\t\r\n'
        '\tSetNodeUsing("QR_VK", true);\r\n'
        '\tSetNodeUsing("QR_DISCORD", false);\r\n'
        '}\r\n'
        '\r\n'
        'void HideQRCodeWindow()\r\n'
        '{\r\n'
        '\tXI_WindowShow("QR_WINDOW", false);\r\n'
        '\tXI_WindowDisable("QR_WINDOW", true);\r\n'
        '}\r\n'
    )
    new_handlers = (
        'void OpenTelegramURL()\r\n'
        '{\r\n'
        '\tOpenExternalURL("https://t.me/iddictive");\r\n'
        '}\r\n'
        '\r\n'
        'void OpenBehanceURL()\r\n'
        '{\r\n'
        '\tOpenExternalURL("https://www.behance.net/drozdovmn7123");\r\n'
        '}\r\n'
        '\r\n'
        'void OpenBannerURL()\r\n'
        '{\r\n'
        '\tOpenExternalURL("https://iddictive.us/cases/corsairs-native-metal/");\r\n'
        '}\r\n'
    )
    if old_handlers not in text:
        raise ValueError("missing old QR handlers in mainmenu.c")
    text = text.replace(old_handlers, new_handlers, 1)

    return text.encode("utf-8")


def _strip_c(data: bytes) -> bytes:
    text = data.decode("utf-8")

    # 1. InitInterface
    old_init = (
        '\tSetFormatedText("VERSION", VERSION_NUMBER1 + GetVerNum());\r\n'
        '\t\r\n'
        '\tSetFormatedText("SUBSCRIBE", XI_ConvertString("Subscribe"));\r\n'
        '\t\r\n'
        '\tSetEventHandler("NewGamePress","NewGamePress",0);\r\n'
        '\tSetEventHandler("LoadPress","LoadPress",0);\r\n'
        '\tSetEventHandler("OptionsPress","OptionsPress",0);\r\n'
        '\tSetEventHandler("CreditsPress","CreditsPress",0);\r\n'
        '\tSetEventHandler("QuitPress","QuitPress",0);\r\n'
        '\t\r\n'
        '\tSetEventHandler("ShowChangesWindow","ShowChangesWindow",0);\r\n'
        '\tSetEventHandler("HideChangesWindow","HideChangesWindow",0);\r\n'
        '\tSetEventHandler("ShowDiscordQRCodeWindow","ShowDiscordQRCodeWindow",0);\r\n'
        '\tSetEventHandler("ShowVKQRCodeWindow","ShowVKQRCodeWindow",0);\r\n'
        '\tSetEventHandler("HideQRCodeWindow","HideQRCodeWindow",0);\r\n'
        '\tSetEventHandler("VolumeFader","VolumeFadeIn",0);\r\n'
    )
    new_init = (
        '\tSetFormatedText("VERSION", "IDDICTIVE REMASTER · GPK 1.3.2 AT + ReConstruction 1.4.1");\r\n'
        '\t\r\n'
        '\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_TELEGRAM", 0, "#Telegram");\r\n'
        '\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_BEHANCE", 0, "#Behance");\r\n'
        '\tSendMessage(&GameInterface, "lsls", MSG_INTERFACE_MSG_TO_NODE, "BTN_BANNER", 0, "#IDDICTIVE REMASTER · macOS Edition");\r\n'
        '\t\r\n'
        '\tSetEventHandler("NewGamePress","NewGamePress",0);\r\n'
        '\tSetEventHandler("LoadPress","LoadPress",0);\r\n'
        '\tSetEventHandler("OptionsPress","OptionsPress",0);\r\n'
        '\tSetEventHandler("CreditsPress","CreditsPress",0);\r\n'
        '\tSetEventHandler("QuitPress","QuitPress",0);\r\n'
        '\t\r\n'
        '\tSetEventHandler("ShowChangesWindow","ShowChangesWindow",0);\r\n'
        '\tSetEventHandler("HideChangesWindow","HideChangesWindow",0);\r\n'
        '\tSetEventHandler("OpenTelegramURL","OpenTelegramURL",0);\r\n'
        '\tSetEventHandler("OpenBehanceURL","OpenBehanceURL",0);\r\n'
        '\tSetEventHandler("OpenBannerURL","OpenBannerURL",0);\r\n'
        '\tSetEventHandler("VolumeFader","VolumeFadeIn",0);\r\n'
    )
    if new_init not in text:
        raise ValueError("missing new init in mainmenu.c")
    text = text.replace(new_init, old_init, 1)

    # 2. IDoExit
    old_exit = (
        '\tDelEventHandler("ShowDiscordQRCodeWindow","ShowDiscordQRCodeWindow");\r\n'
        '\tDelEventHandler("ShowVKQRCodeWindow","ShowVKQRCodeWindow");\r\n'
        '\tDelEventHandler("HideQRCodeWindow","HideQRCodeWindow");\r\n'
    )
    new_exit = (
        '\tDelEventHandler("OpenTelegramURL","OpenTelegramURL");\r\n'
        '\tDelEventHandler("OpenBehanceURL","OpenBehanceURL");\r\n'
        '\tDelEventHandler("OpenBannerURL","OpenBannerURL");\r\n'
    )
    if new_exit not in text:
        raise ValueError("missing new exit in mainmenu.c")
    text = text.replace(new_exit, old_exit, 1)

    # 3. ShowChangesWindow
    old_changes = (
        '\tXI_WindowShow("CHANGES_WINDOW", true);\r\n'
        '\tXI_WindowDisable("CHANGES_WINDOW", false);\r\n'
        '\tHideQRCodeWindow();\r\n'
    )
    new_changes = (
        '\tXI_WindowShow("CHANGES_WINDOW", true);\r\n'
        '\tXI_WindowDisable("CHANGES_WINDOW", false);\r\n'
    )
    if new_changes not in text:
        raise ValueError("missing new ShowChangesWindow in mainmenu.c")
    text = text.replace(new_changes, old_changes, 1)

    # 4. QR handlers
    old_handlers = (
        'void ShowDiscordQRCodeWindow()\r\n'
        '{\r\n'
        '\tHideChangesWindow();\r\n'
        '\t\r\n'
        '\tXI_WindowShow("QR_WINDOW", true);\r\n'
        '\tXI_WindowDisable("QR_WINDOW", false);\r\n'
        '\t\r\n'
        '\tSetNodeUsing("QR_DISCORD", true);\r\n'
        '\tSetNodeUsing("QR_VK", false);\r\n'
        '}\r\n'
        '\r\n'
        'void ShowVKQRCodeWindow()\r\n'
        '{\r\n'
        '\tHideChangesWindow();\r\n'
        '\t\r\n'
        '\tXI_WindowShow("QR_WINDOW", true);\r\n'
        '\tXI_WindowDisable("QR_WINDOW", false);\r\n'
        '\t\r\n'
        '\tSetNodeUsing("QR_VK", true);\r\n'
        '\tSetNodeUsing("QR_DISCORD", false);\r\n'
        '}\r\n'
        '\r\n'
        'void HideQRCodeWindow()\r\n'
        '{\r\n'
        '\tXI_WindowShow("QR_WINDOW", false);\r\n'
        '\tXI_WindowDisable("QR_WINDOW", true);\r\n'
        '}\r\n'
    )
    new_handlers = (
        'void OpenTelegramURL()\r\n'
        '{\r\n'
        '\tOpenExternalURL("https://t.me/iddictive");\r\n'
        '}\r\n'
        '\r\n'
        'void OpenBehanceURL()\r\n'
        '{\r\n'
        '\tOpenExternalURL("https://www.behance.net/drozdovmn7123");\r\n'
        '}\r\n'
        '\r\n'
        'void OpenBannerURL()\r\n'
        '{\r\n'
        '\tOpenExternalURL("https://iddictive.us/cases/corsairs-native-metal/");\r\n'
        '}\r\n'
    )
    if new_handlers not in text:
        raise ValueError("missing new URL handlers in mainmenu.c")
    text = text.replace(new_handlers, old_handlers, 1)

    return text.encode("utf-8")


def strip(relative: str, data: bytes) -> tuple[bytes, str]:
    if relative not in BASE:
        raise ValueError(f"unsupported menu branding target: {relative}")
    if relative == PICTURES and digest(data) == "2805c35bf9d4a750fb399f0fc53e22df41ad1625716e7f4a09bb0811daf7ee63":
        data = data.replace(b"interfaces\\iddictive_menu_logo.tga", b"iddictive_menu_logo.tga")
    if relative == LAYOUT and digest(data) == "172a7ea274f5a5c52163574bd61fc70e291b252206b8e9480b0099291803574b":
        data = data.replace("string = #IDDICTIVE REMASTER · macOS Edition\r\nfontScale = 0.65\r\n".encode(), "string = #IDDICTIVE REMASTER · macOS Edition\r\nfont = INTERFACE_NORMAL\r\nfontScale = 0.9\r\nstrOffset = 7\r\n".encode()).replace(b"fontScale = 0.75\r\n", b"fontScale = 0.75\r\nstrOffset = 7\r\n")
    if relative == SCRIPT and digest(data) == "6ff003eb1a19078078a8fbb459f38f63f72d12b4db2d48e8116b6718c946f4bb":
        data = data.replace('\tSetFormatedText("VERSION", VERSION_NUMBER1 + GetVerNum());\r\n'.encode(), '\tSetFormatedText("VERSION", "IDDICTIVE REMASTER · GPK 1.3.2 AT + ReConstruction 1.4.1");\r\n'.encode())
    d = digest(data)
    if d == BASE[relative]:
        return data, "base"
    if d == UPDATED[relative]:
        if relative == SCRIPT:
            res = _strip_c(data)
        elif relative == LAYOUT:
            res = _strip_ini(data)
        elif relative == PICTURES:
            res = _strip_pictures(data)
        else:
            raise ValueError(f"unknown handler for {relative}")
        if digest(res) != BASE[relative]:
            raise ValueError(f"stripped bytes hash mismatch for {relative}")
        return res, "base"
    raise ValueError(f"unrecognized menu branding revision: {relative} (sha256={d})")


def prepare(relative: str, data: bytes) -> tuple[bytes, str]:
    if relative not in BASE:
        raise ValueError(f"unsupported menu branding target: {relative}")
    if relative == PICTURES and digest(data) == "2805c35bf9d4a750fb399f0fc53e22df41ad1625716e7f4a09bb0811daf7ee63":
        data = data.replace(b"interfaces\\iddictive_menu_logo.tga", b"iddictive_menu_logo.tga")
    if relative == LAYOUT and digest(data) == "172a7ea274f5a5c52163574bd61fc70e291b252206b8e9480b0099291803574b":
        data = data.replace("string = #IDDICTIVE REMASTER · macOS Edition\r\nfontScale = 0.65\r\n".encode(), "string = #IDDICTIVE REMASTER · macOS Edition\r\nfont = INTERFACE_NORMAL\r\nfontScale = 0.9\r\nstrOffset = 7\r\n".encode()).replace(b"fontScale = 0.75\r\n", b"fontScale = 0.75\r\nstrOffset = 7\r\n")
    if relative == SCRIPT and digest(data) == "6ff003eb1a19078078a8fbb459f38f63f72d12b4db2d48e8116b6718c946f4bb":
        data = data.replace('\tSetFormatedText("VERSION", VERSION_NUMBER1 + GetVerNum());\r\n'.encode(), '\tSetFormatedText("VERSION", "IDDICTIVE REMASTER · GPK 1.3.2 AT + ReConstruction 1.4.1");\r\n'.encode())
    d = digest(data)
    if d == UPDATED[relative]:
        return data, "patched"
    if d == BASE[relative]:
        if relative == SCRIPT:
            res = _transform_c(data)
        elif relative == LAYOUT:
            res = _transform_ini(data)
        elif relative == PICTURES:
            res = _transform_pictures(data)
        else:
            raise ValueError(f"unknown handler for {relative}")
        if digest(res) != UPDATED[relative]:
            raise ValueError(f"prepared bytes hash mismatch for {relative}")
        return res, "patched"
    raise ValueError(f"unrecognized menu branding source revision: {relative} (sha256={d})")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["check", "apply", "strip"], nargs="?", default="check")
    parser.add_argument("--runtime", type=Path, default=DEFAULT_RUNTIME)
    args = parser.parse_args()

    runtime = args.runtime.resolve()
    for rel in FILES:
        target = runtime / rel
        if not target.is_file():
            raise SystemExit(f"Missing file: {target}")
        data = target.read_bytes()
        d = digest(data)
        if args.command == "check":
            status = "patched" if d == UPDATED[rel] else ("base" if d == BASE[rel] else "unknown")
            print(f"{rel}: {status} ({d[:8]}...)")
        elif args.command == "apply":
            patched, state = prepare(rel, data)
            if state != "patched" or patched != data:
                target.write_bytes(patched)
                print(f"Applied branding: {rel}")
            else:
                print(f"Already branded: {rel}")
        elif args.command == "strip":
            stripped, state = strip(rel, data)
            if state != "base" or stripped != data:
                target.write_bytes(stripped)
                print(f"Stripped branding: {rel}")
            else:
                print(f"Already base: {rel}")


if __name__ == "__main__":
    main()
