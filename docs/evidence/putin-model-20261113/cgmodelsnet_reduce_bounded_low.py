"""Keep the full chin by lowering the bounded-reduction input cut."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_bounded.py';source=path.read_text()
assert source.count("source=path.read_text()") == 1
source=source.replace("source=path.read_text()","source=path.read_text()\nassert source.count('input_triangles[:,:,2]>=25.')==1\nsource=source.replace('input_triangles[:,:,2]>=25.','input_triangles[:,:,2]>=20.')")
assert source.count('cgmodelsnet-head-bounded.STL')==1
source=source.replace('cgmodelsnet-head-bounded.STL','cgmodelsnet-head-bounded-low.STL')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
