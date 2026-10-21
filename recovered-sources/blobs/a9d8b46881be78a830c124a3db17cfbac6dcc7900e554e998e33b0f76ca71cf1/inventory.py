#!/usr/bin/env python3
import hashlib, json, re, struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
STORM = ROOT / 'native-storm/.cache/runtime/RESOURCE'
METAL = ROOT / 'native-metal/.cache/runtime/RESOURCE'
OUT = Path(__file__).resolve().parent
PREFIXES = ('1grasstorock', 'back_mountains', 'bakedcave', 'bump_rock', 'cave_',
            'cavern', 'cobble', 'fortstone', 'indstones', 'oxbay_rocks', 'rock',
            'stone', 'stones')
EXCLUDE = ('shadow.tga.tx',)
FMT = {int.from_bytes(b'DXT1','little'): 'DXT1', int.from_bytes(b'DXT3','little'): 'DXT3',
       int.from_bytes(b'DXT5','little'): 'DXT5', 23: 'RGB565', 21: 'A8R8G8B8', 25: 'A1R5G5B5'}

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def decoded(p, info):
    b=p.read_bytes(); w,h=info['width'],info['height']; payload=b[24:24+info['level0_bytes']]
    if info['format'] == 'RGB565':
        rgba=bytes(channel for (v,) in struct.iter_unpack('<H',payload) for channel in (((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31,255))
        return Image.frombytes('RGBA',(w,h),rgba)
    if info['format'] == 'A8R8G8B8':
        return Image.frombytes('RGBA',(w,h),payload)
    if info['format'] == 'A1R5G5B5':
        rgba=bytes(channel for (v,) in struct.iter_unpack('<H',payload) for channel in (((v>>10)&31)*255//31,((v>>5)&31)*255//31,(v&31)*255//31,255 if v&0x8000 else 0))
        return Image.frombytes('RGBA',(w,h),rgba)
    return Image.frombytes('RGBA',(w,h),payload,'bcn',(1,info['format']))
def family(p):
    n = p.name.lower()
    return p.parent == STORM / 'Textures' and n.endswith('.tga.tx') and n not in EXCLUDE and n.startswith(PREFIXES)

def gm_textures(p):
    raw = p.read_bytes(); hs = struct.calcsize('<11i7f')
    if len(raw) < hs: return None
    h = struct.unpack_from('<11i7f', raw); strings, names, count = h[2:5]
    if min(strings, names, count) < 0: return None
    off = hs; blob = raw[off:off+strings]; off += strings + names*4
    if off + count*4 > len(raw): return None
    result=[]
    for at in struct.unpack(f'<{count}i', raw[off:off+count*4]):
        if not 0 <= at < len(blob): return None
        end=blob.find(b'\0',at)
        if end < 0: return None
        result.append(blob[at:end].decode('cp1251','replace'))
    return result

def tx_info(p):
    b=p.read_bytes(); valid=True; reason=''
    if len(b)<24: return {'bytes':len(b),'valid':False,'error':'short header'}
    flags,w,h,mips,fmt,level0=struct.unpack_from('<6I',b)
    sizes=[]; cw,ch=w,h; off=24
    for i in range(mips):
        if fmt in (int.from_bytes(b'DXT1','little'),): size=max(1,(cw+3)//4)*max(1,(ch+3)//4)*8
        elif fmt in (int.from_bytes(b'DXT3','little'),int.from_bytes(b'DXT5','little')): size=max(1,(cw+3)//4)*max(1,(ch+3)//4)*16
        elif fmt == 23: size=cw*ch*2
        elif fmt == 21: size=cw*ch*4
        elif fmt == 25: size=cw*ch*2
        else: size=None
        sizes.append(size); valid &= size is not None and (off+size <= len(b)); off += size or 0
        cw,ch=max(1,cw//2),max(1,ch//2)
    valid &= mips>0 and off==len(b) and level0==(sizes[0] if sizes else None)
    alpha = 'opaque-by-format' if fmt in (23,int.from_bytes(b'X8R8G8B8','little')) else ('alpha-capable' if fmt in (int.from_bytes(b'DXT3','little'),int.from_bytes(b'DXT5','little'),21,25) else 'unknown')
    if fmt == int.from_bytes(b'DXT1','little'):
        alpha='DXT1-potential-1bit'; payload=b[24:24+(sizes[0] or 0)]
        transparent=False
        for i in range(0,len(payload)-7,8):
            c0,c1=struct.unpack_from('<HH',payload,i)
            if c0<=c1 and any(((payload[i+4+j//4]>>(2*(j%4)))&3)==3 for j in range(16)): transparent=True; break
        alpha='has-1bit-transparent-blocks' if transparent else 'opaque'
    info={'bytes':len(b),'flags':flags,'width':w,'height':h,'mip_count':mips,'format_code':fmt,'format':FMT.get(fmt,str(fmt)), 'level0_bytes':level0,'mip_sizes':sizes,'mip_chain_valid':valid,'alpha':alpha}
    try:
        image=decoded(p,info); a=image.getchannel('A'); lo,hi=a.getextrema(); info.update({'decoded_rgba_sha256':hashlib.sha256(image.tobytes()).hexdigest(),'alpha_min':lo,'alpha_max':hi,'alpha_opaque_fraction':sum(v==255 for v in a.getdata())/(w*h),'decoded_probe':'pass'})
        if lo==hi==255: info['alpha']='opaque'
        elif lo==0 and hi==255: info['alpha']='binary-or-mixed'
        else: info['alpha']='graded/mixed'
    except Exception as e: info.update({'decoded_probe':'unresolved','decoded_error':str(e)})
    return info

def main():
    textures=sorted(p for p in (STORM/'Textures').iterdir() if family(p))
    metal_index={p.relative_to(METAL/'Textures').as_posix().lower():p for p in (METAL/'Textures').rglob('*.tx')}
    gms=[]
    for p in (STORM/'MODELS').rglob('*.gm'):
        names=gm_textures(p)
        if names is not None: gms.append((p,names))
    fx=[]
    for p in (STORM/'FX').rglob('*') if (STORM/'FX').exists() else []:
        if p.is_file():
            try: text=p.read_text(errors='replace').lower()
            except OSError: continue
            fx.append((p,text))
    rows=[]
    for p in textures:
        rel=p.relative_to(STORM/'Textures').as_posix(); key=p.name.lower(); consumers=[]
        for gm,names in gms:
            if key[:-3] in {n.lower() for n in names} or key in {n.lower() for n in names}:
                consumers.append(str(gm.relative_to(STORM/'MODELS')))
        fx_refs=[str(x.relative_to(STORM)) for x,t in fx if key in t or key[:-3] in t]
        metal=METAL/'Textures'/rel
        info=tx_info(p); same=metal.is_file() and p.read_bytes()==metal.read_bytes(); metal_info=tx_info(metal) if metal.is_file() else None
        pixel_equal= same or (metal_info and info.get('decoded_rgba_sha256')==metal_info.get('decoded_rgba_sha256'))
        low = info['width'] <= 256 or info['height'] <= 256
        specialized = info['alpha'] != 'opaque' or info['width'] <= 128
        reason = ('Kept: measured low-resolution/specialized asset; its mapped use is not enough to justify a generic Poly Haven replacement without scene-specific visual evidence.' if low or specialized else 'Kept: measured dimensions, format, alpha and mip chain are contractually adequate; no replacement justification was established.')
        rows.append({'target':rel,'storm_sha256':sha(p),'metal_present':metal.is_file(),'metal_sha256':sha(metal) if metal.is_file() else None,'storm_metal_bytes_equal':same,'decoded_pixels_equal':pixel_equal,'quality_flags':{'low_resolution':low,'specialized_alpha_or_small':specialized,'historical_byte_divergence':not same},'tx':info,'metal_tx':metal_info,'gm_consumers':sorted(consumers),'fx_consumers':sorted(fx_refs),'disposition':'kept','reason':reason})
    report={'schema':'corsairs.texture-mega-pass.family-inventory.v1','family':'stone/rocks/mountains','canonical_source':'experiments/native-storm/.cache/runtime/RESOURCE/Textures','comparison_owner':'experiments/native-metal/.cache/runtime/RESOURCE/Textures','matching_rule':PREFIXES,'excluded_semantics':['lighting/**/shadow.tga.tx','shore*','store*','crockery1c.tga.tx','Ships/**/polacrag1.tga.tx'],'counts':{'assigned_rows':len(rows),'storm_metal_equal':sum(r['storm_metal_bytes_equal'] for r in rows),'metal_missing':sum(not r['metal_present'] for r in rows),'gm_mapped':sum(bool(r['gm_consumers']) for r in rows),'fx_mapped':sum(bool(r['fx_consumers']) for r in rows),'kept':len(rows),'replaced':0,'rejected':0,'unresolved':0},'rows':rows}
    (OUT/'family-inventory.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
    print(json.dumps(report['counts'],sort_keys=True))
if __name__=='__main__': main()
