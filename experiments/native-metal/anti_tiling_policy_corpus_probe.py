#!/usr/bin/env python3
"""Verify the exact-name policy against the staged authored GM corpus."""
from __future__ import annotations

import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
POLICY = ROOT / "anti_tiling_policy.hpp"
MODELS = ROOT / ".cache/runtime/RESOURCE/MODELS"


def gm_textures(path: Path) -> set[str]:
    with path.open("rb") as source:
        header = struct.unpack("<11i7f", source.read(struct.calcsize("<11i7f")))
        string_bytes, name_count, texture_count = header[2], header[3], header[4]
        if min(string_bytes, name_count, texture_count) < 0:
            raise ValueError("invalid GM counts")
        names = source.read(string_bytes)
        source.seek(name_count * 4, 1)
        offsets = struct.unpack(f"<{texture_count}i", source.read(texture_count * 4)) if texture_count else ()
    result = set()
    for offset in offsets:
        if not 0 <= offset < len(names):
            raise ValueError("invalid GM texture offset")
        end = names.find(b"\0", offset)
        if end < 0:
            raise ValueError("unterminated GM texture name")
        result.add(names[offset:end].decode("cp1251", "replace").lower())
    return result


def main() -> None:
    source = POLICY.read_text()
    admitted = set(re.findall(r'"([A-Za-z0-9_]+\.tga)"', source.split("Deliberately absent:")[0]))
    assert admitted == {"floorU3.tga", "Sandtile.tga", "rockA2.tga", "rockB2.tga", "rockC2.tga"}

    watched = {name.lower(): [] for name in admitted | {"cobbleM1.tga", "town_cob.tga", "tileU2.tga"}}
    scanned = 0
    for model in MODELS.rglob("*.gm"):
        try:
            textures = gm_textures(model)
        except (OSError, struct.error, ValueError):
            continue
        scanned += 1
        for name in watched.keys() & textures:
            watched[name].append(model.relative_to(MODELS))
    assert scanned > 1000
    assert len(watched["flooru3.tga"]) >= 10, "floorU3 must remain a real repeated floor"
    assert len(watched["sandtile.tga"]) >= 20, "Sandtile must remain a real terrain material"
    # These are real consumers too; their absence from the policy is intentional.
    assert all(watched[name] for name in ("cobblem1.tga", "town_cob.tga", "tileu2.tga"))
    print(
        "PASS anti-tiling corpus: "
        f"{scanned} GMs; floorU3={len(watched['flooru3.tga'])}; "
        f"Sandtile={len(watched['sandtile.tga'])}; structured paving excluded"
    )


if __name__ == "__main__":
    main()
