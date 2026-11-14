"""Fit lower-head weights towards nearby body-boundary motion, preserving body."""
import struct
from pathlib import Path
import numpy as np
from fit_skin_trial import SkinFit
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-stockuv-retopology.gm').read_bytes());h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3);off=tri+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4
start=off+8;dt=np.dtype({'names':['p','w','b'],'formats':[('<f4',3),'<f4','<u4'],'offsets':[0,12,16],'itemsize':44})
v=np.frombuffer(data,dt,size//44,start);pos=v['p'].astype(float).copy();old_w=v['w'].copy();old_b=v['b'].copy()
bridge=np.unique(faces[-694:]);body=bridge[~((bridge>=15016)&(bridge<15623))];assert len(body)==89
field=pos[:,1]+.25*(pos[:,2]-.02)-.30*abs(pos[:,0]-.01)-1.595
ids=np.arange(2106,14993);ids=ids[(field[ids]>=-.00002)&(field[ids]<.025)]
nearest=np.argmin(np.sum((pos[ids,None,:]-pos[body][None,:,:])**2,axis=2),axis=1)
fitter=SkinFit();cache={};updates=[]
for i,j in zip(ids,body[nearest]):
    row=struct.unpack_from('<3ffI3fI2f',data,start+int(i)*44);other=struct.unpack_from('<3ffI3fI2f',data,start+int(j)*44)
    target=list(row);target[3]=other[3];target[4]=other[4]
    x=np.clip(1-field[i]/.025,0,1);blend=.85*x*x*(3-2*x)
    key=(tuple(row[:3]),row[3],row[4],other[3],other[4],round(float(blend),10))
    if key not in cache:cache[key]=fitter.fit(row,target,float(blend),pos[i],((row[4]&255,(row[4]>>8)&255),row[3]))
    pair,weight=cache[key];updates.append((int(i),pair,weight))
for i,pair,weight in updates:
    v['w'][i]=weight;v['b'][i]=(int(v['b'][i])&0xffff0000)|pair[0]|(pair[1]<<8)
# Copy fitted head-boundary influences to duplicate bridge UV vertices.
bypos={}
for i in range(2106,14993):
    k=tuple(np.rint(pos[i]*1e6).astype(int));value=(int(v['b'][i]),float(v['w'][i]))
    if k in bypos:assert bypos[k]==value
    bypos[k]=value
for i in range(15016,15623):
    k=tuple(np.rint(pos[i]*1e6).astype(int));assert k in bypos
    v['b'][i],v['w'][i]=bypos[k]
assert np.array_equal(v['w'][:2106],old_w[:2106]) and np.array_equal(v['b'][:2106],old_b[:2106])
print('head vertex records',len(ids),'unique fitting problems',len(cache),'bridge head copies',607)
fitter.report()
(here/'cgmodelsnet-head-weight-taper-trial.gm').write_bytes(data)
