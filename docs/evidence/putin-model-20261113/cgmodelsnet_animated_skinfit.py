"""Render actual skin-fitted graft in sample animation poses, offline."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for before,after,count in [
    ('nose-subdivision-prototype.gm','cgmodelsnet-neckfit-skinfit-prototype.gm',1),
    ('identity-geometry-face-atlas.png','cgmodelsnet-scarf-blend-atlas.png',1),
    ('animated-face-','animated-cgmodelsnet-skinfit-',2)]:
    assert source.count(before)==count,(before,source.count(before))
    source=source.replace(before,after)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
