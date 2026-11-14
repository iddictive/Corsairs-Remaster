"""Measure bridge deformation across 128 sampled frames of stock man.an."""
import json
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk

source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
verts,faces=animated_mesh((here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes())
faces=np.asarray(faces,dtype=int);bridge=faces[-628:]
assert bridge.min()>=14993 and bridge.max()<15623
indices=np.unique(bridge); mapping=np.full(len(verts),-1);mapping[indices]=np.arange(len(indices));local=mapping[bridge]
bound=bind_skin(source,[verts[i] for i in indices])
anim=rig((root/source['source_path']).read_bytes());assert anim['parents']==source['parents']
frames=np.unique(np.r_[np.linspace(0,anim['frames']-1,128,dtype=int),10,163,8532])
edges=np.unique(np.sort(np.vstack((local[:,[0,1]],local[:,[1,2]],local[:,[2,0]])),axis=1),axis=0)
rows=[]
for frame in frames:
    p,q=fk(anim,anim['pose'](int(frame)),anim['roots'][int(frame)])
    points=np.asarray(skin_coordinates(p,q,bound));assert np.isfinite(points).all()
    lengths=np.linalg.norm(points[edges[:,0]]-points[edges[:,1]],axis=1)
    face=points[local];cross=np.cross(face[:,1]-face[:,0],face[:,2]-face[:,0]);area=np.linalg.norm(cross,axis=1)
    rows.append((int(frame),float(lengths.max()),float(np.percentile(lengths,99)),float(area.min())))
order=sorted(rows,key=lambda x:x[1],reverse=True)
print('frames',len(frames),'of',anim['frames'],'bridge vertices',len(indices),'faces',len(local))
print('top 8 frames by maximum bridge edge (frame, max, p99, minimum double area):')
for row in order[:8]:print(row)
out={'source':source['source_path'],'frame_count':anim['frames'],'samples':len(frames),'rows':rows,'metric':'maximum bridge edge length in game units'}
(here/'cgmodelsnet-neck-animation-scan.json').write_text(json.dumps(out,indent=2)+'\n')
