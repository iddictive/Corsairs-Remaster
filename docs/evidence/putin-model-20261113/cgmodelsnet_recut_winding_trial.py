"""Align graft and bridge triangle winding with the stock body, without changing vertices."""
import struct
from pathlib import Path
import numpy as np
here=Path(__file__).resolve().parent
data=bytearray((here/'cgmodelsnet-recut-trial.gm').read_bytes());h=struct.unpack_from('<11i7f',data)
tri=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,'<u2',h[9]*3,tri).reshape(-1,3)
head=(faces.min(axis=1)>=2106)&(faces.max(axis=1)<16012)
bridge=np.arange(len(faces))>=len(faces)-344
assert head.sum()==24200 and bridge.sum()==344 and not np.any(head&bridge)
selected=head|bridge
faces[selected]=faces[selected][:,[0,2,1]]
(here/'cgmodelsnet-recut-winding-trial.gm').write_bytes(data)
print('reversed head',int(head.sum()),'bridge',int(bridge.sum()),'unchanged vertex/UV/skin data',flush=True)
