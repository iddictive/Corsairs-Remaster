#!/usr/bin/env python3
"""Verify prepared jungle texture contracts without writing any files."""
from pathlib import Path
import hashlib, importlib.util, json, struct

ROOT=Path(__file__).resolve().parent
MATERIALS=ROOT.parent
RUNTIME=MATERIALS.parent/".cache/runtime/RESOURCE/Textures"
spec=importlib.util.spec_from_file_location("codec",MATERIALS/"prepare.py")
codec=importlib.util.module_from_spec(spec); spec.loader.exec_module(codec)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
manifest=json.loads((ROOT/"manifest.json").read_text())
assert len(manifest["records"])==13
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
print("verified",len(manifest["records"]),"prepared jungle textures")
