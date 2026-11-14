"""Test projection onto all front-visible eye-region triangles, offline."""
import sys
from pathlib import Path

here = Path(__file__).resolve().parent

def run(name, args, replacements):
    path = here / name
    source = path.read_text()
    for old, new in replacements:
        assert source.count(old) == 1, old
        source = source.replace(old, new)
    sys.argv = [str(path), *map(str, args)]
    exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})

old = 'if not (np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))):continue'
new = '''skin = np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))
    eye = 160 < center[1] < 210 and (190 < center[0] < 245 or 260 < center[0] < 320)
    if not (skin or eye):continue'''
run('separate_face_uv.py', ['aligned-width-target.png', here / 'no-earring-prototype.gm', 'width-morph-baseline.png'], [
    (old, new),
    ('if np.any(normals@np.array([-s,0,c])<.55):continue', 'if not eye and np.any(normals@np.array([-s,0,c])<.55):continue'),
    ("blur=np.asarray(Image.fromarray(visible).filter(ImageFilter.GaussianBlur(6)),dtype=float)/255", "Image.fromarray(visible).save(here/'eye-full-coverage.png')\nblur=np.asarray(Image.fromarray(visible).filter(ImageFilter.GaussianBlur(6)),dtype=float)/255"),
    ('separate-face-prototype.gm', 'eye-full-prototype.gm'),
    ('separate-face-atlas.png', 'eye-full-atlas.png'),
])
for angle, output in [(-.15, 'eye-full-front.png'), (1, 'eye-full-profile.png'), (-1, 'eye-full-other-profile.png')]:
    run('render_officer.py', ['officer_5', here / 'eye-full-atlas.png', angle, here / 'eye-full-prototype.gm'], [
        ("f'{prefix}-candidate-preview.png'", repr(output)),
        ("f'{prefix}-candidate-angle-{angle:g}.png'", repr(output)),
    ])
