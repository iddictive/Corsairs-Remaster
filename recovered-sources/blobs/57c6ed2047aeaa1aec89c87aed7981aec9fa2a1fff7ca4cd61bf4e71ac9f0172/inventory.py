#!/usr/bin/env python3
import hashlib, io, json, struct, subprocess, re
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
STORM = ROOT / 'experiments/native-storm/.cache/runtime/RESOURCE'
METAL = ROOT / 'experiments/native-metal/.cache/runtime/RESOURCE'
OUT = Path(__file__).resolve().parent
NAMES = '''1grass1 1grasstogrounsU1 1grasstogrounsU2 1grasstorock1 1grasstosand1 1sand1 1sandShore1 1sandShoretoSand1 Antigua_island Dirt GrottoLand GrottoLandU1 GrottoLandU2 Ground100km GroundFortLand Guadeloupe_land Path Sandtile SentMartin_terra1 SentMartin_terra2 SentMartin_terra3 SentMartin_terra4 alpha_path beach beach1 beach2 bortoutbrigShore bortoutfleuShoreU6 bump_land bump_land_j bump_sand deckShore dirtK2 dirtK3 dirtM1 fort_dirt grassgreen1 occlShoreShipU1 shoreShipU1 shoreStMartin shoreU1 shoreU2 shoreU3 shoreU5 shoreU6'''.split()
FORMATS = {21: 'A8R8G8B8', 23: 'R5G6B5', 25: 'A1R5G5B5', 827611204: 'DXT1', 861165636: 'DXT3', 894720068: 'DXT5'}

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def header(p): return struct.unpack('<6I', p.read_bytes()[:24])
def level_bytes(w, h, fmt):
    if fmt in (21, 22, 23, 25, 26): return w*h*(4 if fmt in (21,22) else 2)
    return ((w+3)//4)*((h+3)//4)*(8 if fmt == 827611204 else 16)
def mip_probe(h, total):
    _, w, hh, declared, fmt, first = h
    levels=[]; cw,ch=w,hh
    while True:
        levels.append(level_bytes(cw,ch,fmt))
        if cw == 1 and ch == 1: break
        cw,ch=max(1,cw//2),max(1,ch//2)
    expected=sum(levels)
    return {'declared_mips':declared,'geometric_mips':len(levels),'declared_level0_bytes':first,
            'geometric_payload_bytes':expected,'actual_payload_bytes':total-24,
            'payload_chain_valid':expected == total-24 and first == levels[0]}
consumer_map={name:[] for name in NAMES}
pattern='|'.join(re.escape(f'{name}.tga') for name in NAMES)
files=[str(p) for p in STORM.rglob('*') if p.suffix.lower() in ('.gm','.fx')]
scan=subprocess.run(['rg','-a','-i','-o','--no-line-number',pattern,*files],capture_output=True,text=True).stdout.splitlines()
for row in scan:
    path, match = row.rsplit(':',1)
    for name in NAMES:
        if match.casefold() == f'{name}.tga'.casefold():
            rel=str(Path(path).relative_to(STORM))
            if rel not in consumer_map[name]: consumer_map[name].append(rel)

rows=[]
for name in NAMES:
    s=STORM/'Textures'/f'{name}.tga.tx'; m=METAL/'Textures'/f'{name}.tga.tx'
    if not s.exists(): continue
    sd=s.read_bytes(); sh=header(s); mh=header(m) if m.exists() else None
    consumers_s=consumer_map[name]
    alpha='opaque' if sh[4] in (23,827611204) else ('alpha-capable' if sh[4] in (21,25,26,861165636,894720068) else 'unknown')
    quality='low-quality' if sh[1] <= 512 and sh[2] <= 512 else 'adequate-resolution'
    if name == 'Sandtile': quality='low-quality-and-contract-mismatch'
    disposition='candidate-replaced-prepared' if name == 'Sandtile' else ('unresolved-no-discoverable-consumer' if not consumers_s else 'kept')
    rows.append({'target':f'{name}.tga.tx','storm_path':str(s.relative_to(ROOT)),'metal_path':str(m.relative_to(ROOT)) if m.exists() else None,
      'storm_sha256':sha(s),'metal_sha256':sha(m) if m.exists() else None,'storm_header':{'flags':sh[0],'width':sh[1],'height':sh[2],'mips':sh[3],'format':FORMATS.get(sh[4],str(sh[4])),'format_code':sh[4],'level0_bytes':sh[5]},
      'metal_header':({'flags':mh[0],'width':mh[1],'height':mh[2],'mips':mh[3],'format':FORMATS.get(mh[4],str(mh[4])),'format_code':mh[4],'level0_bytes':mh[5]} if mh else None),
      'alpha':alpha,'quality':quality,'mip_probe':mip_probe(sh,len(sd)),'consumers':consumers_s,'consumer_count':len(consumers_s),'disposition':disposition})

(OUT/'inventory.json').write_text(json.dumps({'family':'earth/sand/roads','source_owner':'native-storm','comparison_owner':'native-metal','rows':rows},indent=2,sort_keys=True)+'\n')
print(f'rows={len(rows)} consumers={sum(r["consumer_count"] for r in rows)}')
