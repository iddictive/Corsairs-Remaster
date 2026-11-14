"""Compare source and UV-split GM in stock bind/guard poses, offline."""
import json
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import animated_mesh,bind_skin,skin_coordinates,source_rig
from prepare_port_rowboat import fk

before,after=[animated_mesh(Path(x).read_bytes()) for x in sys.argv[1:3]]
va,fa=before; vb,fb=after
assert len(fa)==len(fb)
for a,b in zip(fa,fb):
    for i,j in zip(a,b):assert va[i]==vb[j]
source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
rig=source_rig(source); ba=bind_skin(source,va); bb=bind_skin(source,vb)
for name,pose,origin in (('bind',source['bind_pose'],source['bind_root']),('guard',source['guard_pose'],source['guard_root'])):
    p,q=fk(rig,pose,origin); a=np.asarray(skin_coordinates(p,q,ba)); b=np.asarray(skin_coordinates(p,q,bb))
    d=max(float(np.linalg.norm(a[i]-b[j])) for facea,faceb in zip(fa,fb) for i,j in zip(facea,faceb))
    assert d<1e-7,(name,d)
    print(name,'max positional difference',d)
print('preserved per-corner positions, bones, weights and both sampled poses')
