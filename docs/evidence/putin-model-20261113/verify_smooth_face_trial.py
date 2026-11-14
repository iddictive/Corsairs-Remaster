"""Compare the modified GM with its baseline across sampled stock poses."""
import json
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk

source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
paths=[here/'photo-face-prototype.gm',here/'smooth-face-fit-prototype.gm']
a,b=[animated_mesh(p.read_bytes()) for p in paths]
assert a[1]==b[1] and [v[1:] for v in a[0]]==[v[1:] for v in b[0]]
pa=np.array([v[0] for v in a[0]]);pb=np.array([v[0] for v in b[0]])
assert np.array_equal(pa[pa[:,1]<1.6],pb[pa[:,1]<1.6])
bound=[bind_skin(source,item[0]) for item in (a,b)]
anim=rig((root/source['source_path']).read_bytes())
maximum=0.
for frame in np.linspace(0,anim['frames']-1,32,dtype=int):
    p,q=fk(anim,anim['pose'](int(frame)),anim['roots'][int(frame)])
    x=np.asarray(skin_coordinates(p,q,bound[0]));y=np.asarray(skin_coordinates(p,q,bound[1]))
    assert np.isfinite(y).all()
    maximum=max(maximum,float(np.linalg.norm(y-x,axis=1).max()))
print('same triangle indices and skin influences; neck vertices unchanged')
print('32 sampled man frames: maximum changed vertex displacement',round(maximum,7))
