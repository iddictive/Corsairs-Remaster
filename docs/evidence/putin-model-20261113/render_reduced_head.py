"""Render the reduced STL head in its tentative GM coordinate frame."""
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here=Path(__file__).resolve().parent
root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from render_unarmed_atlas import panel

voxel=float(sys.argv[1]) if len(sys.argv)>1 else 1.5
path=here/f'bobnojio-head-reduced-{voxel:g}.STL'
raw=path.read_bytes(); count=struct.unpack_from('<I',raw,80)[0]
assert len(raw)==84+50*count
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')])
tri=np.frombuffer(raw,dtype=dt,count=count,offset=84)['vertices'].reshape(-1,3)
points=np.column_stack((.010+tri[:,0]*.0025,1.385+tri[:,2]*.0028,.06-tri[:,1]*.0025))
faces=np.arange(len(points)).reshape(-1,3)
canvas=Image.new('RGB',(1536,512))
for i,angle in enumerate((0,90,-90)):
    center={0:.010,90:-.06,-90:.06}[angle]
    canvas.paste(panel(points,faces,angle,(512,512),f'angle {angle}',(center,1.57,1800)),(i*512,0))
canvas.save(here/f'bobnojio-reduced-{voxel:g}-views.png')
print('rendered',count,'triangles, three views')
