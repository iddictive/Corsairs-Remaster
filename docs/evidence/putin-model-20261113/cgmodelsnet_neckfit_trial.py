"""Try tapered neck contour alignment; preserves stock body and head skinning."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-graft-clipped-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',data);off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8);split=2132
old=v['p'][split:].astype(float).copy();norm=v['n'][split:].astype(float).copy()
def warp(p):
    t=np.clip((1.625-p[:,1])/(1.625-1.59349),0,1);t=t*t*(3-2*t)
    out=p.copy();out[:,0]+=t*(.0037+.73*(p[:,0]-.0103)-p[:,0])
    out[:,2]+=t*(-.01535+1.30*(p[:,2]-.02515)-p[:,2])
    return out
new=warp(old);eps=1e-5
j=np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
newn=np.linalg.solve(np.transpose(j,(0,2,1)),norm[...,None])[...,0];newn/=np.maximum(np.linalg.norm(newn,axis=1,keepdims=True),1e-9)
assert np.isfinite(new).all() and np.isfinite(newn).all()
v['p'][split:]=new;v['n'][split:]=newn
(here/'cgmodelsnet-neckfit-input.gm').write_bytes(data)
print('moved head vertices',int((np.linalg.norm(new-old,axis=1)>1e-7).sum()),'max',float(np.linalg.norm(new-old,axis=1).max()))
path=here/'cgmodelsnet_bridge_trial.py';source=path.read_text()
for before,after in [('cgmodelsnet-graft-clipped-prototype.gm','cgmodelsnet-neckfit-input.gm'),('cgmodelsnet-bridge','cgmodelsnet-neckfit')]:
    assert source.count(before)>0,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
