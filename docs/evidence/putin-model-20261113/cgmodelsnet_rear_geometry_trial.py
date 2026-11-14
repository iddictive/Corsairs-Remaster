"""Test a localized inward taper of the retained sculpture's rear collar."""
import argparse
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('strength',choices=['3','6']);args=parser.parse_args();amount=int(args.strength)/1000
data=bytearray((here/'cgmodelsnet-stockuv-retopology.gm').read_bytes());h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8];faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3).copy();off=tri+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44});v=np.frombuffer(data,dt,size//44,off+8)
pos=v['p'].astype(float).copy();norm=v['n'].astype(float).copy();ids=np.r_[np.arange(2106,14993),np.arange(15016,15623)]
def smooth(x):
    x=np.clip(x,0,1);return x*x*(3-2*x)
def warp(p):
    result=p.copy();f=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
    fade=smooth((.055-f)/.025)*smooth((.025-p[:,2])/.04)
    d=p[:,[0,2]]-np.array([.01,0]);r=np.maximum(np.linalg.norm(d,axis=1),1e-9);newr=r-amount*fade
    assert np.all(newr>0)
    result[:,0]=.01+d[:,0]*newr/r;result[:,2]=d[:,1]*newr/r
    result[:,1]+=-.25*(result[:,2]-p[:,2])+.30*(abs(result[:,0]-.01)-abs(p[:,0]-.01))
    return result
old=pos[ids];new=warp(old);eps=1e-5
j=np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
nn=np.linalg.solve(np.transpose(j,(0,2,1)),norm[ids,...,None])[...,0];nn/=np.maximum(np.linalg.norm(nn,axis=1,keepdims=True),1e-9)
assert np.isfinite(nn).all();v['p'][ids]=new;v['n'][ids]=nn
after=v['p'].astype(float);delta=np.linalg.norm(after-pos,axis=1)
before=np.cross(pos[faces[:,1]]-pos[faces[:,0]],pos[faces[:,2]]-pos[faces[:,0]]);post=np.cross(after[faces[:,1]]-after[faces[:,0]],after[faces[:,2]]-after[faces[:,0]])
den=np.linalg.norm(before,axis=1)*np.linalg.norm(post,axis=1);dot=np.sum(before*post,axis=1)/np.maximum(den,1e-20)
changed=np.any(delta[faces]>1e-8,axis=1)&(den>1e-18)
print('strength',amount,'moved vertices',int((delta>1e-8).sum()),'maximum displacement',float(delta.max()),'affected faces',int(changed.sum()),'rotated over 60',int((dot[changed]<.5).sum()),'over 90',int((dot[changed]<0).sum()))
assert not np.any(dot[changed]<0)
(here/f'cgmodelsnet-rear-geometry-{args.strength}.gm').write_bytes(data)
