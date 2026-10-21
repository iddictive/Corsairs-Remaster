#!/usr/bin/env python3
"""Read-only deterministic inventory for the interiors/objects texture lane."""
from __future__ import annotations
import hashlib, json, mmap, re, struct, subprocess, tempfile, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
STORM = ROOT / "experiments/native-storm/.cache/runtime/RESOURCE"
METAL = ROOT / "experiments/native-metal/.cache/runtime/RESOURCE"
OUT = Path(__file__).resolve().parent
EXCLUDED = ("Ships/", "WorldMap/", "INTERFACES/", "MainMenu/", "Items/", "Ammo/")
TOKENS = re.compile(r"(?i)(interior|inside|wall|floor|roof|door|window|house|hut|tavern|store|shop|bank|church|fort|castle|residence|office|barn|warehouse|shipyard|brothel|farmacia|furniture|furnit|object|chest|lamp|bed|chair|armchair|sofa|cabinet|desk|candle|crockery|carpet|tile|parquet|plank|wood|stone|brick|paver|stair|ladder|gate|fence|grating|glass|plant|flower|statue|sculpture|ornament|rope|chain|barrel|crate|stuff|decor|balk|balki|foundation|estate|portoffice|windmill|warehouse)")
FMT = {21:"A8R8G8B8",22:"X8R8G8B8",23:"R5G6B5",24:"X8B8G8R8",25:"A4R4G4B4",26:"X4R4G4B4",28:"A8",77:"A8L8",int.from_bytes(b"DXT1","little"):"DXT1",int.from_bytes(b"DXT3","little"):"DXT3",int.from_bytes(b"DXT5","little"):"DXT5"}

def sha(p: Path) -> str:
    h=hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda:f.read(1<<20),b""): h.update(b)
    return h.hexdigest()

