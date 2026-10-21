"""Reviewed Metal-only pickup markers; no direct runtime writes.

The canonical gameplay sync owns delivery. Keep the legacy CP1251/CRLF bytes.
"""

import hashlib

RELATIVE_PATH = "PROGRAM/ITEMS/itemLogic.c"
ORIGINAL_SHA256 = "aab9f649883ae523289c865b3cde41afaff76c87c072acb9a5f15a8259651c03"

# Each site is a gameplay role, not a model/texture name. The shared loader is
# also used for mechanism buttons and already-placed quest items: default off.
INSERTIONS = (
    (b"void Items_LoadModel (ref _itemModel, ref _item)\r\n{\r\n",
     b"\t_itemModel.metalPickup = false;\r\n"),
    (b"\t{ //unused\r\n\t\tItems_LoadModel(&itemModels[_itemN], &Items[_itemN]);\r\n",
     b"\t\titemModels[_itemN].metalPickup = true;\r\n"),
    (b"\tItems_LoadModel(&randItemModels[_index],  randItem);\r\n",
     b"\trandItemModels[_index].metalPickup = true;\r\n"),
    (b"\tItems_LoadModel(&randItemModels[_index],  &Items[n]);\r\n",
     b"\trandItemModels[_index].metalPickup = true;\r\n"),
    (b"\t\tif (CheckAttribute(activeLocation, activeRandItemAttribute))\r\n\t\t{\r\n",
     b"\t\t\trandItemModels[sti(chr.activeItem)].metalPickup = false;\r\n"),
    (b"\t\tItems[activeItem].shown = false;\r\n",
     b"\t\titemModels[activeItem].metalPickup = false;\r\n"),
)


def strip(data: bytes) -> bytes:
    if hashlib.sha256(data).hexdigest() == ORIGINAL_SHA256:
        return data
    original = data
    for anchor, addition in INSERTIONS:
        combined = anchor + addition
        if original.count(combined) != 1:
            raise ValueError("unrecognized Metal pickup marker revision")
        original = original.replace(combined, anchor, 1)
    if hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
        raise ValueError("unreviewed itemLogic.c changes")
    return original


def prepare(data: bytes) -> bytes:
    result = strip(data)
    for anchor, addition in INSERTIONS:
        if result.count(anchor) != 1:
            raise ValueError("pickup marker anchor is not unique")
        result = result.replace(anchor, anchor + addition, 1)
    return result
