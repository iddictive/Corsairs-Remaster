#!/usr/bin/env python3
"""Deterministic read-only inventory for the assigned wood/deck/hull family."""
from __future__ import annotations

import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
STORM = ROOT / "experiments/native-storm/.cache/runtime/RESOURCE"
METAL = ROOT / "experiments/native-metal/.cache/runtime/RESOURCE"
OUT = Path(__file__).resolve().parent
TEXTURE_RE = re.compile(r"(?:wood|deck|hull)", re.I)
FORMAT_NAMES = {21: "A8R8G8B8", 22: "X8R8G8B8", 23: "R5G6B5", 25: "A1R5G5B5", 26: "A4R4G4B4", 827611204: "DXT1", 894720068: "DXT5", 861165636: "DXT3"}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def mip_bytes(width: int, height: int, fmt: int) -> int:
    if fmt in (21, 22):
        return width * height * 4
    if fmt in (23, 25, 26):
        return width * height * 2
    if fmt in (827611204, 861165636, 894720068):
        block = 8 if fmt == 827611204 else 16
        return ((width + 3) // 4) * ((height + 3) // 4) * block
    return -1


def tx_info(path: Path) -> dict:
    raw = path.read_bytes()
    result = {"present": True, "bytes": len(raw), "sha256": sha(raw), "header_valid": False, "mip_chain_valid": False}
    if len(raw) < 24:
        return result
    flags, width, height, mips, fmt, level0_size = struct.unpack_from("<6I", raw)
    result.update({"flags": flags, "width": width, "height": height, "mip_count": mips, "format_code": fmt,
                   "format": FORMAT_NAMES.get(fmt, f"UNKNOWN({fmt})"), "level0_size": level0_size})
    result["header_valid"] = bool(width and height and mips and level0_size == mip_bytes(width, height, fmt) and level0_size >= 0)
    expected = []
    w, h = width, height
    for _ in range(mips):
        size = mip_bytes(w, h, fmt)
        if size < 0:
            break
        expected.append({"width": w, "height": h, "bytes": size})
        w, h = max(1, w // 2), max(1, h // 2)
    payload = sum(x["bytes"] for x in expected)
    # Storm assets commonly stop before 1x1; validity is exact ordered payload
    # sizing for the declared chain, not a requirement to include every tail mip.
    result["mip_chain_valid"] = len(expected) == mips and len(raw) == 24 + payload
    result["mips"] = expected
    result["alpha"] = alpha_class(raw, width, height, fmt, level0_size)
    return result


def alpha_class(raw: bytes, width: int, height: int, fmt: int, size: int) -> dict:
    if fmt in (22, 23) or fmt == 827611204:
        if fmt == 827611204:
            # DXT1 is opaque unless any 4x4 block uses the 1-bit-alpha endpoint ordering.
            payload = raw[24:24 + size]
            transparent_blocks = sum(1 for i in range(0, len(payload), 8) if int.from_bytes(payload[i + 0:i + 2], "little") <= int.from_bytes(payload[i + 2:i + 4], "little"))
            return {"contract": "opaque_or_1bit", "transparent_blocks": transparent_blocks}
        return {"contract": "opaque", "decoded": "not_required_for_native_opaque_format"}
    if fmt == 21:
        vals = raw[24:24 + size][3::4]
        return {"contract": "explicit_alpha", "min": min(vals, default=255), "max": max(vals, default=255), "transparent_pixels": sum(v == 0 for v in vals), "nonopaque_pixels": sum(v < 255 for v in vals)}
    if fmt == 25:
        vals = [255 if v & 0x8000 else 0 for (v,) in struct.iter_unpack("<H", raw[24:24 + size])]
        return {"contract": "explicit_alpha_1bit", "min": min(vals, default=255), "max": max(vals, default=255), "transparent_pixels": sum(v == 0 for v in vals), "nonopaque_pixels": sum(v < 255 for v in vals)}
    if fmt == 26:
        vals = [((v >> 12) & 15) * 17 for (v,) in struct.iter_unpack("<H", raw[24:24 + size])]
        return {"contract": "explicit_alpha_4bit", "min": min(vals, default=255), "max": max(vals, default=255), "transparent_pixels": sum(v == 0 for v in vals), "nonopaque_pixels": sum(v < 255 for v in vals)}
    if fmt in (861165636, 894720068):
        return {"contract": "explicit_alpha_block_compressed", "decoded": "requires_bc_decoder"}
    return {"contract": "unknown"}


def gm_consumers(root: Path, wanted: set[str]) -> dict[str, list[str]]:
    result = {x: [] for x in wanted}
    by_base = {}
    for rel in wanted:
        by_base.setdefault(Path(rel).name.lower().removesuffix(".tx"), []).append(rel)
    for model in sorted((root / "MODELS").rglob("*.gm")):
        data = model.read_bytes()
        if len(data) < struct.calcsize("<11i7f"):
            continue
        try:
            header = struct.unpack_from("<11i7f", data)
            string_bytes, name_count, texture_count = header[2:5]
            if min(string_bytes, name_count, texture_count) < 0:
                continue
            off = struct.calcsize("<11i7f")
            strings = data[off:off + string_bytes]
            off += string_bytes + name_count * 4
            offsets = struct.unpack_from(f"<{texture_count}i", data, off) if texture_count else ()
            names = set()
            for pos in offsets:
                if 0 <= pos < len(strings):
                    end = strings.find(b"\0", pos)
                    if end >= 0:
                        names.add(strings[pos:end].decode("cp1251", "replace").replace("\\", "/").lower())
            for name in names:
                for rel in by_base.get(name, ()):
                    result[rel].append(model.relative_to(root / "MODELS").as_posix())
        except (struct.error, UnicodeError):
            continue
    return result


def fx_consumers(root: Path, wanted: set[str]) -> dict[str, list[str]]:
    result = {x: [] for x in wanted}
    tokens = {x: {Path(x).name.lower(), Path(x).stem.lower()} for x in wanted}
    fx_root = root / "techniques"
    for fx in sorted(fx_root.rglob("*")):
        if not fx.is_file():
            continue
        try:
            text = fx.read_text("cp1251", errors="ignore").lower()
        except OSError:
            continue
        for rel, names in tokens.items():
            if any(re.search(r"(?<![a-z0-9_])" + re.escape(n) + r"(?![a-z0-9_])", text) for n in names):
                result[rel].append(fx.relative_to(fx_root).as_posix())
    return result


def main() -> None:
    storm_root = STORM / "Textures"
    metal_root = METAL / "Textures"
    rels = sorted(p.relative_to(storm_root).as_posix() for p in storm_root.rglob("*.tx") if TEXTURE_RE.search(p.relative_to(storm_root).as_posix()))
    wanted = set(rels)
    gm = gm_consumers(STORM, wanted)
    fx = fx_consumers(STORM, wanted)
    records = []
    for rel in rels:
        sp = storm_root / rel
        mp = metal_root / rel
        storm = tx_info(sp)
        metal = tx_info(mp) if mp.is_file() else {"present": False}
        consumers = sorted(set(gm[rel] + [f"FX/{x}" for x in fx[rel]]))
        low_resolution = max(storm.get("width", 0), storm.get("height", 0)) < 512
        malformed = not storm.get("header_valid", False) or not storm.get("mip_chain_valid", False)
        records.append({"target": rel, "storm": storm, "metal": metal, "same_bytes_in_metal": mp.is_file() and sp.read_bytes() == mp.read_bytes(), "consumers": consumers,
                        "consumer_evidence": "exact GM texture-table basename/path token" if consumers else "none found by exact GM table or FX token scan",
                        "quality": {"resolution_class": "low_resolution_lt_512" if low_resolution else "adequate_resolution_ge_512", "measured_low_quality": low_resolution or malformed, "historical_uv_or_alpha_preservation_required": bool(consumers)},
                        "disposition": "unresolved" if malformed else "kept",
                        "decision_basis": "mip payload length does not match declared chain; no runtime mutation" if malformed else ("low resolution measured, but no semantically safe Poly Haven replacement can be justified without changing authored UV/pattern/history" if low_resolution else "measured header, format, alpha and mip contract valid; preserve original" )})
    manifest = {"schema": 1, "family": "wood/decks/hulls", "selection_predicate": "case-insensitive regex (wood|deck|hull) over canonical Storm relative texture path", "canonical_runtime": str(storm_root), "comparison_runtime": str(metal_root), "gm_root": str(STORM / "MODELS"), "fx_root": str(STORM / "techniques"), "records": records}
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    summary = {"family_rows": len(records), "storm_present": len(records), "metal_missing": sum(not r["metal"].get("present", True) for r in records), "same_bytes": sum(r["same_bytes_in_metal"] for r in records), "header_invalid": sum(not r["storm"].get("header_valid", False) for r in records), "mip_invalid": sum(not r["storm"].get("mip_chain_valid", False) for r in records), "measured_low_resolution_lt_512": sum(r["quality"]["resolution_class"] == "low_resolution_lt_512" for r in records), "low_resolution_with_exact_consumer": sum(r["quality"]["resolution_class"] == "low_resolution_lt_512" and bool(r["consumers"]) for r in records), "with_gm_consumer": sum(any(not x.startswith("FX/") for x in r["consumers"]) for r in records), "with_fx_consumer": sum(any(x.startswith("FX/") for x in r["consumers"]) for r in records), "no_discoverable_consumer": sum(not r["consumers"] for r in records), "kept": sum(r["disposition"] == "kept" for r in records), "replaced": 0, "rejected": 0, "unresolved": sum(r["disposition"] == "unresolved" for r in records)}
    (OUT / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
