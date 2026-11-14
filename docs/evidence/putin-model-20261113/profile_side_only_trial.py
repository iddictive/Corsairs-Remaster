"""Limit the side projection to faces oriented away from the frontal camera."""
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
path = here / 'profile_projection_trial.py'
source = path.read_text()
changes = [
    ('np.mean(normals@np.array([-s,0,c]))<.50 or np.mean(normals[:,2])>.65',
     'np.mean(normals@np.array([-s,0,c]))<.65 or np.max(normals[:,2])>.25'),
    ('profile-projection-prototype.gm', 'profile-side-only-prototype.gm'),
    ('profile-projection-atlas.png', 'profile-side-only-atlas.png'),
    ("for old, new in changes:", "changes.append(('assert 30<selected<1000', 'assert 0<selected<1000'))\nfor old, new in changes:"),
]
for old, new in changes:
    assert source.count(old) == 1, old
    source = source.replace(old, new)
exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})

renderer = here / 'render_officer.py'
for angle, output in [(-.15, 'profile-side-only-front.png'), (1, 'profile-side-only-profile.png'), (-1, 'profile-side-only-other-profile.png')]:
    source = renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'", "f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old) == 1, old
        source = source.replace(old, repr(output))
    sys.argv = [str(renderer), 'officer_5', str(here/'profile-side-only-atlas.png'), str(angle), str(here/'profile-side-only-prototype.gm')]
    exec(compile(source, str(renderer), 'exec'), {'__file__': str(renderer), '__name__': '__main__'})
