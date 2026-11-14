"""Map the star-shaped head cut inside the actual stock-body boundary."""
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-graft-shaped-skinfit-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',data);tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3).copy();off=tri+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=size//44,offset=off+8);pos=v['p'].astype(float).copy();normal=v['n'].astype(float).copy();split=2106
def residual(p):return p[...,1]+.25*(p[...,2]-.02)-.30*abs(p[...,0]-.01)-1.595
def boundary(selection):
    subset=faces[selection];edges=np.vstack((subset[:,[0,1]],subset[:,[1,2]],subset[:,[2,0]]));edges=edges[np.all(abs(residual(pos[edges]))<1e-5,axis=1)]
    keys,inv=np.unique(np.rint(pos[:,[0,2]]*1000000).astype(int),axis=0,return_inverse=True)
    pairs=np.sort(inv[edges],axis=1);unique,counts=np.unique(pairs,axis=0,return_counts=True);pairs=unique[counts==1]
    adjacency={}
    for a,b in pairs:adjacency.setdefault(int(a),[]).append(int(b));adjacency.setdefault(int(b),[]).append(int(a))
    assert adjacency and all(len(a)==2 for a in adjacency.values())
    first=next(iter(adjacency));order=[first];previous=None
    while True:
        current=order[-1];nxt=next(x for x in adjacency[current] if x!=previous)
        if nxt==first:break
        assert nxt not in order;previous=current;order.append(nxt)
    assert len(order)==len(adjacency)
    return keys[order].astype(float)/1000000
body=boundary(np.all(faces<split,axis=1));head=boundary(np.all(faces>=split,axis=1))
center=np.array([.01,0.]);b0=body-center;b1=np.roll(body,-1,axis=0)-center;edge=b1-b0
def cross(a,b):return a[...,0]*b[...,1]-a[...,1]*b[...,0]
def body_radius(theta):
    direction=np.column_stack((np.cos(theta),np.sin(theta)))
    den=cross(direction[:,None,:],edge[None,:,:]);safe=np.where(abs(den)>1e-12,den,1.)
    r=cross(b0,edge)[None,:]/safe
    t=cross(b0[None,:,:],direction[:,None,:])/safe
    valid=(abs(den)>1e-12)&(r>0)&(t>=-1e-7)&(t<=1+1e-7)
    assert np.all(valid.sum(axis=1)==1),np.unique(valid.sum(axis=1),return_counts=True)
    return np.sum(np.where(valid,r,0),axis=1)
delta=head-center;angles=np.mod(np.arctan2(delta[:,1],delta[:,0]),2*np.pi);order=np.argsort(angles)
angles=angles[order];radii=np.linalg.norm(delta[order],axis=1)
assert np.min(np.diff(angles))>1e-9
ext_angles=np.r_[angles[-1]-2*np.pi,angles,angles[0]+2*np.pi];ext_radii=np.r_[radii[-1],radii,radii[0]]
def warp(points):
    out=points.copy();height=residual(points);selected=(height>=-1e-5)&(height<.035)
    p=points[selected];d=p[:,[0,2]]-center;angle=np.mod(np.arctan2(d[:,1],d[:,0]),2*np.pi)
    length=np.maximum(np.linalg.norm(d,axis=1),1e-9);old_r=np.interp(angle,ext_angles,ext_radii);target=body_radius(angle)-.002
    t=np.clip(1-height[selected]/.035,0,1);t=t*t*(3-2*t)
    next_r=length+t*(target-old_r);assert np.all(next_r>0)
    out[selected,0]=center[0]+d[:,0]*next_r/length;out[selected,2]=center[1]+d[:,1]*next_r/length
    out[selected,1]+=-.25*(out[selected,2]-p[:,2])+.30*(abs(out[selected,0]-.01)-abs(p[:,0]-.01))
    return out
old=pos[split:];new=warp(old);eps=1e-5
j=np.stack([(warp(old+np.eye(3)[i]*eps)-warp(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
nn=np.linalg.solve(np.transpose(j,(0,2,1)),normal[split:,...,None])[...,0];nn/=np.maximum(np.linalg.norm(nn,axis=1,keepdims=True),1e-9)
assert np.isfinite(new).all() and np.isfinite(nn).all()
v['p'][split:]=new;v['n'][split:]=nn
print('body/head boundary vertices',len(body),len(head),'moved',int((np.linalg.norm(new-old,axis=1)>1e-7).sum()),'max displacement',float(np.linalg.norm(new-old,axis=1).max()))
(here/'cgmodelsnet-graft-shaped-radialfit-prototype.gm').write_bytes(data)
(here/'cgmodelsnet-graft-shaped-radialfit-atlas.png').write_bytes((here/'cgmodelsnet-graft-shaped-skinfit-atlas.png').read_bytes())
path=here/'cgmodelsnet_bridge_shaped_skinfit.py';source=path.read_text()
for old,new in [('cgmodelsnet-graft-shaped-skinfit-','cgmodelsnet-graft-shaped-radialfit-'),('cgmodelsnet-bridge-shaped-skinfit','cgmodelsnet-bridge-shaped-radialfit')]:
    assert source.count(old)==1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
