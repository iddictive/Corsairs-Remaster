#!/usr/bin/env python3
"""Build source-only jungle materials without touching a runtime."""
from pathlib import Path
import hashlib, importlib.util, io, json, shutil, struct, urllib.request
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageStat, __version__ as pillow_version

ROOT = Path(__file__).resolve().parent
MATERIALS = ROOT.parent
GROUND = MATERIALS / "ground-new"
FOLIAGE = MATERIALS / "foliage-new"
SOURCE_TEXTURES = MATERIALS.parent.parent / "native-storm/.cache/runtime/RESOURCE/Textures"
spec = importlib.util.spec_from_file_location("material_codec", MATERIALS / "prepare.py")
codec = importlib.util.module_from_spec(spec); spec.loader.exec_module(codec)

CC0 = {
    "1grass1": {
        "asset": "sparse_grass", "author": "Amal Kumar",
        "page": "https://polyhaven.com/a/sparse_grass",
        "url": "https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/sparse_grass/sparse_grass_diff_4k.png",
        "sha256": "2d6d0b4a167a402be781f2d6259206479c31c5e17506b8333273e81b20ed7744",
    },
    "grassgreen1": {
        "asset": "leafy_grass", "author": "Charlotte Baglioni",
        "page": "https://polyhaven.com/a/leafy_grass",
        "url": "https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/leafy_grass/leafy_grass_diff_4k.png",
        "sha256": "e03f0425197ac40d5ab636e28e4434c2db33234001b1f8fa04ccc3f05604dc33",
    },
}

def sha(data): return hashlib.sha256(data).hexdigest()

def source_ref(path):
    """Keep manifests portable while retaining the canonical repository owner."""
    return str(path.relative_to(MATERIALS))

