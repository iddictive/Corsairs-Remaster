#!/usr/bin/env python3
"""Read installed stock root GM/TX files; produce geometry facts, not material truth."""
import collections
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct

APP = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE')
OUT = Path(__file__).resolve().parent
REPO = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
spec = importlib.util.spec_from_file_location('material_prepare', REPO / 'experiments/native-metal/materials/prepare.py')
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)

def parse(path):
    data = path.read_bytes()
    h = struct.unpack_from('<11i7f', data)
    names = data[72:72+h[2]]
    def name(offset):
        return names[offset:names.index(b'\0', offset)].decode('cp1251')
    pos = 72+h[2]+h[3]*4
    textures = struct.unpack_from('<%di' % h[4], data, pos)
    pos += h[4]*4
    materials = [struct.unpack_from('<2i4f8i', data, pos+i*56) for i in range(h[5])]
    pos += h[5]*56+h[6]*80+h[7]*108
    objects = [struct.unpack_from('<3i4f19i', data, pos+i*104) for i in range(h[8])]
    pos += h[8]*104
    triangles = [struct.unpack_from('<3H', data, pos+i*6) for i in range(h[9])]
    pos += h[9]*6
    descriptors = [struct.unpack_from('<2i', data, pos+i*8) for i in range(h[10])]
    pos += h[10]*8
    buffers = []
    for typ, size in descriptors:
        stride = 36+(typ&3)*8+(typ>>2)*8
        assert size % stride == 0
        buffers.append([struct.unpack_from('<3f', data, p) for p in range(pos, pos+size, stride)])
        pos += size
    rows = []
    for index, ob in enumerate(objects):
        mat = materials[ob[12]]
        texture = name(textures[mat[10]]) if mat[6] == 1 else None
        area = submerged = horizontal = vertical = 0.0
        lo = [math.inf]*3
        hi = [-math.inf]*3
        for a, b, c in triangles[ob[9]:ob[9]+ob[8]]:
            ps = [buffers[ob[7]][ob[11]+v] for v in (a,b,c)]
            for p in ps:
                for k in range(3):
                    lo[k] = min(lo[k],p[k]); hi[k] = max(hi[k],p[k])
            a = [ps[1][k]-ps[0][k] for k in range(3)]
            b = [ps[2][k]-ps[0][k] for k in range(3)]
            cr = [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
            ar = math.sqrt(sum(v*v for v in cr))*.5
            area += ar
            if max(v[1] for v in ps) < 0: submerged += ar
            if ar > 0:
                y = abs(cr[1])/(ar*2)
                if y > .85: horizontal += ar
                if y < .3: vertical += ar
        rows.append(dict(object=index,name=name(ob[1]),flags=hex(ob[2]),material=ob[12],texture=texture,
                         triangles=ob[8],area=area,submerged=submerged,horizontal=horizontal,
                         vertical=vertical,lo=lo,hi=hi))
    return dict(model=path.parent.name,path=str(path),sha256=hashlib.sha256(data).hexdigest(),objects=rows)

def aggregate(rows):
    grouped = {}
    for row in rows:
        key = row['texture']
        entry = grouped.setdefault(key,dict(texture=key,objects=[],area=0.,submerged=0.,horizontal=0.,vertical=0.,
                                            lo=[math.inf]*3,hi=[-math.inf]*3))
        entry['objects'].append(row['object'])
        for k in ('area','submerged','horizontal','vertical'): entry[k] += row[k]
        for k in range(3):
            entry['lo'][k] = min(entry['lo'][k],row['lo'][k]);entry['hi'][k] = max(entry['hi'][k],row['hi'][k])
    return list(grouped.values())

texture_files = collections.defaultdict(list)
for file in (APP/'Textures').rglob('*.tx'):
    texture_files[file.name.lower()].append(file)

def alpha(model,texture):
    if not texture:return []
    files = texture_files[texture.lower()+'.tx']
    local = [p for p in files if any(part.lower()==model.lower() for part in p.parts)]
    if not local:
        local = [p for p in files if p.parent == APP/'Textures']
    result = []
    for path in local:
        try:
            data = path.read_bytes()
            flags,w,height,mips,fmt,size = struct.unpack_from('<6I',data)
            payload = data[24:24+size]
            if fmt == 26:
                rgba = bytes(c for (v,) in struct.iter_unpack('<H',payload)
                             for c in (((v>>10)&31)*255//31,((v>>5)&31)*255//31,(v&31)*255//31,255 if v&0x8000 else 0))
                im = decoder.Image.frombytes('RGBA',(w,height),rgba)
            elif fmt == 22:
                im = decoder.Image.frombytes('RGB',(w,height),payload,'raw','BGRX').convert('RGBA')
            elif fmt == int.from_bytes(b'DXT3','little'):
                im = decoder.Image.frombytes('RGBA',(w,height),payload,'bcn',(2,'DXT3'))
            elif fmt in (21,23,25,int.from_bytes(b'DXT1','little'),int.from_bytes(b'DXT5','little')):
                im = decoder.decode_tx(data)
            else:
                raise ValueError('unsupported TX format '+str(fmt))
            h = im.getchannel('A').histogram(); n = im.width*im.height
            result.append(dict(path=str(path.relative_to(APP)),sha256=hashlib.sha256(data).hexdigest(),
                               format=fmt,dimensions=list(im.size),clear_pct=sum(h[:25])/n*100,
                               mid_pct=sum(h[25:250])/n*100,solid_pct=sum(h[250:])/n*100))
        except Exception as e:result.append(dict(path=str(path.relative_to(APP)),error=str(e)))
    return result

models = []
for directory in sorted((APP/'Models/Ships').iterdir(),key=lambda p:p.name.lower()):
    if not directory.is_dir():continue
    roots = [p for p in directory.glob('*.gm') if p.stem.lower()==directory.name.lower()]
    for path in roots:
        row = parse(path)
        groups = aggregate(row['objects'])
        hulls = sorted(groups,key=lambda a:a['submerged'],reverse=True)
        decks = sorted(groups,key=lambda a:a['horizontal'],reverse=True)
        row['hull_candidates'] = hulls[:3]
        row['deck_candidates'] = decks[:3]
        selected = {c['texture'] for c in hulls[:2]+decks[:2]}
        row['alpha'] = {t:alpha(row['model'],t) for t in selected if t}
        models.append(row)
result = dict(schema=1,owner='parent /root; read-only candidate evidence for explicit ship wood profiles',
              scope='58 installed root models; no visual/semantic acceptance or automatic profile authorization',
              measurement='triangle area m2; fully submerged iff all vertices y<0; horizontal iff abs(face-normal.y)>.85; vertical iff <.3',
              profiles=models)
OUT.joinpath('profiles.json').write_text(json.dumps(result,indent=2)+'\n')
for row in models:
    h=row['hull_candidates'][0]; d=next((x for x in row['deck_candidates'] if x['texture']!=h['texture']),row['deck_candidates'][0])
    def fmt(c):
        aa=row['alpha'].get(c['texture'],[])
        clear=max((a.get('clear_pct',0) for a in aa),default=-1)
        return '%s[%s] A%.0f/U%.0f/H%.0f/C%.2f'%(c['texture'],','.join(map(str,c['objects'])),c['area'],c['submerged'],c['horizontal'],clear)
    print(row['model']+' | '+fmt(h)+' | '+fmt(d),flush=True)
print('FACTS',len(models),OUT/'profiles.json',flush=True)
