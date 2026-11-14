"""Compare stock-body and sculpture boundary positions at the same height."""
import struct
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
from clip_body_trial import clip_body

here=Path(__file__).resolve().parent
data=(here/'no-earring-prototype.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
obj=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]
tri_at=obj+104*h[8];buff=tri_at+6*h[9];kind,size=struct.unpack_from('<2i',data,buff)
assert kind==4 and size%44==0
n=size//44;start=buff+8
rows=[data[start+44*i:start+44*(i+1)] for i in range(n)]
points=np.array([struct.unpack_from('<3f',row) for row in rows])
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
rows,clipped=clip_body(rows,faces,points,1.595)
body=np.array([struct.unpack_from('<3f',row) for row in rows[n:]])
allpos=np.array([struct.unpack_from('<3f',row) for row in rows])
edges=np.vstack((clipped[:,[0,1]],clipped[:,[1,2]],clipped[:,[2,0]]))
edges=edges[np.all(abs(allpos[edges,1]-1.595)<1e-5,axis=1)]
# Join positional duplicates caused by stock UV seams for contour inspection.
keys,inv=np.unique(np.rint(allpos[:,[0,2]]*100000).astype(int),axis=0,return_inverse=True)
edgekeys=np.sort(inv[edges],axis=1);unique,degree=np.unique(edgekeys,axis=0,return_counts=True)
boundary=unique[degree==1];groups=[]
while len(boundary):
    group={int(boundary[0,0]),int(boundary[0,1])};old=-1
    while old!=len(group):
        old=len(group);hit=np.isin(boundary[:,0],list(group))|np.isin(boundary[:,1],list(group))
        group.update(boundary[hit].flatten().tolist())
    groups.append(group);boundary=boundary[~hit]
print('stock contour components',[(len(g),np.round(keys[list(g)].min(axis=0)/100000,4).tolist(),np.round(keys[list(g)].max(axis=0)/100000,4).tolist()) for g in sorted(groups,key=len,reverse=True)])
for group in sorted(groups,key=len,reverse=True):
    selected=[i for i in range(n,len(rows)) if inv[i] in group]
    payload=[struct.unpack('<3ffI3fI2f',rows[i]) for i in selected]
    uv=np.array([r[9:11] for r in payload])%1
    print('component',len(group),'cut vertices',len(selected),'uv bounds',np.round(uv.min(0),3),np.round(uv.max(0),3),'bones',sorted({r[4]&255 for r in payload}))
stl=(here/'bobnojio-head-edge-collapse-68.STL').read_bytes();count=struct.unpack_from('<I',stl,80)[0]
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
tri=np.frombuffer(stl,dtype=dt,count=count,offset=84)['vertices']
crossing=tri[np.any(tri[:,:,2]>=75,axis=1)&np.any(tri[:,:,2]<75,axis=1)]
segments=[]
for f in crossing:
    hits=[]
    for i in range(3):
        a,b=f[i],f[(i+1)%3]
        if (a[2]<75)!=(b[2]<75):
            p=a+(b-a)*((75-a[2])/(b[2]-a[2]));hits.append((.010+p[0]*.0025,.06-p[1]*.0025))
    if len(hits)==2:segments.append(hits)
canvas=Image.new('RGB',(800,800),(19,23,31));draw=ImageDraw.Draw(canvas)
def xy(x,z):return (400+x*3000,400-z*3000)
for a,b in segments:draw.line((xy(*a),xy(*b)),fill=(238,179,90),width=2)
for x,y,z in body:
    px,py=xy(x,z);draw.ellipse((px-3,py-3,px+3,py+3),fill=(83,195,255))
draw.text((14,14),'orange: head cut  |  blue: stock body intersections',fill=(245,245,245))
canvas.save(here/'neck-contours.png')
print('body cut vertices',len(body),'head cut segments',len(segments),'same plane',1.595)
