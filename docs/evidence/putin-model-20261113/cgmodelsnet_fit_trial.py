"""Fit a licensed GLB sculpture's facial depth to the original skinned head."""
import json
import math
import struct
import sys
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=(here/'objaverse-cgmodelsnet.glb').read_bytes()
n=struct.unpack_from('<I',data,12)[0];meta=json.loads(data[20:20+n]);base=28+n
parts=[]
for mesh in meta['meshes']:
    a=meta['accessors'][mesh['primitives'][0]['attributes']['POSITION']];v=meta['bufferViews'][a['bufferView']]
    assert a['componentType']==5126 and a['type']=='VEC3'
    off=base+v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',12)
    parts.append(np.ndarray((a['count'],3),'<f4',data,off,strides=(stride,4)).copy())
points=np.concatenate(parts);assert np.isfinite(points).all()
path=here/'smooth_face_fit_trial.py';source=path.read_text()
loader="""mesh=here/'bobnojio-vvp.STL'
stldt=np.dtype([('n','<f4',3),('v','<f4',(3,3)),('attr','<u2')])
ref=np.memmap(mesh,dtype=stldt,mode='r',offset=84)['v'].reshape(-1,3)"""
old_map="x=.010+ref[:,0]*.0025;y=1.385+ref[:,2]*.0028;z=.06-ref[:,1]*.0025"
new_map="x=.010+(ref[:,0]-18.67)*.00919;y=1.44855+ref[:,2]*.007247;z=.152-ref[:,1]*.0073"
for old,new in [(loader,'ref=points'),(old_map,new_map),('photo-face-prototype.gm','nose-subdivision-prototype.gm'),('smooth-face-fit-prototype.gm','cgmodelsnet-fit-prototype.gm')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','points':points})

original=(here/'nose-subdivision-prototype.gm').read_bytes();out=bytearray((here/'cgmodelsnet-fit-prototype.gm').read_bytes())
h=struct.unpack_from('<11i7f',out);off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',out,off);assert kind==4 and size%44==0
dt=np.dtype({'names':['p','uv'],'formats':[('<f4',3),('<f4',2)],'offsets':[0,36],'itemsize':44})
before=np.frombuffer(original,dtype=dt,count=size//44,offset=off+8);after=np.frombuffer(out,dtype=dt,count=size//44,offset=off+8)
uv=after['uv'].astype(float).copy();mask=(uv[:,0]>.5)&(uv[:,0]<.75)&(uv[:,1]>=0)&(uv[:,1]<.25)
diff=after['p'].astype(float)-before['p'].astype(float);scale=500/.41;c,s=math.cos(-.15),math.sin(-.15)
uv[mask,0]+=(diff[mask,0]*c+diff[mask,2]*s)*scale/2048;uv[mask,1]-=diff[mask,1]*scale/2048;after['uv'][:]=uv
(here/'cgmodelsnet-fit-prototype.gm').write_bytes(out)
renderer=here/'render_officer.py'
for angle,label in [(-.15,'front'),(1.,'profile'),(-1.,'other-profile'),(-math.pi/2,'true-profile')]:
    src=renderer.read_text()
    for needle in ("f'{prefix}-candidate-preview.png'","f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert src.count(needle)==1
        src=src.replace(needle,repr(f'cgmodelsnet-fit-{label}.png'))
    sys.argv=[str(renderer),'officer_5',str(here/'identity-geometry-face-atlas.png'),str(angle),str(here/'cgmodelsnet-fit-prototype.gm')]
    exec(compile(src,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
