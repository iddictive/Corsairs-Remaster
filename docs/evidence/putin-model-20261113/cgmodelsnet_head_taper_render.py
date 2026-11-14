"""Render regular and worst discrete poses of the lower-head-weight trial."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-head-weight-taper-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-head-taper-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('run',163),('612',612),('5320',5320),('7236',7236)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_musket_activebind_probe.py';source=path.read_text();source=source[:source.index("path=here/'cgmodelsnet_neck_animation_scan.py'")]
for old,new in [('cgmodelsnet-stockuv-retopology.gm','cgmodelsnet-head-weight-taper-trial.gm'),('animated-cgmodelsnet-musket-activebind-','animated-cgmodelsnet-head-taper-musket-'),("[('idle',63),('shot',207),('run',318)]","[('shot',207),('959',959)]")]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
