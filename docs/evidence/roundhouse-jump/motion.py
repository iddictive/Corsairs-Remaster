from pathlib import Path
exec((Path(__file__).with_name('probe.py')).read_text().split('sheet=Image.new')[0])
from PIL import Image
name='man'
vertices,faces=r.mesh((r.INPUTS/(records[name]['model']+'.gm')).read_bytes())
anim=rig((BANKS/f'RESOURCE/animation/Roundhouse_{name}.an').read_bytes())
first,last=manifest['banks'][name]['actions']['roundhouse_jump']
guard=dict(anim,pose=lambda frame: source[name]['guard_pose'],roots=[source[name]['guard_root']])
neutral=r.deform(vertices,source[name],guard,0)
target=[(-x,y,recipe['victim_separation_m']-z) for x,y,z in neutral]
sheet=Image.new('RGB',(8*200,8*230))
for i in range(31):
    phase=i*4
    points=r.deform(vertices,source[name],anim,first+phase)
    for view,angle in enumerate((-55,65)):
        x=(i%8)*200; y=(i//8)*460+view*230
        r.panel(sheet,(x,y,200,230),points,faces,f'{phase/60:.2f}s / {angle}',azimuth=angle,target_mesh=target)
sheet.save(OUTPUT/'motion.png')
