"""Taper the sculpted head toward the measured shaped body cut."""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-graft-shaped-skinfit-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',data);off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8);split=2106
old=v['p'][split:].astype(float).copy();normal=v['n'][split:].astype(float).copy()
def field(p):return p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
def warp(p):
    t=np.clip(1-field(p)/.035,0,1);t=t*t*(3-2*t)
    out=p.copy();out[:,0]+=t*(.0116+.824*(p[:,0]-.0099)-p[:,0])
    out[:,2]+=t*(-.00425+1.023*(p[:,2]-.0326)-p[:,2])
    out[:,1]+=-.25*(out[:,2]-p[:,2])+.30*(abs(out[:,0]-.01)-abs(p[:,0]-.01))
    return out
new=warp(old);eps=1e-5
j=np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
nn=np.linalg.solve(np.transpose(j,(0,2,1)),normal[...,None])[...,0];nn/=np.maximum(np.linalg.norm(nn,axis=1,keepdims=True),1e-9)
assert np.isfinite(new).all() and np.isfinite(nn).all()
v['p'][split:]=new;v['n'][split:]=nn
delta=np.linalg.norm(new-old,axis=1)
print('moved vertices',int((delta>1e-7).sum()),'max displacement',float(delta.max()),'max field drift',float(abs(field(new)-field(old)).max()))
(here/'cgmodelsnet-graft-shaped-contourfit-prototype.gm').write_bytes(data)
(here/'cgmodelsnet-graft-shaped-contourfit-atlas.png').write_bytes((here/'cgmodelsnet-graft-shaped-skinfit-atlas.png').read_bytes())
path=here/'cgmodelsnet_bridge_shaped_skinfit.py';source=path.read_text()
for old,new in [('cgmodelsnet-graft-shaped-skinfit-','cgmodelsnet-graft-shaped-contourfit-'),('cgmodelsnet-bridge-shaped-skinfit','cgmodelsnet-bridge-shaped-contourfit')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
