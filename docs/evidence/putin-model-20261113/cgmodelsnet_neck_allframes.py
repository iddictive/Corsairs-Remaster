"""Scan every discrete frame of one stock bank for candidate neck-edge lengths."""
import argparse
import hashlib
import json
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk

parser=argparse.ArgumentParser();parser.add_argument('bank',choices=['man','mushketer']);args=parser.parse_args()
source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
bank=(root/source['source_path']) if args.bank=='man' else (here/'mushketer-source.an')
raw=bank.read_bytes();bank_hash=hashlib.sha256(raw).hexdigest()
expected=source['source_sha256'] if args.bank=='man' else '9651c14e7b3acb7e8e5f541c5696534a6782295b00dd2994b88c02240ab65788'
assert bank_hash==expected
anim=rig(raw);assert anim['parents']==source['parents']
source=dict(source);source.update(rest=anim['rest'],bind_pose=anim['pose'](0),bind_root=anim['roots'][0])
gm=(root/'src/assets/characters/putin-phonk-candidate/Putin.gm').read_bytes();gm_hash=hashlib.sha256(gm).hexdigest()
assert gm_hash=='59f2d10900c0352f8f03f2d56b679208b816c10a1bdd3ddba11cfe66a6c72cd6'
verts,faces=animated_mesh(gm);bridge=np.asarray(faces,dtype=int)[-694:];indices=np.unique(bridge)
mapping=np.full(len(verts),-1);mapping[indices]=np.arange(len(indices));local=mapping[bridge]
edges=np.unique(np.sort(np.vstack((local[:,[0,1]],local[:,[1,2]],local[:,[2,0]])),axis=1),axis=0)
head=(indices[edges]>=15016)&(indices[edges]<15623);edges=edges[np.sum(head,axis=1)==1];assert len(edges)==695
bound=bind_skin(source,[verts[i] for i in indices]);rows=[]
for frame in range(anim['frames']):
    p,q=fk(anim,anim['pose'](frame),anim['roots'][frame]);points=np.asarray(skin_coordinates(p,q,bound));assert np.isfinite(points).all()
    lengths=np.linalg.norm(points[edges[:,0]]-points[edges[:,1]],axis=1)
    triangles=points[local];area=np.linalg.norm(np.cross(triangles[:,1]-triangles[:,0],triangles[:,2]-triangles[:,0]),axis=1)
    rows.append((frame,float(lengths.max()),float(np.percentile(lengths,99)),float(area.min())))
    if frame and frame%2000==0:print(args.bank,'frames processed',frame,flush=True)
worst=sorted(rows,key=lambda r:r[1],reverse=True)
out={'bank':args.bank,'bank_sha256':bank_hash,'candidate_sha256':gm_hash,'frame_count':anim['frames'],'edges':len(edges),'metric':'maximum body-to-head edge length in game units; discrete frames only','rows':rows}
(here/f'cgmodelsnet-neck-allframes-{args.bank}.json').write_text(json.dumps(out,separators=(',',':'))+'\n')
print('completed',args.bank,'frames',len(rows),'top',worst[:5],flush=True)
