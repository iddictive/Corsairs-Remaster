"""Compare deformation of head edges touched by the skin-weight taper."""
import json
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2];sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk
source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
before,faces=animated_mesh((here/'cgmodelsnet-stockuv-retopology.gm').read_bytes());after,other=animated_mesh((here/'cgmodelsnet-head-weight-taper-trial.gm').read_bytes());assert faces==other
faces=np.asarray(faces,dtype=int);changed=np.array([a[1:]!=b[1:] for a,b in zip(before,after)])
affected=faces[np.any(changed[faces],axis=1)];edges=np.unique(np.sort(np.vstack((affected[:,[0,1]],affected[:,[1,2]],affected[:,[2,0]])),axis=1),axis=0)
ids=np.unique(edges);mapping=np.full(len(before),-1);mapping[ids]=np.arange(len(ids));local=mapping[edges]
b0=bind_skin(source,[before[i] for i in ids]);b1=bind_skin(source,[after[i] for i in ids]);anim=rig((root/source['source_path']).read_bytes());frames=np.unique(np.r_[np.linspace(0,anim['frames']-1,128,dtype=int),10,163,612,5320,7236])
out=[]
for frame in frames:
    p,q=fk(anim,anim['pose'](int(frame)),anim['roots'][int(frame)])
    a=np.asarray(skin_coordinates(p,q,b0));b=np.asarray(skin_coordinates(p,q,b1));la=np.linalg.norm(a[local[:,0]]-a[local[:,1]],axis=1);lb=np.linalg.norm(b[local[:,0]]-b[local[:,1]],axis=1)
    diff=lb-la;relative=lb/np.maximum(la,.001);i=int(np.argmax(diff));j=int(np.argmax(relative))
    out.append({'frame':int(frame),'max_increase':float(diff[i]),'edge':edges[i].tolist(),'max_relative':float(relative[j]),'relative_edge':edges[j].tolist(),'p99_increase':float(np.percentile(diff,99))})
print('affected faces',len(affected),'edges',len(edges),'vertices',len(ids),'frames',len(frames))
for row in sorted(out,key=lambda x:x['max_increase'],reverse=True)[:6]:print(row)
(here/'cgmodelsnet-head-taper-edge-probe.json').write_text(json.dumps({'rows':out,'faces':len(affected),'edges':len(edges)},indent=2)+'\n')
