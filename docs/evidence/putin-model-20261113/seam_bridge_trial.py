"""Experimental ruled bridge between the actual stock and graft boundaries."""
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
inset=len(sys.argv)>1 and sys.argv[1]=='inset'
input_prefix='seam-inset' if inset else 'seam-high'
output_prefix='inset-bridge' if inset else 'seam-bridge'
data=(here/f'{input_prefix}-graft-prototype.gm').read_bytes();h=list(struct.unpack_from('<11i7f',data))
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
tri_at=obj+104*h[8];buff=tri_at+6*h[9]
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3).copy()
kind,size=struct.unpack_from('<2i',data,buff);assert kind==4 and size%44==0
start=buff+8;n=size//44
rows=[data[start+44*i:start+44*(i+1)] for i in range(n)]
v=[struct.unpack('<3ffI3fI2f',r) for r in rows]
pos=np.array([r[:3] for r in v]);split=2152

def loop(select,plane):
    f=faces[select];edges=np.vstack((f[:,[0,1]],f[:,[1,2]],f[:,[2,0]]))
    edges=edges[np.all(abs(pos[edges,1]-plane)<1e-5,axis=1)]
    keys,inv=np.unique(np.rint(pos[:,[0,2]]*1000000).astype(int),axis=0,return_inverse=True)
    pair=np.sort(inv[edges],axis=1);uniq,counts=np.unique(pair,axis=0,return_counts=True);boundary=uniq[counts==1]
    groups=[]
    while len(boundary):
        group={int(boundary[0,0]),int(boundary[0,1])};old=-1
        while old!=len(group):
            old=len(group);hit=np.isin(boundary[:,0],list(group))|np.isin(boundary[:,1],list(group));group.update(boundary[hit].flatten().tolist())
        groups.append((group,boundary[hit]));boundary=boundary[~hit]
    group,edges=max(groups,key=lambda g:len(g[0]));adj={i:[] for i in group}
    for a,b in edges:adj[int(a)].append(int(b));adj[int(b)].append(int(a))
    assert all(len(x)==2 for x in adj.values())
    order=[next(iter(group))];prev=None
    while True:
        cur=order[-1];nxt=next(x for x in adj[cur] if x!=prev)
        if nxt==order[0]:break
        assert nxt not in order
        prev=cur;order.append(nxt)
    assert len(order)==len(group)
    choices=np.unique(f)
    result=[int(choices[np.flatnonzero(inv[choices]==k)[0]]) for k in order]
    return np.asarray(result,dtype=int)

body=loop(np.all(faces<split,axis=1),1.595)
head=loop(np.all(faces>=split,axis=1),1.600)
def area(ids):
    x,z=pos[ids,0],pos[ids,2]
    return np.sum(x*np.roll(z,-1)-z*np.roll(x,-1))
if area(body)*area(head)<0:head=head[::-1]
distance=np.linalg.norm(pos[body,None,:]-pos[head][None,:,:],axis=2)
i,j=np.unravel_index(np.argmin(distance),distance.shape)
body=np.roll(body,-i);head=np.roll(head,-j)
body=np.r_[body,body[0]];head=np.r_[head,head[0]]
m,k=len(body)-1,len(head)-1
# Dynamic program triangulates the two ordered loops while penalizing long edges.
cost=np.full((m+1,k+1),np.inf);choice=np.zeros((m+1,k+1),dtype=np.uint8);cost[0,0]=0
for i in range(m+1):
    for j in range(k+1):
        if i<m:
            score=cost[i,j]+np.sum((pos[body[i+1]]-pos[head[j]])**2)
            if score<cost[i+1,j]:cost[i+1,j]=score;choice[i+1,j]=1
        if j<k:
            score=cost[i,j]+np.sum((pos[body[i]]-pos[head[j+1]])**2)
            if score<cost[i,j+1]:cost[i,j+1]=score;choice[i,j+1]=2
steps=[];i,j=m,k
while i or j:
    step=choice[i,j];assert step in (1,2);steps.append(step)
    if step==1:i-=1
    else:j-=1
steps.reverse()

atlas=np.asarray(Image.open(here/f'{input_prefix}-graft-atlas.png').convert('RGB')).copy()
def samples(ids):
    uv=np.array([v[x][9:11] for x in ids]);x=np.minimum((uv[:,0]%1*2048).astype(int),2047);y=np.minimum((uv[:,1]%1*2048).astype(int),2047)
    return atlas[y,x].astype(float)
body_color=samples(body);head_color=samples(head)
x=np.linspace(0,1,1024)
def ramp(c):return np.column_stack([np.interp(x,np.linspace(0,1,len(c)),c[:,i]) for i in range(3)])
bc,hc=ramp(body_color),ramp(head_color)
for y in range(128):
    blend=y/127
    atlas[y,1024:2048]=np.clip((1-blend)*hc+blend*bc,0,255).astype('uint8')
Image.fromarray(atlas).save(here/f'{output_prefix}-atlas.png')
def clones(ids,tex_y):
    out=[]
    for i,idx in enumerate(ids):
        row=bytearray(rows[idx]);u=.5+.5*i/(len(ids)-1)
        struct.pack_into('<2f',row,36,min(u,.999999),tex_y)
        out.append(len(rows));rows.append(row)
    return out
bc=clones(body,.062);hc=clones(head,.001)
bridge=[];i=j=0
for step in steps:
    if step==1:bridge.append((bc[i],bc[i+1],hc[j]));i+=1
    else:bridge.append((bc[i],hc[j+1],hc[j]));j+=1
assert i==m and j==k and len(rows)<65536
newfaces=np.vstack((faces,np.asarray(bridge,dtype='<u2'))).astype('<u2')
prefix=bytearray(data[:tri_at]);h[9]=len(newfaces);struct.pack_into('<11i7f',prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(newfaces);fields[10]=len(rows)
struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
(here/f'{output_prefix}-prototype.gm').write_bytes(prefix+newfaces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+data[start+size:])
print('body loop',m,'head loop',k,'bridge triangles',len(bridge),'vertices',len(rows),'faces',len(newfaces),'min starting separation',round(float(distance.min()),5))
