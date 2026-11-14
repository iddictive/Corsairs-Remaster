"""Test tapering neck-body skin weights toward the head bone at the join."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes());h=struct.unpack_from('<11i7f',data)
o=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,o);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','w','b'],'formats':[('<f4',3),'<f4','<u4'],'offsets':[0,12,16],'itemsize':44})
v=np.frombuffer(data,dt,size//44,o+8);p=v['p'].astype(float);index=np.arange(len(v))
body=(index<2106)|((index>=14993)&(index<15016))|(index>=15623)
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
select=body&(field>-.05)&(field<.002)&(abs(p[:,0]-.01)<.12)&(abs(p[:,2])<.16)
ids=np.flatnonzero(select);a=(v['b'][ids]&255).astype(int);b=((v['b'][ids]>>8)&255).astype(int);w=v['w'][ids].astype(float)
assert np.all((w>=0)&(w<=1))
old_head=np.where(a==16,w,0)+np.where(b==16,1-w,0)
dominant=np.where(a==16,b,np.where(b==16,a,np.where(w>=.5,a,b)))
t=np.clip((field[ids]+.05)/.05,0,1);t=t*t*(3-2*t)
head=old_head+(1-old_head)*t
v['w'][ids]=1-head
v['b'][ids]=(v['b'][ids]&0xffff0000)|dominant.astype('uint32')|np.uint32(16<<8)
print('changed body/clone vertices',len(ids),'full head influence',int((head>.999).sum()),'largest new head influence',float(np.max(head-old_head)))
(here/'cgmodelsnet-neck-weight-trial.gm').write_bytes(data)
