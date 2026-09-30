#!/usr/bin/env python3
"""Verify prepared jungle texture contracts without writing any files."""
from pathlib import Path
import hashlib, importlib.util, json, struct, sys

ROOT=Path(__file__).resolve().parent
MATERIALS=ROOT.parent
RUNTIME=MATERIALS.parent.parent/"native-storm/.cache/runtime/RESOURCE/Textures"
RESOURCE=MATERIALS.parent.parent/"native-storm/.cache/runtime/RESOURCE"
sys.path.insert(0,str(MATERIALS.parent))
from island_geometry import load_static_gm
spec=importlib.util.spec_from_file_location("codec",MATERIALS/"prepare.py")
codec=importlib.util.module_from_spec(spec); spec.loader.exec_module(codec)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
manifest=json.loads((ROOT/"manifest.json").read_text())
assert len(manifest["records"])==13
records={row["target"]:row for row in manifest["records"]}
for row in manifest["records"]:
    target=MATERIALS/row["prepared_path"]
    assert target.is_file() and sha(target)==row["output_sha256"]
    _,w,h,mips,fmt,_=struct.unpack("<6I",target.read_bytes()[:24])
    assert (w,h,mips)==(row["width"],row["height"],row["mip_count"])
    expected={"DXT1":int.from_bytes(b"DXT1","little"),"DXT5":int.from_bytes(b"DXT5","little"),"A8R8G8B8":21,"A4R4G4B4":25}[row["format"]]
    assert fmt==expected
    if row["target"] in ("treePalms.tga.tx","trees.tga.tx","PalmsAlphaLitle01.tga.tx","PalmsSprites.tga.tx"):
        original=RUNTIME/row["target"]
        assert codec.decode_tx(target.read_bytes()).getchannel("A").tobytes()==codec.decode_tx(original.read_bytes()).getchannel("A").tobytes()
    if row["target"]=="leafPalms.tga.tx": assert target.read_bytes()==(RUNTIME/row["target"]).read_bytes()
    source=row.get("source","")
    assert not source.startswith("/"), f"Non-portable source path: {source}"

# Generated raw-alpha foliage preserves the original silhouette at level zero
# and coverage through useful alpha-tested mips. Tiny mips are excluded because
# one texel necessarily changes coverage in coarse steps.
for name in ("treePalms.tga.tx","trees.tga.tx"):
    row=records[name]; assert row["alpha_test"]=={"ref":160,"comparison":"GREATER","source":"native-storm renderer SetCommonStates"}
    base=row["mips"][0]["coverage"]
    for mip in row["mips"]:
        if min(mip["width"],mip["height"])>=8:
            assert mip["alpha_ref"]==160 and mip["comparison"]=="GREATER"
            # One texel is the tightest possible bound on a discrete mip mask.
            tolerance=max(0.002,1/(mip["width"]*mip["height"]))
            assert abs(mip["coverage"]-base)<=tolerance, (name,mip)

# The delivery manifest must name a real authored GM consumer for every asset;
# a location-specific example is evidence only when that GM actually references
# the texture. This prevents a San Juan example from becoming a global rule.
delivery=json.loads((MATERIALS/"staging.json").read_text())
for row in delivery["assets"]:
    model=RESOURCE/row["consumer"]
    assert model.is_file(), model
    scene=load_static_gm(model)
    expected=row["target"].lower().removesuffix(".tx")
    actual={draw["texture"].lower() for draw in scene["draws"]}
    assert expected in actual, f"{row['target']} is not referenced by {row['consumer']}"

print("verified",len(manifest["records"]),"prepared jungle textures and authored GM consumers")
