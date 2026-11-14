"""Use one continuous body-bone field to avoid weight jumps in the head."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-stockuv-retopology.gm').read_bytes());h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3)
off=tri+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4
dt=np.dtype({'names':['p','w','b'],'formats':[('<f4',3),'<f4','<u4'],'offsets':[0,12,16],'itemsize':44});v=np.frombuffer(data,dt,size//44,off+8);p=v['p'].astype(float).copy()
bridge=np.unique(faces[-694:]);body=bridge[~((bridge>=15016)&(bridge<15623))];assert len(body)==89
a=(v['b'][body]&255).astype(int);b=((v['b'][body]>>8)&255).astype(int);w=v['w'][body].astype(float)
assert np.all((a==16)|(b==16));head=np.where(a==16,w,0)+np.where(b==16,1-w,0);bodyweight=np.clip(1-head,0,1)
angle=np.mod(np.arctan2(p[body,2],p[body,0]-.01),2*np.pi);order=np.argsort(angle);angle=angle[order];bodyweight=bodyweight[order]
keep=np.r_[True,np.diff(angle)>1e-8];angle=angle[keep];bodyweight=bodyweight[keep];assert len(angle)==88
angles=np.r_[angle[-1]-2*np.pi,angle,angle[0]+2*np.pi];weights=np.r_[bodyweight[-1],bodyweight,bodyweight[0]]
field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
ids=np.arange(2106,14993);ids=ids[(field[ids]>=-.00002)&(field[ids]<.025)]
theta=np.mod(np.arctan2(p[ids,2],p[ids,0]-.01),2*np.pi);target=np.interp(theta,angles,weights)
x=np.clip(1-field[ids]/.025,0,1);blend=.85*x*x*(3-2*x)
v['w'][ids]=target*blend;v['b'][ids]=(v['b'][ids]&0xffff0000)|np.uint32(11|(16<<8))
bypos={}
for i in range(2106,14993):
    k=tuple(np.rint(p[i]*1e6).astype(int));value=(int(v['b'][i]),float(v['w'][i]))
    if k in bypos:assert bypos[k]==value
    bypos[k]=value
for i in range(15016,15623):
    k=tuple(np.rint(p[i]*1e6).astype(int));assert k in bypos;v['b'][i],v['w'][i]=bypos[k]
print('head records',len(ids),'bridge copies',607,'boundary samples',len(angle))
(here/'cgmodelsnet-head-weight-smooth-trial.gm').write_bytes(data)
