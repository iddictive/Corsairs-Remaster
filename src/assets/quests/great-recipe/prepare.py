#!/usr/bin/env python3
"""Build additive native item locators over an unchanged stock dummy mesh."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LABEL = struct.Struct("<3i16f4i4f")


def prepare(template, points):
    head = list(struct.unpack_from("<11i7f", template))
    size, names, textures, materials, lights, labels = head[2:8]
    strings = bytearray(template[72:72 + size])
    offsets = list(struct.unpack_from(f"<{names}i", template, 72 + size))
    metadata_start = 72 + size + names * 4
    labels_start = metadata_start + textures * 4 + materials * 56 + lights * 80
    assert labels_start + labels * LABEL.size <= len(template)

    def add_name(value):
        offset = len(strings)
        strings.extend(value.encode("ascii") + b"\0")
        offsets.append(offset)
        return offset

    group = add_name("item")
    records = []
    for name, matrix in points.items():
        assert len(matrix) == 16
        records.append(LABEL.pack(group, add_name(name), 0, *matrix,
                                  0, 0, 0, 0, 0.0, 0.0, 0.0, 0.0))
    head[2], head[3], head[7] = len(strings), len(offsets), len(records)
    return (struct.pack("<11i7f", *head) + strings +
            struct.pack(f"<{len(offsets)}i", *offsets) +
            template[metadata_start:labels_start] + b"".join(records) +
            template[labels_start + labels * LABEL.size:])


def main():
    layout = json.loads((ROOT / "layout.json").read_text())
    template = (ROOT / "hold_l.original.gm").read_bytes()
    assert hashlib.sha256(template).hexdigest() == layout["template_sha256"]
    files = {}
    for room, spec in layout["rooms"].items():
        data = prepare(template, spec["points"])
        destination = ROOT / (room + "_items.gm")
        destination.write_bytes(data)
        files[spec["target"]] = {
            "source": "hold_l.original.gm",
            "source_sha256": layout["template_sha256"],
            "prepared": destination.name,
            "sha256": hashlib.sha256(data).hexdigest(),
            "consumer": "GreatRecipe" + room.title() + ".models.always.greatRecipeItems",
        }
    (ROOT / "manifest.json").write_text(json.dumps({"version": 1, "files": files}, indent=2) + "\n")


if __name__ == "__main__":
    main()
