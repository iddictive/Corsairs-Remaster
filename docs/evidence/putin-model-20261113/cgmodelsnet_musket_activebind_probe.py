"""Use the active musket bank's frame-zero bind, as the engine does."""
import hashlib
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
bank=here/'mushketer-source.an'
assert hashlib.sha256(bank.read_bytes()).hexdigest()=='9651c14e7b3acb7e8e5f541c5696534a6782295b00dd2994b88c02240ab65788'
source_line="source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']"
injection=f"\nactive=rig(Path({str(bank)!r}).read_bytes());assert active['parents']==source['parents']\nsource=dict(source);source.update(rest=active['rest'],bind_pose=active['pose'](0),bind_root=active['roots'][0])"
path=here/'animated_face_render_trial.py';source=path.read_text();assert source.count(source_line)==1
source=source.replace(source_line,source_line+injection)
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-stockuv-retopology.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-musket-activebind-'),("(root/source['source_path']).read_bytes()",f'Path({str(bank)!r}).read_bytes()'),("[('walk',10),('run',163),('fight-stand',8532)]","[('idle',63),('shot',207),('run',318)]")]:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_neck_animation_scan.py';source=path.read_text();assert source.count(source_line)==1
source=source.replace(source_line,source_line+injection)
for old,new in [('cgmodelsnet-body-scarf-worlduv.gm','cgmodelsnet-stockuv-retopology.gm'),('faces[-628:]','faces[-694:]'),('bridge.max()<15623','bridge.max()<15983'),('cgmodelsnet-neck-animation-scan.json','cgmodelsnet-musket-activebind-scan.json'),("(root/source['source_path']).read_bytes()",f'Path({str(bank)!r}).read_bytes()'),("np.linspace(0,anim['frames']-1,128,dtype=int),10,163,8532","np.linspace(0,anim['frames']-1,128,dtype=int),63,207,318")]:
    assert source.count(old)==1,(old,source.count(old));source=source.replace(old,new)
needle='rows=[]';assert source.count(needle)==1
source=source.replace(needle,"labels=(indices[edges]>=15016)&(indices[edges]<15623)\nedges=edges[np.sum(labels,axis=1)==1]\nprint('body-head edges',len(edges))\nrows=[]")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
