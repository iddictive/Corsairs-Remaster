"""Run the existing guarded reducer on the licensed cgmodelsnet head."""
import json
import struct
from pathlib import Path
import numpy as np

here=Path(__file__).resolve().parent
data=(here/'objaverse-cgmodelsnet.glb').read_bytes()
n=struct.unpack_from('<I',data,12)[0];meta=json.loads(data[20:20+n]);base=28+n
def accessor(i):
    a=meta['accessors'][i];v=meta['bufferViews'][a['bufferView']]
    dtype=np.dtype({5125:'<u4',5126:'<f4'}[a['componentType']]);width={'SCALAR':1,'VEC3':3}[a['type']]
    offset=base+v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',width*dtype.itemsize)
    return np.ndarray((a['count'],width),dtype,buffer=data,offset=offset,strides=(stride,dtype.itemsize)).copy()
parts=[]
for mesh in meta['meshes']:
    primitive=mesh['primitives'][0];vertices=accessor(primitive['attributes']['POSITION']);indices=accessor(primitive['indices']).reshape(-1,3)
    assert np.isfinite(vertices).all() and int(indices.max())<len(vertices)
    parts.append(vertices[indices])
input_triangles=np.concatenate(parts)
path=here/'edge_collapse_trial.py';source=path.read_text()
old="""raw=(here/'bobnojio-vvp.STL').read_bytes()
count=struct.unpack_from('<I',raw,80)[0];assert len(raw)==84+50*count
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
tri=np.frombuffer(raw,dtype=dt,count=count,offset=84)['vertices']
tri=tri[np.all(tri[:,:,2]>=75.,axis=1)]"""
new="""dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
tri=input_triangles[np.all(input_triangles[:,:,2]>=25.,axis=1)]"""
for before,after in [(old,new),('bobnojio-head-edge-collapse.STL','cgmodelsnet-head-edge-collapse.STL'),('bobnojio CC BY 4.0 edge collapse trial','cgmodelsnet CC BY 4.0 edge collapse trial')]:
    assert source.count(before)==1,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles})
