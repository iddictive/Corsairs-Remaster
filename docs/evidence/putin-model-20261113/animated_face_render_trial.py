"""CPU skin actual GM vertices into three sampled man.an poses; no game."""
import json
import struct
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent; root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates,source_rig
from prepare_port_rowboat import rig,fk,qm,qi,rotate

source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
original=(here/'nose-subdivision-prototype.gm').read_bytes()
vertices,faces=animated_mesh(original); bound=bind_skin(source,vertices)
base_p,base_q=fk(source_rig(source),source['bind_pose'],source['bind_root'])
test=np.asarray(skin_coordinates(base_p,base_q,bound));test[:,0]*=-1
positions=np.asarray([v[0] for v in vertices]); error=np.linalg.norm(test-positions,axis=1).max()
assert error<1e-5, error
anim=rig((root/source['source_path']).read_bytes()); assert anim['parents']==source['parents']
h=struct.unpack_from('<11i7f',original)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',original,off); assert kind==4 and size==44*len(vertices)
dtype=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
raw=np.frombuffer(original,dtype=dtype,count=len(vertices),offset=off+8)
normals=raw['n'].astype(float).copy()
head=np.array([i for i,v in enumerate(vertices) if v[2]==16 and v[1]>.999 and v[0][1]>1.72 and abs(v[0][0])<.08])
assert len(head)>10
renderer=here/'render_officer.py'
print('bind recovery maximum error',error)
for label,frame in [('walk',10),('run',163),('fight-stand',8532)]:
    p,q=fk(anim,anim['pose'](frame),anim['roots'][frame])
    posed=np.asarray(skin_coordinates(p,q,bound)); posed[:,0]*=-1
    # Keep the head in the same portrait crop; do not rotate or deform it.
    posed+=positions[head].mean(axis=0)-posed[head].mean(axis=0)
    rotations=[qm(q[i],qi(base_q[i])) for i in range(len(q))]
    ns=[]
    for v,n in zip(vertices,normals):
        _,w,a,b=v
        value=w*np.asarray(rotate(rotations[a],n))+(1-w)*np.asarray(rotate(rotations[b],n))
        ns.append(value/max(np.linalg.norm(value),1e-9))
    posed_normals=np.asarray(ns); assert np.isfinite(posed).all() and np.isfinite(posed_normals).all()
    data=bytearray(original); out=np.frombuffer(data,dtype=dtype,count=len(vertices),offset=off+8)
    out['p'][:]=posed; out['n'][:]=posed_normals
    gm=here/('animated-face-'+label+'.gm');gm.write_bytes(data)
    src=renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert src.count(old)==1
        src=src.replace(old,repr('animated-face-'+label+'.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'identity-geometry-face-atlas.png'),'-.15',str(gm)]
    exec(compile(src,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
    print(label,'frame',frame,'head recentered, normals CPU skinned')