def encode_dxt(image, codec_name, mip_count):
    levels=[]; mip=image.convert("RGBA")
    for _ in range(mip_count):
        if codec_name=="DXT1": levels.append(codec.encode_level(mip,codec_name))
        else:
            w,h=mip.size; padded=Image.new("RGBA",(max(4,w),max(4,h))); padded.paste(mip)
            if w<4 or h<4:
                for y in range(padded.height):
                    for x in range(padded.width): padded.putpixel((x,y),mip.getpixel((min(x,w-1),min(y,h-1))))
            stream=io.BytesIO(); padded.save(stream,format="DDS",pixel_format=codec_name)
            payload=stream.getvalue()[128:]
            assert len(payload)==((w+3)//4)*((h+3)//4)*16
            levels.append(payload)
        mip=mip.resize((max(1,mip.width//2),max(1,mip.height//2)), Image.Resampling.LANCZOS)
    fmt=int.from_bytes(codec_name.encode(), "little")
    return struct.pack("<6I",0,image.width,image.height,mip_count,fmt,len(levels[0]))+b"".join(levels)

def encode_raw_argb(image, mip_count):
    levels=[]; mip=image.convert("RGBA"); base=sum(mip.getchannel("A").histogram()[128:])/(mip.width*mip.height)
    for _ in range(mip_count):
        levels.append(mip.tobytes("raw","BGRA"))
        if mip.size==(1,1): break
        size=(max(1,mip.width//2),max(1,mip.height//2))
        mip=mip.convert("RGBa").resize(size,Image.Resampling.LANCZOS).convert("RGBA")
        if min(size)>=8:
            alpha=mip.getchannel("A"); lo,hi=0.0,8.0
            for _ in range(16):
                scale=(lo+hi)/2; candidate=alpha.point(lambda x:min(255,round(x*scale)))
                cov=sum(candidate.histogram()[128:])/(candidate.width*candidate.height)
                if cov<base: lo=scale
                else: hi=scale
            mip.putalpha(alpha.point(lambda x:min(255,round(x*((lo+hi)/2)))))
    return struct.pack("<6I",0,image.width,image.height,len(levels),21,len(levels[0]))+b"".join(levels)

def restore_dxt5_alpha_blocks(encoded, original):
    """DXT5 stores alpha and RGB in separate 8-byte halves of each block."""
    out=bytearray(encoded); _,w,h,mips,fmt,_=struct.unpack("<6I",original[:24]); assert fmt==int.from_bytes(b"DXT5","little")
    new_at=old_at=24
    for _ in range(mips):
        blocks=((w+3)//4)*((h+3)//4)
        for block in range(blocks): out[new_at+block*16:new_at+block*16+8]=original[old_at+block*16:old_at+block*16+8]
        new_at+=blocks*16; old_at+=blocks*16; w=max(1,w//2); h=max(1,h//2)
    assert new_at==len(out) and old_at==len(original)
    return bytes(out)

def registered_cleanup(original):
    """Mild RGB cleanup only: suppress decoded block noise, restore local detail."""
    alpha=original.getchannel("A")
    rgb=original.convert("RGB")
    denoised=rgb.filter(ImageFilter.MedianFilter(3))
    mixed=Image.blend(rgb,denoised,0.22)
    mixed=mixed.filter(ImageFilter.UnsharpMask(radius=1.1,percent=45,threshold=5))
    out=mixed.convert("RGBA"); out.putalpha(alpha)
    assert out.getchannel("A").tobytes()==alpha.tobytes()
    return out

def match_color_stats(image, reference):
    """Keep the CC0 detail while fitting the game's established terrain palette."""
    source=image.convert("RGB"); target=reference.convert("RGB"); out=[]
    ss=ImageStat.Stat(source); ts=ImageStat.Stat(target)
    for i,channel in enumerate(source.split()):
        scale=min(1.35,max(0.65,ts.stddev[i]/max(1.0,ss.stddev[i])))
        offset=ts.mean[i]-ss.mean[i]*scale
        out.append(channel.point(lambda v,s=scale,o=offset:max(0,min(255,round(v*s+o)))))
    return Image.merge("RGB",out)

def main():
    for d in (ROOT/"sources",ROOT/"prepared",ROOT/"previews"): d.mkdir(parents=True,exist_ok=True)
    records=[]; comparisons=[]
    # True seamless materials: replace from authoritative CC0 4K diffuse files.
    for name,meta in CC0.items():
        source=ROOT/"sources"/f"{meta['asset']}_diff_4k.png"
        if not source.exists(): urllib.request.urlretrieve(meta["url"],source)
        assert sha(source.read_bytes())==meta["sha256"], f"Changed CC0 source payload: {source.name}"
        incoming=Image.open(source).convert("RGB")
        assert incoming.size==(4096,4096)
        original=Image.open(GROUND/f"{name}-original.png").convert("RGBA")
        image=match_color_stats(incoming.resize((512,512),Image.Resampling.LANCZOS),original).convert("RGBA")
        data=encode_dxt(image,"DXT1",5 if name=="1grass1" else 8)
        target=ROOT/"prepared"/f"{name}.tga.tx"; target.write_bytes(data)
        decoded=codec.decode_tx(data); decoded.save(ROOT/"previews"/f"{name}-decoded.png")
        comparisons.append((name,original,decoded))
        records.append({"target":target.name,"prepared_path":str(target.relative_to(MATERIALS)),"role":"seamless close-range ground diffuse","method":"CC0 replacement, bounded mean/contrast palette match to original","source":meta["url"],"source_page":meta["page"],"author":meta["author"],"license":"CC0-1.0","license_url":"https://polyhaven.com/license","source_sha256":sha(source.read_bytes()),"original_sha256":sha((SOURCE_TEXTURES/f"{name}.tga.tx").read_bytes()),"output_sha256":sha(data),"width":512,"height":512,"format":"DXT1","mip_count":5 if name=="1grass1" else 8,"alpha":"opaque","composition":"full square uniform downsample; source is a seamless material; original per-channel mean and bounded contrast restored","runtime_status":"candidate only; requires representative scene acceptance"})
    # Fixed path/transition paintings: no geometric or compositional edits.
    for name in ("1grasstogrounsU1","1grasstogrounsU2","jungleU1","jungleU2","jungleU3","jungle_gray"):
        original=Image.open(GROUND/f"{name}-original.png" if (GROUND/f"{name}-original.png").exists() else FOLIAGE/f"{name}-original.png").convert("RGBA")
        improved=registered_cleanup(original)
        header=struct.unpack("<6I",(SOURCE_TEXTURES/f"{name}.tga.tx").read_bytes()[:24])
        data=encode_dxt(improved,"DXT1",header[3])
        target=ROOT/"prepared"/f"{name}.tga.tx"; target.write_bytes(data)
        decoded=codec.decode_tx(data); decoded.save(ROOT/"previews"/f"{name}-decoded.png")
        comparisons.append((name,original,decoded))
        records.append({"target":target.name,"prepared_path":str(target.relative_to(MATERIALS)),"role":"registered UV composite" if name!="jungle_gray" else "opaque jungle diffuse","method":"deterministic RGB block-noise cleanup; no invented detail","source":source_ref((GROUND if (GROUND/f"{name}-original.png").exists() else FOLIAGE)/f"{name}-original.png"),"license":"user-supplied game asset","original_sha256":sha((SOURCE_TEXTURES/f"{name}.tga.tx").read_bytes()),"output_sha256":sha(data),"width":original.width,"height":original.height,"format":"DXT1","mip_count":header[3],"alpha":"opaque","composition":"pixel registration, dimensions, aspect and authored path/transition layout retained","limitation":"cleanup reduces DXT noise but cannot create true high-resolution detail or repair an authored material seam","runtime_status":"candidate only; requires representative scene acceptance"})
    # Reuse the two completed generated atlases whose manifests already prove exact alpha.
    foliage_manifest={x["target"]:x for x in json.loads((FOLIAGE/"manifest.json").read_text())}
    for name in ("treePalms","trees"):
        src=FOLIAGE/"prepared"/f"{name}.tga.tx"; target=ROOT/"prepared"/src.name
        shutil.copyfile(src,target); row=dict(foliage_manifest[target.name]); row["prepared_path"]=str(target.relative_to(MATERIALS)); row["runtime_status"]="not staged"; records.append(row)
        comparisons.append((name,Image.open(FOLIAGE/f"{name}-original.png"),codec.decode_tx(target.read_bytes())))
    # Remaining alpha atlases: conservative registered cleanup with byte-exact alpha.
    for name in ("leafPalms","PalmsAlphaLitle01","PalmsSprites"):
        original=Image.open(FOLIAGE/f"{name}-original.png").convert("RGBA"); improved=registered_cleanup(original)
        raw=(SOURCE_TEXTURES/f"{name}.tga.tx").read_bytes(); header=struct.unpack("<6I",raw[:24]); fmt=header[4]
        if fmt==21: data=encode_raw_argb(improved,header[3])
        elif fmt==int.from_bytes(b"DXT5","little"): data=restore_dxt5_alpha_blocks(encode_dxt(improved,"DXT5",header[3]),raw)
        else:
            # leafPalms uses legacy format 25 (A4R4G4B4). Keep it byte-identical
            # until that codec has a proven encoder; UV/alpha safety wins here.
            assert fmt==25; data=raw
        target=ROOT/"prepared"/f"{name}.tga.tx"; target.write_bytes(data)
        decoded=original if fmt==25 else codec.decode_tx(data); decoded.save(ROOT/"previews"/f"{name}-decoded.png")
        assert improved.getchannel("A").tobytes()==original.getchannel("A").tobytes()
        if fmt!=21:
            assert codec.decode_tx(data).getchannel("A").tobytes()==codec.decode_tx(raw).getchannel("A").tobytes()
        comparisons.append((name,original,decoded))
        records.append({"target":target.name,"prepared_path":str(target.relative_to(MATERIALS)),"role":"fixed-UV alpha foliage atlas","method":"byte-identical carry-forward pending format-25 encoder" if fmt==25 else "deterministic registered RGB cleanup","source":source_ref(FOLIAGE/f"{name}-original.png"),"license":"user-supplied game asset","original_sha256":sha(raw),"output_sha256":sha(data),"width":original.width,"height":original.height,"format":"A8R8G8B8" if fmt==21 else ("A4R4G4B4" if fmt==25 else "DXT5"),"mip_count":header[3],"alpha":"all bytes unchanged" if fmt==25 else "byte-exact level-0 alpha; original DXT5 alpha blocks retained at every mip","composition":"no crop, warp, repack or silhouette edits","limitation":"unchanged: no proven format-25 encoder" if fmt==25 else "cleanup only; a convincing foliage modernization requires registered image-generation RGB while retaining original alpha","runtime_status":"not staged"})
    (ROOT/"manifest.json").write_text(json.dumps({"scope":"source-only; no runtime writes","pillow_version":pillow_version,"records":records},indent=2)+"\n")
    tw=512; th=256; sheet=Image.new("RGB",(tw*2,th*len(comparisons)+34),(24,24,24)); draw=ImageDraw.Draw(sheet)
    draw.text((8,8),"ORIGINAL",fill="white"); draw.text((tw+8,8),"PREPARED / DECODED",fill="white")
    for i,(name,a,b) in enumerate(comparisons):
        y=34+i*th
        for x,im in ((0,a),(tw,b)):
            tile=Image.new("RGBA",im.size,"#263036"); tile.alpha_composite(im.convert("RGBA")); tile.thumbnail((tw,th),Image.Resampling.LANCZOS)
            sheet.paste(tile.convert("RGB"),(x+(tw-tile.width)//2,y+(th-tile.height)//2))
        draw.text((8,y+8),name,fill="white")
    sheet.save(ROOT/"previews"/"contact-sheet.jpg",quality=92)
    print(json.dumps({"records":len(records),"prepared":str(ROOT/"prepared"),"contact_sheet":str(ROOT/"previews"/"contact-sheet.jpg")}))

if __name__=="__main__": main()
