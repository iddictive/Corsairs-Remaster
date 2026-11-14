"""Reduce a bust clipped below the chin on an oblique join plane."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_clipped.py';source=path.read_text()
needle="clip_mesh=scope['clip_mesh']"
replacement="""horizontal_clip=scope['clip_mesh']
def clip_mesh(triangles, unused):
    work=triangles.astype('float64').copy()
    offset=(1.44855-1.595+.25*(.152-.0073*work[:,:,1]-.02))/.007247
    work[:,:,2]+=offset
    result=horizontal_clip(work,0.)
    result[:,:,2]-=(1.44855-1.595+.25*(.152-.0073*result[:,:,1]-.02))/.007247
    return result.astype('float32')
"""
assert source.count(needle)==1
source=source.replace(needle,replacement)
assert source.count('cgmodelsnet-head-clipped-bounded.STL')==1
source=source.replace('cgmodelsnet-head-clipped-bounded.STL','cgmodelsnet-head-slanted.STL')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
