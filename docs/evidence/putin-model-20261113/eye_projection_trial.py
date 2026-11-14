"""Include visible eye geometry in the offline front projection experiment."""
import sys
from pathlib import Path

here = Path(__file__).resolve().parent


def run(name, args, replacements):
    path = here / name
    source = path.read_text()
    for before, after in replacements:
        assert source.count(before) == 1, before
        source = source.replace(before, after)
    sys.argv = [str(path), *map(str, args)]
    exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})


old = 'if not (np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))):continue'
new = '''skin = np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))
    eye = 165 < center[1] < 205 and (195 < center[0] < 235 or 270 < center[0] < 310)
    if not (skin or eye):continue'''
run('separate_face_uv.py', ['aligned-width-target.png', here / 'morph-prototype.gm', 'width-morph-baseline.png'], [
    (old, new),
    ('separate-face-prototype.gm', 'eye-projection-prototype.gm'),
    ('separate-face-atlas.png', 'eye-projection-atlas.png'),
])
for angle, output in [(-.15, 'eye-projection-front.png'), (1, 'eye-projection-profile.png'), (-1, 'eye-projection-other-profile.png')]:
    run('render_officer.py', ['officer_5', here / 'eye-projection-atlas.png', angle, here / 'eye-projection-prototype.gm'], [
        ("f'{prefix}-candidate-preview.png'", repr(output)),
        ("f'{prefix}-candidate-angle-{angle:g}.png'", repr(output)),
    ])
