"""Offline landmark-driven, smooth stock-GM face morph; not a delivery asset."""
import hashlib
import json
import math
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
root=here.parents[2]
gm=bytearray((root/'src/assets/characters/putin-phonk/original.gm').read_bytes())
assert hashlib.sha256(gm).hexdigest()=='74b36c17871a497d7eaf25a7c5ab372bc9e1abb9addd806611b3ad1a407eccd1'
ref=json.loads((here/'reference-putin-2024-landmarks.json').read_text())
orig=json.loads((here/'officer_5-original-preview-landmarks.json').read_text())
regions=('contour','leftEye','rightEye','nose','noseCrest','outerLips','leftBrow','rightBrow')
src=np.array([p for key in regions for p in ref[key]],dtype=float)
dst=np.array([p for key in regions for p in orig[key]],dtype=float)
cent=lambda a:np.asarray(a).mean(axis=0)
re=np.array([cent(ref[x]) for x in ('leftEye','rightEye')]); oe=np.array([cent(orig[x]) for x in ('leftEye','rightEye')])
v0=re[1]-re[0]; v1=oe[1]-oe[0]; scale=np.linalg.norm(v1)/np.linalg.norm(v0)
theta=math.atan2(v1[1],v1[0])-math.atan2(v0[1],v0[0]); c,s=math.cos(theta),math.sin(theta)
rot=np.array([[c,-s],[s,c]])*scale
target=(src-re.mean(0))@rot.T+oe.mean(0)
movement=np.clip(target-dst,-22,22)
# Damped RBF: preserve local detail and reduce unstable jumps near landmarks.
sigma=24.; diff=dst[:,None,:]-dst[None,:,:]
k=np.exp(-np.sum(diff*diff,axis=2)/(2*sigma*sigma))
coeff=np.linalg.solve(k+.15*np.eye(len(dst)),movement)
h=struct.unpack_from('<11i7f',gm); off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
typ,size=struct.unpack_from('<2i',gm,off); assert typ==4 and size%44==0
dtype=np.dtype({'names':['p','norm'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(gm,dtype=dtype,count=size//44,offset=off+8)
old=v['p'].astype(float).copy(); normal=v['norm'].astype(float).copy()
a=-.15; ca,sa=math.cos(a),math.sin(a); zoom=500/.41
screen=np.column_stack(((old[:,0]*ca+old[:,2]*sa+.004)*zoom+256,490-(old[:,1]-1.465)*zoom))
d=screen[:,None,:]-dst[None,:,:]; g=np.exp(-np.sum(d*d,axis=2)/(2*sigma*sigma))
f=g@coeff
# Restrict to head; smooth fade on neck ensures no rigid boundary.
t=np.clip((old[:,1]-1.53)/.08,0,1); fade=t*t*(3-2*t)
eligible=(np.abs(old[:,0])<.15)&(old[:,1]>1.53)&(old[:,1]<1.9)
f*=fade[:,None]*eligible[:,None]
v['p'][:,0]=old[:,0]+f[:,0]/(zoom*ca)
v['p'][:,1]=old[:,1]-f[:,1]/zoom
# Approximate differential of the smooth head-space warp for normals.
grad=np.einsum('ni,nij,ik->njk',g,-d/(sigma*sigma),coeff)
for i in np.flatnonzero(eligible):
    fx,fy=grad[i,:,0]*fade[i],grad[i,:,1]*fade[i]
    j=np.array([[1+fx[0]/ca,-fx[1]/ca,fx[0]*sa/ca],[-fy[0]*ca,1+fy[1],-fy[0]*sa],[0,0,1]])
    try: n=np.linalg.solve(j.T,normal[i]); n/=np.linalg.norm(n); v['norm'][i]=n
    except np.linalg.LinAlgError: pass
disp=np.linalg.norm(v['p'].astype(float)-old,axis=1)
assert np.isfinite(v['p']).all() and disp.max()<.025
out=here/'landmark-morph-prototype.gm'; out.write_bytes(gm)
print('vertices moved',int((disp>1e-6).sum()),'max displacement',round(float(disp.max()),6),'sha256',hashlib.sha256(gm).hexdigest())
