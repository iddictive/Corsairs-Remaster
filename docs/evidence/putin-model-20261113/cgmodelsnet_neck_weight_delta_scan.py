"""Measure displaced body vertices from the candidate skin-weight change."""
import json
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk
source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
before,faces=animated_mesh((here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes())
after,other=animated_mesh((here/'cgmodelsnet-neck-weight-trial.gm').read_bytes())
assert faces==other and len(before)==len(after)
ids=[i for i,(a,b) in enumerate(zip(before,after)) if a[1:]!=b[1:]]
assert ids and all(before[i][0]==after[i][0] for i in ids)
bind0=bind_skin(source,[before[i] for i in ids]);bind1=bind_skin(source,[after[i] for i in ids])
anim=rig((root/source['source_path']).read_bytes());frames=np.unique(np.r_[np.linspace(0,anim['frames']-1,128,dtype=int),10,163,8532])
rows=[]
for frame in frames:
    p,q=fk(anim,anim['pose'](int(frame)),anim['roots'][int(frame)])
    a=np.asarray(skin_coordinates(p,q,bind0));b=np.asarray(skin_coordinates(p,q,bind1));d=np.linalg.norm(a-b,axis=1)
    rows.append((int(frame),float(d.max()),float(np.mean(d)),ids[int(np.argmax(d))]))
print('changed packed-weight rows',len(ids),'sample frames',len(frames))
for row in sorted(rows,key=lambda x:x[1],reverse=True)[:8]:print('frame, max shift, mean shift, vertex',row)
(here/'cgmodelsnet-neck-weight-delta.json').write_text(json.dumps({'rows':rows,'vertex_count':len(ids)},indent=2)+'\n')
