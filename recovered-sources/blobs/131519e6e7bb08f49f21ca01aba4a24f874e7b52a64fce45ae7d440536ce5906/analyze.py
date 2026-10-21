"""CPU-only material closure for retained eight-Manowar1 fixture (not capture residency)."""
import sys
sys.dont_write_bytecode = True
import hashlib
import importlib.util
import json
import struct
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'tools'))
from patch_metal_portobello_ground import parse_gm
spec = importlib.util.spec_from_file_location('mips', ROOT / 'experiments/native-metal/texture_loader_mip_probe.py')
mips = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mips)
RESOURCE = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE')
TEX = RESOURCE / 'Textures'
OUT = Path(__file__).parent

def resolve(root, relative):
    current = root
    for part in relative.replace('\\', '/').split('/'):
        if not part: continue
        matches = [p for p in current.iterdir() if p.name.lower() == part.lower()]
        if len(matches) != 1: return None
        current = matches[0]
    return current

consumers = defaultdict(list)
models = []
for path in sorted((RESOURCE / 'MODELS/Ships/Manowar1').glob('*.gm')):
    gm = parse_gm(path.read_bytes())
    material_ids = {o[12] for o in gm['objects']}
    used = sorted({gm['textures'][row[10+s]] for i,row in enumerate(gm['materials'])
                   if i in material_ids for s in range(4)
                   if row[6+s] and 0 <= row[10+s] < len(gm['textures'])})
    models.append({'model': str(path.relative_to(RESOURCE)), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                   'objects':len(gm['objects']), 'material_count':len(material_ids), 'textures':used})
    for name in used:
        p = resolve(TEX, 'Ships/Manowar1/Hull1/' + name + '.tx') or resolve(TEX, name + '.tx')
        consumers[str(p.relative_to(TEX)) if p else '?'+name].append(path.name)

rigging = ['ships/parus_common.tga', 'ships/parus_hole.tga', 'ships/emptygerald.tga',
           'sail_rope.tga', 'vant.tga', 'vanx.tga', 'vanz.tga', 'flags/flagall_ship_b.tga']
for name in rigging:
    p=resolve(TEX,name+'.tx')
    consumers[str(p.relative_to(TEX)) if p else '?'+name].append('fixture/rigging source-selected')

rows=[]
for relative,users in sorted(consumers.items()):
    if relative.startswith('?'):
        rows.append({'path':relative,'consumers':users,'status':'missing'});continue
    p=TEX/relative
    with p.open('rb') as f: header=f.read(24)
    flags,w,h,n,fmt,first=struct.unpack('<6I',header)
    sizes,required=mips.plan(header,p.stat().st_size-24)
    rgba=sum(max(1,w>>i)*max(1,h>>i)*4 for i in range(n)) * (6 if flags&2 else 1)
    rows.append({'path':relative,'consumers':users,'width':w,'height':h,'mips':n,'flags':flags,
                 'format':fmt.to_bytes(4,'little').decode('ascii') if fmt in (mips.DXT1,mips.DXT3,mips.DXT5) else str(fmt),
                 'file_bytes':p.stat().st_size,'payload_bytes':required,'rgba8_logical_bytes':rgba,
                 'mip_payload_sizes':sizes,'status':'header/payload checked'})

def total(rows):
    valid=[r for r in rows if 'payload_bytes' in r]
    return {'textures':len(valid),'formats':dict(Counter(r['format'] for r in valid)),
            'authored_payload_bytes':sum(r['payload_bytes'] for r in valid),
            'rgba8_logical_bytes':sum(r['rgba8_logical_bytes'] for r in valid),
            'saved_logical_bytes':sum(r['rgba8_logical_bytes']-r['payload_bytes'] for r in valid)}

result={'scope':'Source-associated Manowar1/hull1 GM material closure and fixture rigging. Not exact capture residency.',
        'fixture':'experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-012905/fixture.c',
        'texture_degradation':0,'ships':8,
        'deduplication':'TextureCreate caches uppercase resolved path; eight identical hull1 ships share these texture objects.',
        'population':rows,'models':models,'total':total(rows),
        'gm_only':total([r for r in rows if any(u.endswith('.gm') for u in r['consumers'])]),
        'direct_bc_only':total([r for r in rows if r.get('format','').startswith('DXT')]),
        'uncertainties':['Does not prove per-draw sampling or exact capture allocation.',
          'Includes day/night lantern and path/part GM references; visibility differs from allocation.',
          'Crew and sea/sky/foam/particles are not included in this bounded ship-material closure.',
          'Current installed RESOURCE used; retained trace has no resource hash manifest for these textures.',
          'RGBA8 values are logical payload, excluding Metal padding/heap alignment and CPU compressed backing.'],
        'counterargument':'4x/8x asset-byte reduction does not imply 4x/8x GPU time or 2-3x FPS: static decode is load-time, texture cache and hardware memory compression reduce live bandwidth, modern lighting/cloud/sea/shadow ALU and command overhead remain. Need matched whole-frame replay.',
        'header_owner':'experiments/native-metal/engine/src/libs/renderer/src/texture.h::TX_FILE_HEADER (<6I, 24 bytes)',
        'mip_math_owner':'experiments/native-metal/engine/src/libs/renderer/src/texture_mip_layout.hpp / texture_loader_mip_probe.py',
        'backend_owner':'experiments/native-metal/resources.hpp::Texture / Surface::upload'}
result['direct_bc_candidate_total_logical_bytes'] = result['total']['rgba8_logical_bytes'] - result['direct_bc_only']['saved_logical_bytes']
(OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'total':result['total'],'gm_only':result['gm_only'],'missing':[r['path'] for r in rows if r['status']=='missing']},indent=2))
