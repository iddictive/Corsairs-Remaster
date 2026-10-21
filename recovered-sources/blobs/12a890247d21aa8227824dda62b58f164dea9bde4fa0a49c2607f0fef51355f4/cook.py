"""Cook a bounded captured-mesh reflection experiment; never change game assets."""
from pathlib import Path
import contextlib, ctypes as C, hashlib, io, json, runpy, struct

HERE=Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    fit=runpy.run_path(str(HERE.parent/'ship-instance-fit/command-fit.py'))
projection=json.loads((HERE.parent/'projected-geometry-fit/result.json').read_text())
lib=C.CDLL(str(HERE.parent/'mesh-lod-reduction-fit/libmeshoptimizer.dylib'))
U,F=C.c_uint,C.c_float
lib.meshopt_simplifyWithAttributes.restype=C.c_size_t
lib.meshopt_simplifyWithAttributes.argtypes=[C.POINTER(U),C.POINTER(U),C.c_size_t,C.POINTER(F),C.c_size_t,C.c_size_t,C.POINTER(F),C.c_size_t,C.POINTER(F),C.c_size_t,C.c_void_p,C.c_size_t,F,U,C.POINTER(F)]
rows={d['geometry_key']:d for d in projection['draws'] if d['epoch']==1}
assert len(rows)==78
records=[];report=[]
for key,d in sorted(rows.items()):
    vb_id,vo,ib_id,ibo,count,kind,base=d['address'];shape=d['geometry_alpha_key']
    assert shape[4:8]==[36,28,24,1]
    vb,ib=fit['geometry']['buffers'][vb_id],fit['geometry']['buffers'][ib_id]
    indices=struct.unpack_from('<'+('I' if kind=='UInt32' else 'H')*count,ib,ibo)
    unique=sorted(set(indices));remap={old:new for new,old in enumerate(unique)}
    compact=[remap[i] for i in indices]
    vertices=[vb[vo+(i+base)*36:vo+(i+base+1)*36] for i in unique]
    digest=hashlib.sha256()
    for v in vertices: assert len(v)==36;digest.update(v)
    digest.update(struct.pack('<'+'I'*count,*compact));assert digest.hexdigest()==key
    positions=[];attributes=[]
    for v in vertices:
        positions.extend(struct.unpack_from('<3f',v));attributes.extend(struct.unpack_from('<3f',v,12));attributes.extend(struct.unpack_from('<2f',v,28))
        b,g,r,a=v[24:28];attributes.extend((r/255,g/255,b/255,a/255))
    pos=(F*len(positions))(*positions);attr=(F*len(attributes))(*attributes);inp=(U*count)(*compact);out=(U*count)();weights=(F*9)(1,1,1,10,10,1,1,1,1);error=F()
    size=lib.meshopt_simplifyWithAttributes(out,inp,count,pos,len(unique),12,attr,36,weights,9,None,max(3,(count//24)*3),.01,0,C.byref(error))
    assert size%3==0 and 0<size<=count and all(i<len(unique) for i in out[:size])
    payload=struct.pack('<'+'I'*size,*out[:size])
    records.append(key.encode('ascii')+struct.pack('<II',count,size)+payload)
    report.append({'key':key,'original_indices':count,'candidate_indices':size,'combined_relative_error':error.value,'index_sha256':hashlib.sha256(payload).hexdigest()})
data=b'RLOD0001'+struct.pack('<I',len(records))+b''.join(records)
(HERE/'indices.bin').write_bytes(data)
result={'scope':'Diagnostic captured rigid meshes only; main geometry and game assets unchanged. Relative QEM error .01 is not a silhouette certificate. Reflection fine-detail omission is authorized; full scene remains unaccepted.',
        'upstream':json.loads((HERE.parent/'mesh-lod-reduction-fit/upstream/receipt.json').read_text()),'sha256':hashlib.sha256(data).hexdigest(),'records':report}
(HERE/'cook-result.json').write_text(json.dumps(result,indent=2)+'\n')
original=sum(r['original_indices'] for r in report);reduced=sum(r['candidate_indices'] for r in report)
print(json.dumps({'meshes':len(records),'original_indices':original,'candidate_indices':reduced,'reduction':1-reduced/original,'bytes':len(data)}),flush=True)
