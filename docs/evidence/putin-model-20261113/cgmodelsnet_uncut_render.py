"""Render the original head triangles to separate source from reducer errors."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_trial.py';source=path.read_text()
needle="exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','input_triangles':input_triangles})"
assert source.count(needle)==1
replacement="""tri=input_triangles[np.all(input_triangles[:,:,2]>=25.,axis=1)]
dt=np.dtype([('normal','<f4',3),('vertices','<f4',(3,3)),('attr','<u2')]); rows=np.zeros(len(tri),dtype=dt)
rows['vertices']=tri
(here/'cgmodelsnet-head-original-cut.STL').write_bytes(b'cgmodelsnet CC BY 4.0 original cut'.ljust(80,b'\\0')+struct.pack('<I',len(rows))+rows.tobytes())
print('original head triangles',len(rows))
"""
source=source.replace(needle,replacement)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
renderer=here/'render_cgmodelsnet_reduced.py';source=renderer.read_text()
for old,new in [('cgmodelsnet-head-edge-collapse.STL','cgmodelsnet-head-original-cut.STL'),('cgmodelsnet-reduced-views.png','cgmodelsnet-original-cut-views.png')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(renderer),'exec'),{'__file__':str(renderer),'__name__':'__main__'})
