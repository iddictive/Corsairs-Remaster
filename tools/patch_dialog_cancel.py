#!/usr/bin/env python3
"""Esc uses the dialogue's own harmless exit option when the current line offers one."""

from __future__ import annotations

from runtime_script_patch import FilePatch


PATCH = FilePatch(
    "PROGRAM/dialog.c",
    "948e47d62c451f204e9e7bfe41bcc6d235255d2438a3b06ef3ab758170361ee8",
    "75cf22a1481414f20629afd922ac4afebc2621ddd8b1852918a8003908b7635d",
    (
        # The tab-padded revision in SelfDialog is replaced first so the plain
        # block below stays unique.
        (
            '\t//SetEventHandler("DialogCancel","DialogExit",0);\t\n',
            '\tSetEventHandler("DialogCancel","DialogCancel_Exit",0);\n',
        ),
        (
            '\t//SetEventHandler("DialogCancel","DialogExit",0);\n',
            '\tSetEventHandler("DialogCancel","DialogCancel_Exit",0);\n',
        ),
        (
            '//\u041d\u0430\u0447\u0430\u0442\u044c \u0434\u0438\u0430\u043b\u043e\u0433\nbool DialogMain(ref Character)\n',
            '//Esc \u0432\u044b\u0445\u043e\u0434 \u0431\u0435\u0437 \u043f\u043e\u0441\u043b\u0435\u0434\u0441\u0442\u0432\u0438\u0439\n'
            'string DialogCancel_FreeExitNode()\n'
            '{\n'
            '\tif(!CheckAttribute(&Dialog, "Links")) return "";\n'
            '\taref Link;\n'
            '\tmakearef(Link, Dialog.Links);\n'
            '\tint num = GetAttributesNum(Link);\n'
            '\tfor(int i = 0; i < num; i++)\n'
            '\t{\n'
            '\t\taref lnk = GetAttributeN(Link, i);\n'
            '\t\tif(!CheckAttribute(lnk, "go")) continue;\n'
            '\t\tstring go = lnk.go;\n'
            '\t\tif(go == "exit" || go == "Exit") return go;\n'
            '\t}\n'
            '\treturn "";\n'
            '}\n'
            '\n'
            'void DialogCancel_Exit()\n'
            '{\n'
            '\tif(!dialogRun) return;\n'
            '\tstring node = DialogCancel_FreeExitNode();\n'
            '\tif(node == "") return;\n'
            '\tDialog.CurrentNode = node;\n'
            '\tEvent("DialogEvent");\n'
            '}\n'
            '\n'
            '//\u041d\u0430\u0447\u0430\u0442\u044c \u0434\u0438\u0430\u043b\u043e\u0433\nbool DialogMain(ref Character)\n',
        ),
    ),
)


def verify(source: str) -> None:
    """Reject a composed dialog.c whose Esc path stopped using the exit link."""
    registered = 'SetEventHandler("DialogCancel","DialogCancel_Exit",0);'
    if source.count(registered) != 2:
        raise RuntimeError("Esc must be registered in both dialogue entry points")
    if 'SetEventHandler("DialogCancel","DialogExit",0)' in source:
        raise RuntimeError("Esc must not exit a dialogue unconditionally")
    start = source.index("void DialogCancel_Exit()")
    block = source[start:source.index("//\u041d\u0430\u0447\u0430\u0442\u044c \u0434\u0438\u0430\u043b\u043e\u0433", start)]
    if 'if(go == "exit" || go == "Exit") return go;' not in source:
        raise RuntimeError("Esc must look for the dialogue's own exit link")
    for token in ('if(!dialogRun) return;', 'Dialog.CurrentNode = node;', 'Event("DialogEvent");'):
        if token not in block:
            raise RuntimeError(f"Esc handler token is missing: {token}")
    if "DialogExit();" in block:
        raise RuntimeError("Esc must run the dialogue's exit node, not exit on its own")
