"""Fit stock head vertices to the generated target's 2D landmarks, offline."""
import json
import math
import struct
from pathlib import Path
import numpy as np
from PIL import Image

here = Path(__file__).resolve().parent
data = bytearray((here/'no-earring-prototype.gm').read_bytes())
baseline = json.loads((here/'width-morph-baseline-landmarks.json').read_text())
target = json.loads((here/'width-morph-target-landmarks.json').read_text())
keys = ('leftEye', 'rightEye', 'leftBrow', 'rightBrow', 'nose', 'noseCrest', 'outerLips')
points = np.array([p for k in keys for p in baseline[k]], dtype=float)
size = Image.open(here/'width-morph-target.png').size
assert size[0] == size[1]
goals = np.array([p for k in keys for p in target[k]], dtype=float)*(512/size[0])
delta = np.clip(goals-points, -12, 12)
sigma = 19.
d = points[:,None,:]-points[None,:,:]
kernel = np.exp(-np.sum(d*d, axis=2)/(2*sigma*sigma))
weights = np.linalg.solve(kernel+.2*np.eye(len(points)), delta)
h = struct.unpack_from('<11i7f',data)
off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,bufsize=struct.unpack_from('<2i',data,off)
assert kind == 4 and bufsize%44 == 0
dt=np.dtype({'names':['p','n'],'formats':[('<f4',3),('<f4',3)],'offsets':[0,20],'itemsize':44})
v=np.frombuffer(data,dtype=dt,count=bufsize//44,offset=off+8)
old=v['p'].astype(float).copy(); normals=v['n'].astype(float).copy()
a=-.15; c,s=math.cos(a),math.sin(a); zoom=500/.41

def smooth(t):
    t=np.clip(t,0,1)
    return t*t*(3-2*t)

def transformed(p):
    x,y,z=p.T
    screen=np.column_stack(((x*c+z*s+.004)*zoom+256,490-(y-1.465)*zoom))
    dist=screen[:,None,:]-points[None,:,:]
    rbf=np.exp(-np.sum(dist*dist,axis=2)/(2*sigma*sigma))@weights
    # Keep the ears, back of head and neckline rigid; smoothly blend the face.
    influence=smooth((z-.025)/.035)*smooth((y-1.57)/.045)*smooth((1.82-y)/.05)
    result=p.copy()
    result[:,0]+=rbf[:,0]*influence/(zoom*c)
    result[:,1]-=rbf[:,1]*influence/zoom
    return result

new=transformed(old)
eps=1e-5
jac=np.stack([(transformed(old+np.eye(3)[i]*eps)-transformed(old-np.eye(3)[i]*eps))/(2*eps) for i in range(3)],axis=2)
new_normals=np.linalg.solve(np.transpose(jac,(0,2,1)),normals[...,None])[...,0]
new_normals/=np.maximum(np.linalg.norm(new_normals,axis=1,keepdims=True),1e-10)
v['p'][:]=new; v['n'][:]=new_normals
disp=np.linalg.norm(new-old,axis=1)
assert np.isfinite(new).all() and np.isfinite(new_normals).all() and disp.max()<.014
(here/'target-geometry-prototype.gm').write_bytes(data)
print('moved',int((disp>1e-6).sum()),'vertices, maximum displacement',round(float(disp.max()),6))
