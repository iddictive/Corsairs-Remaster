"""Render the worst all-frame neck samples and stock comparison."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-stockuv-retopology.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-allframes-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('7236',7236),('7249',7249)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_musket_activebind_probe.py';source=path.read_text();source=source[:source.index("path=here/'cgmodelsnet_neck_animation_scan.py'")]
assert source.count("[('idle',63),('shot',207),('run',318)]")==1
source=source.replace("[('idle',63),('shot',207),('run',318)]","[('976',976)]").replace('animated-cgmodelsnet-musket-activebind-','animated-cgmodelsnet-allframes-musket-')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_stock_poses.py';source=path.read_text()
old="[('walk',10),('run',163),('fight-stand',8532),('8423',8423),('410',410),('1985',1985)]"
assert source.count(old)==1
source=source.replace(old,"[('7236',7236),('7249',7249)]").replace('animated-stock-officer-','animated-stock-allframes-')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
