"""Inspect actual cut boundaries in the offline two-part GM prototype."""
import struct
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
high=len(sys.argv)>1 and sys.argv[1]=='high'
data=(here/('seam-high-graft-prototype.gm' if high else 'low-seam-graft-prototype.gm')).read_bytes();h=struct.unpack_from('<11i7f',data)
tri_at=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
buff=tri_at+6*h[9];kind,size=struct.unpack_from('<2i',data,buff);assert kind==4
dt=np.dtype({'names':['p','w','bone'],'formats':[('<f4',3),'<f4','<u4'],'offsets':[0,12,16],'itemsize':44})
vertices=np.frombuffer(data,dtype=dt,count=size//44,offset=buff+8);pos=vertices['p']
# 2083 stock vertices plus 69 clipped body boundary vertices in this candidate.
split=2152
primary={}
for label,select in [('body',np.all(faces<split,axis=1)),('head',np.all(faces>=split,axis=1))]:
    subset=faces[select];edges=np.vstack((subset[:,[0,1]],subset[:,[1,2]],subset[:,[2,0]]))
    plane=1.600 if high and label=='head' else 1.595
    edges=edges[np.all(np.abs(pos[edges,1]-plane)<1e-5,axis=1)]
    keys,inv=np.unique(np.rint(pos[:,[0,2]]*1000000).astype(int),axis=0,return_inverse=True)
    key_edges=np.sort(inv[edges],axis=1);unique,counts=np.unique(key_edges,axis=0,return_counts=True)
    unique=unique[counts==1];groups=[]
    while len(unique):
        group={int(unique[0,0]),int(unique[0,1])};old=-1
        while old!=len(group):
            old=len(group);hit=np.isin(unique[:,0],list(group))|np.isin(unique[:,1],list(group));group.update(unique[hit].flatten().tolist())
        sub=unique[hit];degrees=np.bincount(sub.flatten(),minlength=len(keys));groups.append((group,sub,degrees))
        unique=unique[~hit]
    for group,sub,degrees in sorted(groups,key=lambda x:-len(x[0]))[:4]:
        xy=keys[list(group)]/1000000
        print(label,'vertices',len(group),'edges',len(sub),'degree histogram',np.unique(degrees[list(group)],return_counts=True),'bounds',np.round(xy.min(0),4),np.round(xy.max(0),4))
    if groups:
        group,sub,degrees=max(groups,key=lambda x:len(x[0]))
        assert np.all(degrees[list(group)]==2)
        used=np.unique(edges);different=0
        for key in group:
            choices=used[inv[used]==key]
            influences={(round(float(vertices['w'][i]),6),int(vertices['bone'][i])) for i in choices}
            different+=len(influences)>1
        print(label,'coincident boundary positions with differing packed weights',different)
        adjacent={i:[] for i in group}
        for a,b in sub:adjacent[int(a)].append(int(b));adjacent[int(b)].append(int(a))
        order=[next(iter(group))];previous=None
        while True:
            here_node=order[-1];nxt=next(k for k in adjacent[here_node] if k!=previous)
            if nxt==order[0]:break
            assert nxt not in order
            previous=here_node;order.append(nxt)
        assert len(order)==len(group)
        primary[label]=keys[order]/1000000
        polar=np.unwrap(np.arctan2(primary[label][:,0]-.01,primary[label][:,1]))
        steps=np.diff(polar);print(label,'polar steps positive/negative',int((steps>1e-6).sum()),int((steps<-1e-6).sum()))

def orientation(a,b,c):
    u,v=b-a,c-a
    return u[0]*v[1]-u[1]*v[0]
def intersects(a,b,c,d):
    return orientation(a,b,c)*orientation(a,b,d)<0 and orientation(c,d,a)*orientation(c,d,b)<0
body,head=primary['body'],primary['head']
crossings=sum(intersects(a,b,c,d) for a,b in zip(body,np.roll(body,-1,axis=0)) for c,d in zip(head,np.roll(head,-1,axis=0)))
print('primary body/head contour crossings',crossings)
