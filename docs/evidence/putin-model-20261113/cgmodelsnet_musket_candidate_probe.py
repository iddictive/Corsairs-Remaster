"""Sample the current candidate on the stock musket animation, offline."""
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
bank=root/'experiments/native-metal/.cache/runtime/RESOURCE/animation/mushketer.an';assert bank.is_file()
path=here/'animated_face_render_trial.py';source=path.read_text()
changes=[('nose-subdivision-prototype.gm','cgmodelsnet-stockuv-retopology.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-musket-'),("(root/source['source_path']).read_bytes()",f'Path({str(bank)!r}).read_bytes()'),("[('walk',10),('run',163),('fight-stand',8532)]","[('idle',63),('shot',207),('run',318)]")]
for old,new in changes:
    assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_neck_retopology_checks.py';source=path.read_text()
source=source[:source.index("path=here/'animated_face_render_trial.py'")]
for old,new in [('cgmodelsnet-neck-retopology-trial.gm','cgmodelsnet-stockuv-retopology.gm'),('cgmodelsnet-neck-retopology-scan.json','cgmodelsnet-musket-neck-scan.json')]:
    assert source.count(old)>=1;source=source.replace(old,new)
needle="needle='rows=[]';assert source.count(needle)==1"
assert source.count(needle)==1
injection=f'''\nold="anim=rig((root/source['source_path']).read_bytes())"\nassert source.count(old)==1\nsource=source.replace(old,"anim=rig(Path({str(bank)!r}).read_bytes())")\nold="np.linspace(0,anim['frames']-1,128,dtype=int),10,163,8532"\nassert source.count(old)==1\nsource=source.replace(old,"np.linspace(0,anim['frames']-1,128,dtype=int),63,207,318")\n'''
source=source.replace(needle,injection+needle)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
