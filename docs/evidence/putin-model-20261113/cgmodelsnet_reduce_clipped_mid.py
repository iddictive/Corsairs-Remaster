"""Retain the chin while testing a higher exact cut below the collar."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_clipped.py';source=path.read_text()
for old,new in [('clip_mesh(input_triangles,20.)','clip_mesh(input_triangles,22.3)'),('cgmodelsnet-head-clipped-bounded.STL','cgmodelsnet-head-clipped-mid.STL')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
