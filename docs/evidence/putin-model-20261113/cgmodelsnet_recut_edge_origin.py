"""Locate unusual welded edges and compare them with the original officer GM."""
import collections
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent;root=here.parents[2]
paths=[root/'src/assets/characters/putin-phonk/original.gm',root/'src/assets/characters/putin-phonk-recut-candidate/model-source.gm']
def scan(path):
    data=path.read_bytes();h=struct.unpack_from('<11i7f',data)
    t=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
    f=np.frombuffer(data,'<u2',3*h[9],t).reshape(-1,3);off=t+6*h[9];kind,size=struct.unpack_from('<2i',data,off);assert kind==4
    dt=np.dtype({'names':['p'],'formats':[('<f4',3)],'offsets':[0],'itemsize':44});p=np.frombuffer(data,dt,size//44,off+8)['p']
    q=np.rint(p*1e6).astype('int64');table=collections.defaultdict(list)
    for fid,face in enumerate(f):
        for a,b in ((face[0],face[1]),(face[1],face[2]),(face[2],face[0])):
            key=tuple(sorted((tuple(q[a]),tuple(q[b]))));table[key].append((fid,tuple(int(x) for x in face)))
    return {key:value for key,value in table.items() if len(value)!=2}
old,new=[scan(p) for p in paths]
for key,value in new.items():
    print('incidence',len(value),'edge',tuple(tuple(round(x/1e6,6) for x in p) for p in key),'in original',len(old.get(key,[])),'faces',value)
print('old unusual edges',len(old),'new unusual edges',len(new),'new absent from old',sum(k not in old for k in new))
