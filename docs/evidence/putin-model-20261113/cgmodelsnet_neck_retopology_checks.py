"""Measure and render the rebuilt neck bridge on the established frame sample."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_animation_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-neck-retopology-trial.gm'),('faces[-628:]','faces[-694:]'),('bridge.max()<15623','bridge.max()<15983'),('cgmodelsnet-neck-animation-scan.json','cgmodelsnet-neck-retopology-scan.json')]:
    assert source.count(old)==1
    source=source.replace(old,new)
needle='rows=[]';assert source.count(needle)==1
source=source.replace(needle,"labels=(indices[edges]>=15016)&(indices[edges]<15623)\nedges=edges[np.sum(labels,axis=1)==1]\nprint('body-head edges',len(edges))\nrows=[]")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-neck-retopology-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-nose-baseline-atlas.png'),('animated-face-','animated-cgmodelsnet-neck-retopology-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('walk',10),('run',163),('fight-stand',8532),('8423',8423),('410',410),('1985',1985)]")]:
    assert source.count(old)>=1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
