"""Render the local nose-atlas trial in three stock animation samples."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-body-scarf-worlduv.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-nose-baseline-atlas.png'),('animated-face-','animated-cgmodelsnet-nose-baseline-')]:
    assert source.count(old)>=1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
