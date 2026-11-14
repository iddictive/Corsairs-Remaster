"""Test a V-shaped jaw/neck cut that rises at the lateral collar."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_reduce_slanted.py';source=path.read_text()
for before,after in [
    ('+.25*(.152-.0073*work[:,:,1]-.02)', '+.25*(.152-.0073*work[:,:,1]-.02)-.30*abs(.00919*(work[:,:,0]-18.67))'),
    ('+.25*(.152-.0073*result[:,:,1]-.02)', '+.25*(.152-.0073*result[:,:,1]-.02)-.30*abs(.00919*(result[:,:,0]-18.67))'),
    ('cgmodelsnet-head-slanted.STL', 'cgmodelsnet-head-shaped.STL')]:
    assert source.count(before)==1,before
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
