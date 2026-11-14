"""Render unchanged candidate at the two new worst sampled frames."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-stockuv-retopology.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-head-control-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('612',612)]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_musket_activebind_probe.py';source=path.read_text();source=source[:source.index("path=here/'cgmodelsnet_neck_animation_scan.py'")]
for old,new in [('animated-cgmodelsnet-musket-activebind-','animated-cgmodelsnet-head-control-musket-'),("[('idle',63),('shot',207),('run',318)]","[('959',959)]")]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
