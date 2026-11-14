"""Test lowering the back of the sculpture into the collar, preserving its UV."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_rear_geometry_trial.py';source=path.read_text()
old='''    d=p[:,[0,2]]-np.array([.01,0]);r=np.maximum(np.linalg.norm(d,axis=1),1e-9);newr=r-amount*fade
    assert np.all(newr>0)
    result[:,0]=.01+d[:,0]*newr/r;result[:,2]=d[:,1]*newr/r
    result[:,1]+=-.25*(result[:,2]-p[:,2])+.30*(abs(result[:,0]-.01)-abs(p[:,0]-.01))'''
assert source.count(old)==1
source=source.replace(old,'    result[:,1]-=amount*fade')
source=source.replace("choices=['3','6']","choices=['3','6']")
source=source.replace('cgmodelsnet-rear-geometry-','cgmodelsnet-rear-lower-')
for strength in ['3','6']:
    sys.argv=[str(path),strength]
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
    path2=here/'cgmodelsnet_rear_geometry_render.py';render=path2.read_text()
    render=render.replace('cgmodelsnet-rear-geometry-', 'cgmodelsnet-rear-lower-')
    sys.argv=[str(path2),strength]
    exec(compile(render,str(path2),'exec'),{'__file__':str(path2),'__name__':'__main__'})
