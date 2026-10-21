"""Bounded static GM probe; layout follows Storm geometry/src/rdf.h.
No game files are modified. Animated vertices and missing textures fail closed.
"""
import json, struct, sys
from pathlib import Path

def prepare(model, texture_root, output):
    data = model.read_bytes(); offset = 0
    def read(fmt):
        nonlocal offset
        size = struct.calcsize('<' + fmt)
        if offset + size > len(data): raise ValueError('Truncated GM')
        result = struct.unpack_from('<' + fmt, data, offset); offset += size
        return result
    def skip(size):
        nonlocal offset
        if size < 0 or offset + size > len(data): raise ValueError('Invalid GM range')
        offset += size
    h = read('11i7f')
    if any(n < 0 or n > len(data) for n in h[2:11]): raise ValueError('Invalid GM counts')
    strings = data[offset:offset+h[2]]; skip(h[2]); skip(h[3]*4)
    def name(n):
        if not 0 <= n < len(strings): raise ValueError('Invalid string offset')
        return strings[n:].split(b'\0',1)[0].decode('cp1251')
    textures = [name(read('i')[0]) for _ in range(h[4])]
    materials = [read('2i4f8i') for _ in range(h[5])]
    skip(h[6]*80); skip(h[7]*108)
    objects = [read('3i4f6i8i4ii') for _ in range(h[8])]
    triangles = [read('3H') for _ in range(h[9])]
    desc = [read('2i') for _ in range(h[10])]; buffers=[]
    for typ,size in desc:
        if typ >> 2: raise ValueError('Animated GM outside this static probe')
        stride=36+(typ&3)*8
        if size%stride: raise ValueError('Invalid vertex stride')
        raw=data[offset:offset+size]; skip(size); buffers.append((raw,stride))
    index={str(p.relative_to(texture_root)).replace('\\','/').lower():p for p in texture_root.rglob('*') if p.is_file()}
    packed=bytearray(); draws=[]; points=[]
    for obj in objects:
        vb,nt,st,nv,sv,mi=obj[7:13]
        if not (0<=vb<len(buffers) and 0<=mi<len(materials) and 0<=st<=st+nt<=len(triangles)): raise ValueError('Invalid object')
        material=materials[mi]; ti=material[10]
        if not 0<=ti<len(textures): raise ValueError('No base texture')
        requested=textures[ti].replace('\\','/').lower()
        if not requested.endswith('.tx'): requested += '.tx'
        tex=index.get(requested)
        if tex is None: raise ValueError('Missing texture '+requested)
        start=len(packed)//32; raw,stride=buffers[vb]
        for tri in triangles[st:st+nt]:
            for idx in tri:
                idx += sv
                if idx*stride+36>len(raw): raise ValueError('Invalid vertex index')
                v=struct.unpack_from('<6fI2f',raw,idx*stride)
                points.append(v[:3]); packed.extend(struct.pack('<8f',*v[:6],*v[7:9]))
        draws.append({'start':start,'count':len(packed)//32-start,'texture':str(tex)})
    if not points: raise ValueError('Empty model')
    low=[min(p[i] for p in points) for i in range(3)]; high=[max(p[i] for p in points) for i in range(3)]
    output.mkdir(parents=True,exist_ok=True)
    (output/'vertices.bin').write_bytes(packed)
    (output/'scene.json').write_text(json.dumps({'model':str(model),'center':[(a+b)/2 for a,b in zip(low,high)],'scale':2/max(b-a for a,b in zip(low,high)),'draws':draws}))
    print(f'{model.name}: {len(points)//3} triangles, {len(draws)} draws, {len(set(d["texture"] for d in draws))} textures')

if __name__=='__main__':
    try: prepare(*map(Path,sys.argv[1:4]))
    except (ValueError, OSError, struct.error, IndexError) as e: sys.exit(str(e))
