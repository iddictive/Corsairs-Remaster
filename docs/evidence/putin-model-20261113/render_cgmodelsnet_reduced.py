"""Render the reduced CC BY head using the estimated player-head frame."""
import sys
from pathlib import Path

here=Path(__file__).resolve().parent
path=here/'render_reduced_head.py';source=path.read_text()
for old,new in [
    ("path=here/f'bobnojio-head-reduced-{voxel:g}.STL'", "path=here/'cgmodelsnet-head-edge-collapse.STL'"),
    (".010+tri[:,0]*.0025,1.385+tri[:,2]*.0028,.06-tri[:,1]*.0025", ".010+(tri[:,0]-18.67)*.00919,1.44855+tri[:,2]*.007247,.152-tri[:,1]*.0073"),
    ("(center,1.57,1800)", "(center,1.60,2200)"),
    ("f'bobnojio-reduced-{voxel:g}-views.png'", "'cgmodelsnet-reduced-views.png'")]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
sys.argv=[str(path)]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