def tx_info(p: Path) -> dict:
    b=p.read_bytes(); result={"bytes":len(b),"sha256":hashlib.sha256(b).hexdigest()}
    if len(b)<24:
        result.update(error="short-header"); return result
    flags,w,h,mips,fmt,level0=struct.unpack_from("<6I",b)
    name=FMT.get(fmt,f"UNKNOWN({fmt})"); result.update(width=w,height=h,mip_count=mips,format=name,header_level0_bytes=level0)
    levels=[]; off=24; valid=True; alpha_values=[]
    for i in range(mips):
        lw=max(1,w>>i); lh=max(1,h>>i)
        if name in ("DXT1","DXT3","DXT5"):
            expected=((lw+3)//4)*((lh+3)//4)*(8 if name=="DXT1" else 16)
        elif name=="A8": expected=lw*lh
        elif name in ("R5G6B5","A4R4G4B4","X4R4G4B4","A8L8"): expected=lw*lh*2
        elif name in ("A8R8G8B8","X8R8G8B8","X8B8G8R8"): expected=lw*lh*4
        else: expected=0
        actual=min(expected,max(0,len(b)-off)); payload=b[off:off+expected]
        if name=="DXT1":
            for j in range(0,len(payload)-7,8):
                c0,c1=struct.unpack_from("<HH",payload,j); idx=struct.unpack_from("<I",payload,j+4)[0]
                alpha_values.extend([0 if c0<=c1 and ((idx>>(2*n))&3)==3 else 255 for n in range(16)])
        elif name=="DXT3":
            for j in range(0,len(payload)-15,16):
                alpha_values.extend([((payload[j+n//2] >> (4*(n%2)))&15)*17 for n in range(16)])
        elif name=="DXT5":
            for j in range(0,len(payload)-15,16):
                a0,a1=payload[j],payload[j+1]; bits=int.from_bytes(payload[j+2:j+8],"little")
                table=[a0,a1]+([round(((6-k)*a0+k*a1)/7) for k in range(1,7)] if a0>a1 else [round(((4-k)*a0+k*a1)/5) for k in range(1,5)]+[0,255])
                alpha_values.extend(table[(bits>>(3*n))&7] for n in range(16))
        elif name=="A8": alpha_values.extend(payload)
        elif name=="A8L8": alpha_values.extend(payload[1::2])
        elif name=="A8R8G8B8": alpha_values.extend(payload[3::4])
        elif name=="A4R4G4B4": alpha_values.extend(((v>>12)&15)*17 for v, in struct.iter_unpack("<H",payload))
        elif name.startswith("X") or name in ("R5G6B5",): alpha_values.extend([255]*((lw*lh)))
        levels.append({"level":i,"width":lw,"height":lh,"expected_bytes":expected,"actual_bytes":actual,"valid":actual==expected})
        valid &= actual==expected; off += expected
    result.update(mip_chain_valid=bool(valid and off==len(b)),levels=levels)
    alpha="unknown"
    if alpha_values:
        lo,hi=min(alpha_values),max(alpha_values)
        if lo==hi==255: alpha="opaque"
        elif lo in (0,) and hi==255 and not any(0<v<255 for v in alpha_values): alpha="binary-alpha"
        elif lo<255: alpha="partial-alpha"
        else: alpha="opaque"
    result["alpha_behavior"]=alpha
    return result

def build_consumer_index(resource: Path) -> dict[str, list[str]]:
    stems={p.name[:-3] for p in (STORM/"Textures").glob("*.tx")}
    result={stem:[] for stem in stems}
    with tempfile.NamedTemporaryFile("w",encoding="utf-8") as patterns:
        patterns.write("\n".join(sorted(stems))+"\n"); patterns.flush()
        cmd=["rg","-a","-i","-l","-F","-f",patterns.name,"--glob","*.gm","--glob","*.fx",str(resource)]
        proc=subprocess.run(cmd,capture_output=True,text=True,check=True)
    needles=[(s,s.lower().encode()) for s in stems]
    for name in proc.stdout.splitlines():
        p=Path(name)
        with p.open("rb") as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as data:
            hits={stem for stem,needle in needles if needle in data}
        rel=str(p.relative_to(resource))
        for stem in hits:
            if stem in result: result[stem].append(rel)
    return result

def consumers(index: dict[str, list[str]], filename: str) -> list[str]:
    return index.get(filename, [])

def family_files() -> list[str]:
    out=[]
    for p in sorted((STORM/"Textures").rglob("*.tx")):
        rel=str(p.relative_to(STORM/"Textures"))
        if rel.startswith(EXCLUDED) or "/" in rel: continue
        if TOKENS.search(rel): out.append(rel)
    return out

def main():
    prior=None
    if len(sys.argv)>1 and sys.argv[1]=="--refresh-alpha" and (OUT/"inventory.json").exists():
        prior=json.loads((OUT/"inventory.json").read_text())
    consumer_index=({r["target"][:-3]:r["exact_consumers_gm_fx"] for r in prior["rows"]} if prior else None)
    if consumer_index is None: consumer_index=build_consumer_index(STORM)
    rows=[]
    for rel in family_files():
        s=STORM/"Textures"/rel; m=METAL/"Textures"/rel
        row={"target":rel,"storm":tx_info(s),"metal":tx_info(m) if m.exists() else {"missing":True},"exact_consumers_gm_fx":consumers(consumer_index,rel[:-3])}
        if m.exists(): row["storm_metal_same_sha256"]=row["storm"]["sha256"]==row["metal"]["sha256"]
        row["disposition"]="kept" if not row["storm"].get("error") and row["storm"].get("mip_chain_valid") else "unresolved"
        rows.append(row)
    manifest={"lane":"interiors/objects","selection_rule":"root Textures only; explicit architectural/interior/object token; excluded Ships, WorldMap, INTERFACES, MainMenu, Items, Ammo and nested directories","source_owner":"experiments/native-storm/.cache/runtime/RESOURCE/Textures","comparison_owner":"experiments/native-metal/.cache/runtime/RESOURCE/Textures (read-only)","counts":{"rows":len(rows),"kept":sum(r["disposition"]=="kept" for r in rows),"replaced":0,"rejected":0,"unresolved":sum(r["disposition"]=="unresolved" for r in rows)},"rows":rows}
    (OUT/"inventory.json").write_text(json.dumps(manifest,indent=2,sort_keys=True)+"\n")
    print(json.dumps(manifest["counts"],sort_keys=True))

if __name__=="__main__": main()
