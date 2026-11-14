"""Fit a bounded blend of stock body motion and head motion near the cut."""
import struct
from pathlib import Path
import numpy as np
from fit_skin_trial import SkinFit
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-body-scarf-worlduv.gm').read_bytes());h=struct.unpack_from('<11i7f',data)
o=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',data,o);assert kind==4 and size%44==0
start=o+8
dt=np.dtype({'names':['p','w','b'],'formats':[('<f4',3),'<f4','<u4'],'offsets':[0,12,16],'itemsize':44})
v=np.frombuffer(data,dt,size//44,start);p=v['p'].astype(float);index=np.arange(len(v))
body=(index<2106)|((index>=14993)&(index<15016))|(index>=15623)
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
ids=np.flatnonzero(body&(field>-.05)&(field<.002)&(abs(p[:,0]-.01)<.12)&(abs(p[:,2])<.16))
fitter=SkinFit();cache={};updates=[]
for i in ids:
    row=struct.unpack_from('<3ffI3fI2f',data,start+int(i)*44)
    t=np.clip((field[i]+.05)/.05,0,1);t=.5*t*t*(3-2*t)
    key=(tuple(row[:3]),row[3],row[4],round(float(t),10))
    if key not in cache:
        target=list(row);target[3]=1.;target[4]=16|(16<<8)
        default=((row[4]&255,(row[4]>>8)&255),row[3])
        cache[key]=fitter.fit(row,target,float(t),p[i],default)
    pair,weight=cache[key];updates.append((int(i),pair,weight))
for i,pair,weight in updates:
    v['w'][i]=weight;v['b'][i]=(int(v['b'][i])&0xffff0000)|pair[0]|(pair[1]<<8)
print('selected records',len(ids),'unique fitting problems',len(cache),'maximum desired head blend',max(k[-1] for k in cache))
fitter.report()
(here/'cgmodelsnet-neck-weight-fit-trial.gm').write_bytes(data)
