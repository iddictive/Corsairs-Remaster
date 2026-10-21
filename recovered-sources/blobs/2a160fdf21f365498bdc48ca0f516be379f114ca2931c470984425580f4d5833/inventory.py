#!/usr/bin/env python3
import hashlib, json, re, struct, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
STORM = ROOT / 'experiments/native-storm/.cache/runtime/RESOURCE/Textures'
METAL = ROOT / 'experiments/native-metal/.cache/runtime/RESOURCE/Textures'
GM = ROOT / 'experiments/native-storm/.cache/runtime/RESOURCE'

# Architectural town envelope/wall/roof families.  This is intentionally
# name-based for candidate discovery only; consumers are proven separately.
FAMILY = re.compile(r'(wall|roof|house|church|fort|tavern|store|bank|barrack|residence|hut|warehouse|shipyard|town_|town|brick|stoneh|stonehouse|flaman|wind.*house|wind.*church|wind.*tavern|wind.*store|wind.*residence|wind.*shipyard|wind.*fort|portoffice|mediumhouse|smallhouse|two.?storey|thouse|cobhouse)', re.I)

def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''): h.update(b)
    return h.hexdigest()

def level_bytes(fmt, w, h):
    blocks = ((max(1,w)+3)//4) * ((max(1,h)+3)//4)
    if fmt == 'DXT1': return blocks * 8
    if fmt in ('DXT3','DXT5'): return blocks * 16
    if fmt == 'A8R8G8B8': return w*h*4
    if fmt == 'A4R4G4B4': return w*h*2
    if fmt == 'R5G6B5': return w*h*2
    return None

def alpha_info(fmt, w, h, payload):
    if fmt == 'DXT1':
        # DXT1 transparency is contract-visible from endpoint ordering and
        # selector 3; inspect every block without expanding large images.
        has_zero = has_full = has_other = False
        off=0
        for _ in range(((w+3)//4)*((h+3)//4)):
            c0,c1,selectors = struct.unpack_from('<HHI',payload,off); off += 8
            if c0 > c1: has_full = True
            else:
                for i in range(16):
                    if ((selectors >> (2*i)) & 3) == 3: has_zero = True
            if has_zero: break
        return 'binary' if has_zero else 'opaque'
    if fmt in ('DXT3','DXT5'):
        # DXT3 stores explicit 4-bit alpha; DXT5 stores endpoint/index alpha.
        if fmt == 'DXT3':
            vals=set()
            for off in range(0, len(payload), 16):
                for b in payload[off:off+8]: vals.add((b>>4)*17); vals.add((b&15)*17)
        else:
            vals=set(); off=0
            for _ in range(((w+3)//4)*((h+3)//4)):
                a0,a1=payload[off],payload[off+1]; vals.update((a0,a1)); off += 8
                bits=int.from_bytes(payload[off:off+6],'little'); off += 8
                vals.update((a0*(7-i)+a1*i)//7 for i in range(1,7))
        if vals == {255}: return 'opaque'
        if vals <= {0,255}: return 'binary'
        return 'continuous'
    if fmt == 'A8R8G8B8':
        vals = set(payload[3::4])
    elif fmt == 'A4R4G4B4':
        vals = {(v >> 12) * 17 for (v,) in struct.iter_unpack('<H', payload)}
    else: return 'opaque-by-format'
    if vals == {255}: return 'opaque'
    if vals <= {0,255}: return 'binary'
    return 'continuous'

def inspect(p):
    b = p.read_bytes()
    flags,w,h,mips,fmt_id,size = struct.unpack('<6I', b[:24])
    fourcc = fmt_id.to_bytes(4,'little', signed=False)
    fmt = {b'DXT1':'DXT1',b'DXT3':'DXT3',b'DXT5':'DXT5',b'\x15\x00\x00\x00':'A8R8G8B8',b'\x19\x00\x00\x00':'A4R4G4B4',b'\x17\x00\x00\x00':'R5G6B5'}.get(fourcc, fourcc.decode('latin1','replace'))
    payload = b[24:24+size]
    expected=[]; cw,ch=w,h
    for _ in range(mips):
        expected.append(level_bytes(fmt,cw,ch)); cw,ch=max(1,cw//2),max(1,ch//2)
    valid = len(b) == 24+size and sum(x for x in expected if x is not None) == size and cw == 1 and ch == 1
    return dict(path=str(p.relative_to(ROOT)), sha256=sha(p), bytes=len(b), width=w, height=h,
                header_format=fmt, format_id=fmt_id, mip_count=mips, mip_chain_valid=valid,
                alpha=alpha_info(fmt,w,h,payload), storm_sha256=sha(STORM/p.name),
                metal_sha256=sha(METAL/p.name) if (METAL/p.name).exists() else None,
                same_bytes=(METAL/p.name).exists() and p.read_bytes()==(METAL/p.name).read_bytes())

def consumer_map(names):
    stems = {name: (name[:-7] if name.lower().endswith('.tga.tx') else name) for name in names}
    result={name:[] for name in names}
    # Let ripgrep's compiled scanner identify the small candidate corpus;
    # exact per-name matching below prevents similar-name attribution.
    pattern = '|'.join(re.escape(v) for v in stems.values())
    scan = subprocess.run(['rg','-a','-l','-i','--glob','*.gm','--glob','*.fx',pattern,str(GM)],capture_output=True,text=True,check=False)
    candidates=[Path(x) for x in scan.stdout.splitlines()]
    for p in candidates:
        hit=p.read_bytes(); path=str(p.relative_to(ROOT))
        for name, stem in stems.items():
            if re.search(rb'(?i)(?<![A-Za-z0-9_])'+re.escape(stem.encode())+rb'(?![A-Za-z0-9_])',hit):
                result[name].append(path)
    return result

def main():
    files = sorted(p for p in STORM.glob('*.tga.tx') if FAMILY.search(p.name))
    rows=[]; cmap=consumer_map([p.name for p in files])
    for p in files:
        row=inspect(p); row['family_match']=FAMILY.pattern; row['consumers']=cmap[p.name]
        row['disposition']='kept' if row['alpha'] in ('opaque','binary','continuous') and row['mip_chain_valid'] else 'unresolved'
        row['reason']='inventory-only; no replacement accepted or staged' if row['disposition']=='kept' else 'invalid or undecodable TX contract'
        rows.append(row)
    report={'schema':1,'family':'town-walls-roofs','classification_rule':FAMILY.pattern,
            'canonical':'experiments/native-storm/.cache/runtime/RESOURCE/Textures',
            'comparison':'experiments/native-metal/.cache/runtime/RESOURCE/Textures',
            'consumer_corpus':'experiments/native-storm/.cache/runtime/RESOURCE',
            'rows':rows}
    (OUT/'inventory.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'rows':len(rows),'with_consumers':sum(bool(x['consumers']) for x in rows),
                      'invalid':sum(not x['mip_chain_valid'] for x in rows),
                      'storm_metal_byte_equal':sum(x['same_bytes'] for x in rows)},sort_keys=True))
if __name__=='__main__': main()
