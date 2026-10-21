#!/usr/bin/env python3
"""Read-only, deterministic inventory for the vegetation texture family."""
from __future__ import annotations

import hashlib
import json
import re
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
STORM = ROOT / "experiments/native-storm/.cache/runtime/RESOURCE"
METAL = ROOT / "experiments/native-metal/.cache/runtime/RESOURCE"
OUT = Path(__file__).resolve().parent

# Semantic family admission is filename/path evidence only; lighting shadows,
# UI icons, and wood materials are separate families and are excluded.
TOKENS = ("grass", "fern", "palm", "plant", "tree", "jungle", "leaf", "flower", "weed", "veget", "bush", "reed", "algae")
EXCLUDE = ("/lighting/", "icon", "wood")

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def family_paths(resource: Path) -> list[str]:
    out = []
    for path in resource.joinpath("Textures").rglob("*.tx"):
        rel = path.relative_to(resource / "Textures").as_posix()
        low = rel.lower()
        if any(token in low for token in TOKENS) and not any(token in low for token in EXCLUDE) and not low.startswith("lighting/"):
            out.append(rel)
    return sorted(out, key=str.casefold)

def parse_tx(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) < 24:
        return {"error": "truncated header", "bytes": len(data), "sha256": sha(data)}
    flags, width, height, mips, fmt, level0_size = struct.unpack("<6I", data[:24])
    format_name = {21: "A8R8G8B8", 23: "R5G6B5", 25: "A4R4G4B4", int.from_bytes(b"DXT1", "little"): "DXT1", int.from_bytes(b"DXT3", "little"): "DXT3", int.from_bytes(b"DXT5", "little"): "DXT5"}.get(fmt, f"0x{fmt:08x}")
    def level_size_for(w: int, h: int) -> int:
        if format_name.startswith("DXT"):
            return ((w + 3) // 4) * ((h + 3) // 4) * (8 if format_name == "DXT1" else 16)
        return w * h * ({"A8R8G8B8": 4, "R5G6B5": 2, "A4R4G4B4": 2}.get(format_name, 0))
    sizes = []
    w, h = width, height
    for _ in range(mips):
        sizes.append(level_size_for(w, h))
        w, h = max(1, w // 2), max(1, h // 2)
    payload = data[24:]
    chain_valid = bool(mips) and all(size > 0 for size in sizes) and len(payload) == sum(sizes) and level0_size == sizes[0]
    alpha = {"behavior": "unknown"}
    try:
        raw = payload[:sizes[0]]
        if format_name == "A8R8G8B8":
            im = Image.frombytes("RGBA", (width, height), raw, "raw", "BGRA")
        elif format_name == "A4R4G4B4":
            vals = struct.iter_unpack("<H", raw)
            rgba = bytes(c for (v,) in vals for c in (((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17, ((v >> 12) & 15) * 17))
            im = Image.frombytes("RGBA", (width, height), rgba)
        elif format_name == "R5G6B5":
            vals = struct.iter_unpack("<H", raw)
            rgba = bytes(c for (v,) in vals for c in (((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31, 255))
            im = Image.frombytes("RGBA", (width, height), rgba)
        elif format_name.startswith("DXT"):
            # Alpha-only block probe avoids expanding multi-megapixel BCn images.
            a = []
            for off in range(0, len(raw), 8 if format_name == "DXT1" else 16):
                block = raw[off:off + (8 if format_name == "DXT1" else 16)]
                if format_name == "DXT1":
                    c0, c1 = struct.unpack_from("<HH", block)
                    selectors = int.from_bytes(block[4:8], "little")
                    for i in range(16):
                        a.append(0 if c0 <= c1 and ((selectors >> (2 * i)) & 3) == 3 else 255)
                elif format_name == "DXT3":
                    bits = int.from_bytes(block[:8], "little")
                    a.extend(((bits >> (4 * i)) & 15) * 17 for i in range(16))
                else:
                    a0, a1 = block[:2]
                    table = [a0, a1] + ([round(a0 + (a1 - a0) * i / 7) for i in range(1, 7)] if a0 > a1 else [round(a0 + (a1 - a0) * i / 5) for i in range(1, 5)] + [0, 255])
                    bits = int.from_bytes(block[2:8], "little")
                    a.extend(table[(bits >> (3 * i)) & 7] for i in range(16))
        a = [v[3] for v in im.getdata()] if "im" in locals() else a
        counts = {"min": min(a), "max": max(a), "zero": sum(x == 0 for x in a), "partial": sum(0 < x < 255 for x in a), "opaque": sum(x == 255 for x in a), "pixels": len(a)}
        alpha = {"behavior": "opaque" if counts["min"] == 255 else ("binary-cutout" if counts["partial"] == 0 else "translucent-or-compressed"), **counts}
    except Exception as exc:
        alpha = {"behavior": "decode-error", "error": str(exc)}
    return {"bytes": len(data), "sha256": sha(data), "flags": flags, "width": width, "height": height, "mip_count": mips, "format": format_name, "level0_size": level0_size, "level_sizes": sizes, "chain_valid": chain_valid, "alpha": alpha}

def parse_gm(path: Path) -> set[str]:
    raw = path.read_bytes()
    if len(raw) < struct.calcsize("<11i7f"):
        return set()
    try:
        header = struct.unpack("<11i7f", raw[:struct.calcsize("<11i7f")])
        string_bytes, name_count, texture_count = header[2:5]
        if min(string_bytes, name_count, texture_count) < 0:
            return set()
        start = struct.calcsize("<11i7f")
        names = raw[start:start + string_bytes]
        offsets_start = start + string_bytes + name_count * 4
        offsets = struct.unpack(f"<{texture_count}i", raw[offsets_start:offsets_start + texture_count * 4]) if texture_count else ()
        out = set()
        for off in offsets:
            if 0 <= off < len(names):
                end = names.find(b"\0", off)
                if end >= 0:
                    out.add(names[off:end].decode("cp1251", "replace").replace("\\", "/").lower())
        return out
    except (struct.error, UnicodeError):
        return set()

def consumers(resource: Path, rels: list[str]) -> dict[str, dict[str, list[str]]]:
    basenames = {rel.lower(): Path(rel).name.lower().removesuffix(".tx") for rel in rels}
    result = {rel: {"gm": [], "fx": []} for rel in rels}
    by_name = {base: [] for base in basenames.values()}
    by_rel = {rel: [] for rel in basenames}
    for gm in resource.joinpath("MODELS").rglob("*.gm"):
        names = parse_gm(gm)
        for name in names & set(by_name):
            by_name[name].append(gm.relative_to(resource / "MODELS").as_posix())
        for rel in names & set(by_rel):
            by_rel[rel].append(gm.relative_to(resource / "MODELS").as_posix())
    for rel, base in basenames.items():
        original = next(key for key in result if key.lower() == rel)
        result[original]["gm"] = sorted(set(by_name[base] + by_rel[rel]))
    for fx in resource.rglob("*.fx"):
        try:
            text = fx.read_text("cp1251", errors="replace").lower().replace("\\", "/")
        except OSError:
            continue
        for rel, base in basenames.items():
            original = next(key for key in result if key.lower() == rel)
            if re.search(r"(?<![a-z0-9_])" + re.escape(base) + r"(?![a-z0-9_])", text) or re.search(r"(?<![a-z0-9_])" + re.escape(rel.lower()) + r"(?![a-z0-9_])", text):
                result[original]["fx"].append(fx.relative_to(resource).as_posix())
    return result

def build() -> dict:
    rels = family_paths(STORM)
    storm_consumers = consumers(STORM, rels)
    metal_consumers = consumers(METAL, rels)
    rows = []
    for rel in rels:
        s = parse_tx(STORM / "Textures" / rel)
        m = parse_tx(METAL / "Textures" / rel)
        rows.append({"target": rel, "storm": s, "metal": m, "identical_runtime_bytes": s["sha256"] == m["sha256"], "storm_consumers": storm_consumers[rel], "metal_consumers": metal_consumers[rel], "consumer_union_count": len(set(storm_consumers[rel]["gm"] + storm_consumers[rel]["fx"] + metal_consumers[rel]["gm"] + metal_consumers[rel]["fx"]))})
    for row in rows:
        low_quality_probe = row["storm"]["width"] <= 512 and row["consumer_union_count"] > 0
        row["disposition"] = "candidate_rejected_keep_original" if low_quality_probe else "kept_original"
        row["disposition_reason"] = (
            "Mapped low-resolution source was measured, but no Poly Haven source preserves the required historical alpha-atlas/sprite contract and character; no candidate prepared."
            if low_quality_probe else
            ("No exact GM/FX consumer discovered; retain source and do not infer a mapping." if row["consumer_union_count"] == 0 else "Complete valid mip chain and no proven replacement case."))
    return {"schema": 1, "family": "vegetation", "source_runtime": str(STORM), "comparison_runtime": str(METAL), "target_paths": {"storm": [str(STORM / "Textures" / r) for r in rels], "metal": [str(METAL / "Textures" / r) for r in rels]}, "selection": {"tokens": TOKENS, "excluded": EXCLUDE, "rule": "path/name token match; lighting shadows, icons and wood excluded"}, "research": [{"asset_id": "leafy_grass", "page_url": "https://polyhaven.com/a/leafy_grass", "download_url_pattern": "https://dl.polyhaven.org/file/ph-assets/Textures/png/{resolution}/leafy_grass/leafy_grass_diff_{resolution}.png", "license_url": "https://polyhaven.com/license", "finding": "CC0 seamless ground material; not a historical alpha foliage atlas/sprite."}, {"asset_id": "grass_medium_01", "page_url": "https://polyhaven.com/a/grass_medium_01", "download_url_pattern": "Poly Haven page download; model asset", "license_url": "https://polyhaven.com/license", "finding": "CC0 3D grass model, not a TX-compatible 2D atlas."}, {"asset_id": "forest_ground_01", "page_url": "https://polyhaven.com/a/forrest_ground_01", "download_url_pattern": "https://dl.polyhaven.org/file/ph-assets/Textures/png/{resolution}/forrest_ground_01/forrest_ground_01_diff_{resolution}.png", "license_url": "https://polyhaven.com/license", "finding": "CC0 forest-floor material; opaque ground semantics do not match alpha sprites."}], "research_conclusion": "No candidate justified; no download, preparation, staging, or runtime mutation.", "rows": rows}

if __name__ == "__main__":
    report = build()
    (OUT / "inventory.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"rows": len(report["rows"]), "gm_refs": sum(len(r["storm_consumers"]["gm"]) for r in report["rows"]), "fx_refs": sum(len(r["storm_consumers"]["fx"]) for r in report["rows"]), "runtime_byte_diffs": sum(not r["identical_runtime_bytes"] for r in report["rows"])}, sort_keys=True))
