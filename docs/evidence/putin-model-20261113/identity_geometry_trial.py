"""Move the stock mouth toward the generated reference before UV projection."""
import json
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
base=json.loads((here/'width-morph-baseline-landmarks.json').read_text())
target=json.loads((here/'identity-refinement-target-landmarks.json').read_text())
scale=512/Image.open(here/'identity-refinement-target.png').width
source_y=np.mean(np.asarray(base['outerLips'])[:,1])
target_y=np.mean(np.asarray(target['outerLips'])[:,1])*scale
zoom=500/.41;delta=(source_y-target_y)/zoom
assert 0<delta<.012
data=bytearray((here/'no-earring-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',data)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8)
old=v['p'].astype(float).copy();normals=v['n'].astype(float).copy()
center=1.465+(490-source_y)/zoom
def smooth(t):
    t=np.clip(t,0,1);return t*t*(3-2*t)
def warp(p):
    g=np.exp(-.5*((p[:,1]-center)/.025)**2)
    fade=smooth((p[:,2]-.025)/.055)*smooth((.10-abs(p[:,0]-.01))/.04)*smooth((p[:,1]-1.600)/.015)
    q=p.copy();q[:,1]+=delta*g*fade
    return q
new=warp(old);eps=1e-5
j=np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
n=np.linalg.solve(np.transpose(j,(0,2,1)),normals[...,None])[...,0]
n/=np.maximum(np.linalg.norm(n,axis=1,keepdims=True),1e-10)
v['p'][:]=new;v['n'][:]=n
change=np.linalg.norm(new-old,axis=1)
assert np.isfinite(new).all() and change.max()<.012
(here/'identity-geometry-base.gm').write_bytes(data)
print('mouth target shift pixels',round(source_y-target_y,3),'max movement',round(float(change.max()),6))

def run(name,args,replacements):
    path=here/name;source=path.read_text()
    for before,after,count in replacements:
        assert source.count(before)==count,before
        source=source.replace(before,after)
    sys.argv=[str(path),*map(str,args)]
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})

run('render_officer.py',['officer_5',here/'officer_5-imagegen.png',-.15,here/'identity-geometry-base.gm'],[
    ("f'{prefix}-candidate-preview.png'","'identity-geometry-baseline.png'",1),
])
needle='replacements=['
run('nose_uv_trial.py',[],[
    ('photo-feathered-target.png','identity-refinement-target.png',1),
    ('nose-face-','identity-geometry-face-',1),
    (needle,needle+"\n    ('no-earring-prototype.gm','identity-geometry-base.gm',1),\n    ('width-morph-baseline.png','identity-geometry-baseline.png',1),",1),
])
