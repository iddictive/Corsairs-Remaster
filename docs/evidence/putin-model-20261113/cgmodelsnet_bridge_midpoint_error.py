"""Measure new bridge vertices against their original edge midpoints."""
import json
import sys
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk
source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
verts,_=animated_mesh((here/'cgmodelsnet-bridge-midpoint-trial.gm').read_bytes())
mapping=json.loads((here/'cgmodelsnet-bridge-midpoint-map.json').read_text())
allids=np.unique(np.asarray([e['edge']+[e['midpoint']] for e in mapping],dtype=int));reverse=np.full(len(verts),-1);reverse[allids]=np.arange(len(allids))
triples=reverse[np.asarray([e['edge']+[e['midpoint']] for e in mapping],dtype=int)]
bound=bind_skin(source,[verts[i] for i in allids]);anim=rig((root/source['source_path']).read_bytes())
frames=np.unique(np.r_[np.linspace(0,anim['frames']-1,128,dtype=int),10,163,8532]);rows=[]
for frame in frames:
    p,q=fk(anim,anim['pose'](int(frame)),anim['roots'][int(frame)]);pts=np.asarray(skin_coordinates(p,q,bound))
    error=np.linalg.norm(pts[triples[:,2]]-(pts[triples[:,0]]+pts[triples[:,1]])*.5,axis=1)
    rows.append((int(frame),float(error.max()),float(error.mean())))
print('midpoints',len(mapping),'sample frames',len(frames))
for row in sorted(rows,key=lambda x:x[1],reverse=True)[:8]:print('frame, max deviation, mean deviation',row)
(here/'cgmodelsnet-bridge-midpoint-error.json').write_text(json.dumps({'rows':rows,'units':'game units'},indent=2)+'\n')
