"""Approximate intermediate-frame bridge behavior; no engine/game process."""
import hashlib
import json
import math
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2];sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk

gm=(root/'src/assets/characters/putin-phonk-recut-candidate/Putin.gm').read_bytes()
assert hashlib.sha256(gm).hexdigest()=='91ebfa4ede221009948c492c70f26786ea346cc167f856033ae36ceb07547f47'
base=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
verts,faces=animated_mesh(gm);bridge=np.asarray(faces,dtype=int)[-344:];indices=np.unique(bridge)
mapping=np.full(len(verts),-1);mapping[indices]=np.arange(len(indices));local=mapping[bridge]
edges=np.unique(np.sort(np.vstack((local[:,[0,1]],local[:,[1,2]],local[:,[2,0]])),axis=1),axis=0)
head=(indices[edges]>=16012)&(indices[edges]!=16270);edges=edges[np.sum(head,axis=1)==1];assert len(edges)==346
def slerp(a,b,t):
    a=np.array(a,dtype=float);b=np.array(b,dtype=float);dot=float(a@b);k=-1 if dot<0 else 1;d=min(abs(dot),1.)
    if 1-d>.1:
        omega=math.acos(d);s=math.sin(omega);u=math.sin(omega*(1-t))/s;v=math.sin(omega*t)/s
    else:u=1-t;v=t
    q=a*(u*k)+b*v
    # fk/rotate assumes unit quaternions, unlike the engine's direct matrix path.
    q/=np.linalg.norm(q)
    return tuple(q)
for name,path,expected,extra in [('man',root/base['source_path'],base['source_sha256'],[249,402,5320,6206,7236]),('mushketer',here/'mushketer-source.an','9651c14e7b3acb7e8e5f541c5696534a6782295b00dd2994b88c02240ab65788',[174,207,481,976])]:
    raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==expected;anim=rig(raw);assert anim['parents']==base['parents']
    source=dict(base);source.update(rest=anim['rest'],bind_pose=anim['pose'](0),bind_root=anim['roots'][0]);bound=bind_skin(source,[verts[i] for i in indices])
    frames=np.unique(np.r_[np.linspace(0,anim['frames']-2,128,dtype=int),extra]);out=[]
    for f in frames:
        q0=anim['pose'](int(f));q1=anim['pose'](int(f)+1);r0=np.array(anim['roots'][f]);r1=np.array(anim['roots'][f+1])
        for alpha in (.25,.5,.75):
            qs=[slerp(a,b,alpha) for a,b in zip(q0,q1)];p,q=fk(anim,qs,tuple(r0*(1-alpha)+r1*alpha));points=np.asarray(skin_coordinates(p,q,bound))
            lengths=np.linalg.norm(points[edges[:,0]]-points[edges[:,1]],axis=1);out.append((int(f),alpha,float(lengths.max())))
    out.sort(key=lambda x:x[2],reverse=True)
    print(name,'intervals',len(frames),'samples',len(out),'worst',out[:5],flush=True)
    (here/f'cgmodelsnet-recut-interpolation-{name}.json').write_text(json.dumps({'model_sha256':hashlib.sha256(gm).hexdigest(),'bank_sha256':expected,'results':out},indent=2)+'\n')
