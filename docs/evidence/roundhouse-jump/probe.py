import sys, json, math, hashlib, importlib.util
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[3]
BANKS=Path(sys.argv[1])
OUTPUT=Path(sys.argv[2])
sys.path.insert(0,str(ROOT/'tools'))
import prepare_roundhouse as p
from prepare_port_rowboat import rig, fk, rotate, add
spec=importlib.util.spec_from_file_location('render', ROOT/'docs/evidence/roundhouse-kick/render.py')
r=importlib.util.module_from_spec(spec); spec.loader.exec_module(r)
source=json.loads(p.SOURCES.read_text())['rigs']
manifest=json.loads(p.MANIFEST.read_text())
recipe=json.loads(p.RECIPE.read_text())
records=json.loads((r.INPUTS/'meshes.json').read_text())
sheet=Image.new('RGB',(7*245,6*290))
rows={}
for row,name in enumerate(r.MODELS):
    vertices,faces=r.mesh((r.INPUTS/(records[name]['model']+'.gm')).read_bytes())
    anim=rig((BANKS/f'RESOURCE/animation/Roundhouse_{name}.an').read_bytes())
    first,last=manifest['banks'][name]['actions']['roundhouse_jump']
    ground,_=manifest['banks'][name]['actions']['roundhouse_kick']
    head=manifest['checks'][name]['heel_target_xyz']
    body=source[name]['body']['joints']; offset=manifest['banks'][name]['heel_offset']
    guard=dict(anim,pose=lambda frame: source[name]['guard_pose'],roots=[source[name]['guard_root']])
    neutral=r.deform(vertices,source[name],guard,0)
    target=[(-x,y,recipe['victim_separation_m']-z) for x,y,z in neutral]
    minimum=[]
    for col,phase in enumerate((0,20,38,52,68,85,120)):
        points=r.deform(vertices,source[name],anim,first+phase)
        minimum.append(round(min(x[1] for x in points),3))
        r.panel(sheet,(col*245,row*290,245,290),points,faces,f'{name}: {phase/120*recipe["jump_choreography"]["duration_s"]:.2f}s',head if phase==68 else None,target_mesh=target,view_height=3.1)
    pos,rot=fk(anim,anim['pose'](first+68),anim['roots'][first+68])
    heel=add(pos[body[14]],rotate(rot[body[14]],offset))
    rows[name]={'contact_error_m':math.dist(heel,head),'mesh_floor_y':minimum,'root_lift_at_contact':anim['roots'][first+68][1]-anim['roots'][ground+68][1]}
sheet.save(OUTPUT/'phases.png')
print(json.dumps(rows,indent=2))
