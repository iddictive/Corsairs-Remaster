"""Render three stock animation samples for the scarf bridge trial."""
from pathlib import Path

here = Path(__file__).resolve().parent
path = here / 'animated_face_render_trial.py'
source = path.read_text()
for old, new in [
    ('nose-subdivision-prototype.gm', 'cgmodelsnet-body-scarf-fullring.gm'),
    ('identity-geometry-face-atlas.png', 'cgmodelsnet-scarf-bridge-atlas.png'),
    ('animated-face-', 'animated-cgmodelsnet-scarf-bridge-'),
]:
    assert source.count(old) >= 1
    source = source.replace(old, new)
exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})
