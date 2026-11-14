"""Reproduce the landmark-aligned, offline-only face UV experiment."""
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


run('landmark_warp.py', ['width-morph-baseline', 'width-morph-target'], [
    ('landmark-target.png', 'aligned-width-target.png'),
    ('landmark-coverage.png', 'aligned-width-coverage.png'),
])
run('separate_face_uv.py', ['aligned-width-target.png', here / 'morph-prototype.gm', 'width-morph-baseline.png'], [
    ('separate-face-prototype.gm', 'aligned-face-prototype.gm'),
    ('separate-face-atlas.png', 'aligned-face-atlas.png'),
])
for angle, output in [(-.15, 'aligned-face-front.png'), (1, 'aligned-face-profile.png')]:
    run('render_officer.py', ['officer_5', here / 'aligned-face-atlas.png', angle, here / 'aligned-face-prototype.gm'], [
        ("f'{prefix}-candidate-preview.png'", repr(output)),
        ("f'{prefix}-candidate-angle-{angle:g}.png'", repr(output)),
    ])
