"""Combine small head and fitted-body weight transitions without changing geometry."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_body_fit_half.py';source=path.read_text()
source=source[:source.index("path=here/'cgmodelsnet_neck_body_fit_checks.py'")]
assert source.count('t=.5*t*t*(3-2*t)')==1
source=source.replace('t=.5*t*t*(3-2*t)','t=.25*t*t*(3-2*t)').replace('cgmodelsnet-neck-body-fit-half-trial.gm','cgmodelsnet-neck-body-fit-quarter-trial.gm')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
stock=(here/'cgmodelsnet-stockuv-retopology.gm').read_bytes()
head=(here/'cgmodelsnet-head-field-35.gm').read_bytes()
body=(here/'cgmodelsnet-neck-body-fit-quarter-trial.gm').read_bytes()
assert len(stock)==len(head)==len(body)
h=struct.unpack_from('<11i7f',stock);off=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]+6*h[9]
kind,size=struct.unpack_from('<2i',stock,off);assert kind==4
def rows(data):return np.frombuffer(data,'u1',size,off+8).reshape(-1,44)
a,b,c=map(rows,(stock,head,body));mh=np.any(a!=b,axis=1);mb=np.any(a!=c,axis=1)
assert not np.any(mh&mb)
assert all(np.array_equal(x[:,np.r_[0:12,20:44]],a[:,np.r_[0:12,20:44]]) for x in (b,c))
out=bytearray(stock);v=np.frombuffer(out,'u1',size,off+8).reshape(-1,44);v[mh]=b[mh];v[mb]=c[mb]
(here/'cgmodelsnet-neck-combined-trial.gm').write_bytes(out)
print('changed head rows',int(mh.sum()),'body rows',int(mb.sum()),flush=True)
path=here/'cgmodelsnet_neck_body_fit_checks.py';source=path.read_text()
needle="source=source.replace('cgmodelsnet-neck-body-follow-','cgmodelsnet-neck-body-fit-')"
assert source.count(needle)==1
source=source.replace(needle,"source=source.replace('cgmodelsnet-neck-body-follow-','cgmodelsnet-neck-combined-')")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
