"""Compose the shipped Fort Orange port, a complete frigate, sails, and sea.

Reads source assets only. Output is the flat Metal probe scene format.
"""
import json, math, struct, sys, tempfile
from pathlib import Path

from prepare import prepare


def transform_vertex(v, m):
    x,y,z,nx,ny,nz,u,w=v
    return (x*m[0]+y*m[4]+z*m[8]+m[12],
            x*m[1]+y*m[5]+z*m[9]+m[13],
            x*m[2]+y*m[6]+z*m[10]+m[14],
            nx*m[0]+ny*m[4]+nz*m[8],
            nx*m[1]+ny*m[5]+nz*m[9],
            nx*m[2]+ny*m[6]+nz*m[10],u,w)


def multiply(a,b):
    return [sum(a[r+k*4]*b[k+c*4] for k in range(4)) for c in range(4) for r in range(4)]


def translation(x,y,z):
    return [1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1]


def compose(game_root, output):
    model_root=game_root/'RESOURCE/MODELS'
    texture_root=game_root/'RESOURCE/Textures'
    packed=bytearray(); draws=[]; frame_points=[]; provenance=[]; sail_points=[]

    def add_model(path, matrix, role, frame=True, recurse=False, family=None):
        if not path.is_file(): raise ValueError(f'Missing required {role}: {path}')
        with tempfile.TemporaryDirectory(prefix='metal-probe-') as tmp:
            tmp=Path(tmp); scene=prepare(path,texture_root,tmp)
            raw=(tmp/'vertices.bin').read_bytes()
        base=len(packed)//32
        transformed=[]
        for v in struct.iter_unpack('<8f',raw):
            q=transform_vertex(v,matrix); transformed.append(q); packed.extend(struct.pack('<8f',*q))
            if frame: frame_points.append(q[:3])
        for d in scene['draws']:
            draws.append({'start':base+d['start'],'count':d['count'],'texture':d['texture'],'kind':0})
        provenance.append({'role':role,'model':str(path)})
        labels=scene['labels']
        if role.startswith('mast'):
            groups={x['group'].lower() for x in labels if x['group'].lower().startswith('sail')}
            for group in groups:
                pts={x['name'].lower():transform_vertex((0,0,0,0,1,0,0,0),multiply(matrix,x['matrix']))[:3]
                     for x in labels if x['group'].lower()==group and x['name'].lower().startswith('sail')}
                ordered=[pts[k] for k in ('sail1','sail2','sail3','sail4') if k in pts]
                if len(ordered)>=3: sail_points.append(ordered)
        if recurse:
            stem=family or path.stem
            for label in labels:
                if label['group'].lower()!='geometry': continue
                child=path.with_name(f'{stem}_{label["name"]}.gm')
                if not child.is_file():
                    # Some exporters vary only the case of the suffix.
                    matches=[p for p in path.parent.glob(f'{stem}_*.gm') if p.stem.lower()==f'{stem}_{label["name"]}'.lower()]
                    if not matches: raise ValueError(f'Missing geometry child {label["name"]} for {path.name}')
                    child=matches[0]
                add_model(child,multiply(matrix,label['matrix']),f'{role}/{label["name"]}',frame,
                          recurse=label['name'].lower().startswith('mast'),family=stem)
        return labels

    port=model_root/'Locations/Town_FortOrange/Port'
    identity=translation(0,0,0)
    for filename,role in [('FortOrangeport.gm','port'),('FortOrangeport_sb.gm','seabed'),
                          ('FortOrangeport_e1.gm','day details'),('FortOrangeport_fd.gm','day lanterns'),
                          ('FortOrangeport_j.gm','jungle backdrop')]:
        add_model(port/filename,identity,role)
    with tempfile.TemporaryDirectory(prefix='metal-probe-locators-') as locator_tmp:
        loc_scene=prepare(port/'FortOrangeport_l.gm',texture_root,Path(locator_tmp))
    ship_locator=next(x for x in loc_scene['labels'] if x['group'].lower()=='ships' and x['name'].lower()=='ship_1')
    ship_matrix=ship_locator['matrix']
    ship=model_root/'Ships/FrigateQueen1/FrigateQueen1.gm'
    add_model(ship,ship_matrix,'FrigateQueen1',recurse=True,family='FrigateQueen1')

    # Static sail approximation from each sail group's authored locator polygon.
    for points in sail_points:
        start=len(packed)//32
        tris=[(0,i,i+1) for i in range(1,len(points)-1)]
        for tri in tris:
            for i in tri:
                x,y,z=points[i]; packed.extend(struct.pack('<8f',x,y,z,1,0,0,0,0))
        draws.append({'start':start,'count':len(tris)*3,'texture':None,'kind':0})

    # Finite procedural sea; deliberately excluded from camera framing.
    start=len(packed)//32; y=-0.12; lo=-260.0; hi=330.0
    sea=[(lo,y,lo),(hi,y,lo),(hi,y,hi),(lo,y,lo),(hi,y,hi),(lo,y,hi)]
    for i,(x,yy,z) in enumerate(sea): packed.extend(struct.pack('<8f',x,yy,z,0,1,0,(x-lo)/(hi-lo),(z-lo)/(hi-lo)))
    draws.append({'start':start,'count':6,'texture':None,'kind':1})

    if not frame_points: raise ValueError('Empty harbor scene')
    low=[min(p[i] for p in frame_points) for i in range(3)]; high=[max(p[i] for p in frame_points) for i in range(3)]
    center=[(a+b)/2 for a,b in zip(low,high)]
    extent=max(high[0]-low[0],high[2]-low[2],(high[1]-low[1])*1.6)
    output.mkdir(parents=True,exist_ok=True)
    (output/'vertices.bin').write_bytes(packed)
    scene={'title':'Fort Orange Harbor — FrigateQueen1','harbor':True,'center':center,'scale':2/extent,
           'yaw':-0.45,'pitch':0.28,'distance':3.2,'draws':draws,'bounds':[low,high],
           'ship_placement':{'locator':'ships/ship_1','matrix':ship_matrix},'sources':provenance,
           'sails':'Static approximation generated from authored sail-group locators'}
    (output/'scene.json').write_text(json.dumps(scene,indent=2))
    print(f'harbor: {len(packed)//96} triangles, {len(draws)} draws, {len(sail_points)} sails -> {output}')


if __name__=='__main__':
    try: compose(Path(sys.argv[1]),Path(sys.argv[2]))
    except (ValueError,OSError,struct.error,IndexError,StopIteration) as e: sys.exit(str(e))
