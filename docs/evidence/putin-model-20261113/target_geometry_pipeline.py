"""Reproduce the target-landmark geometry and texture trial, offline."""
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
def run(name, args, replacements=()):
    path = here/name
    source = path.read_text()
    for old,new in replacements:
        expected = 8 if old == 'eye-full-' else 1
        assert source.count(old) == expected, old
        source = source.replace(old,new)
    sys.argv = [str(path),*map(str,args)]
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})

run('target_geometry_trial.py', [])
run('render_officer.py', ['officer_5',here/'officer_5-imagegen.png',-.15,here/'target-geometry-prototype.gm'], [
    ("f'{prefix}-candidate-preview.png'", "'target-geometry-baseline.png'"),
])
run('eye_full_projection_trial.py', [], [
    ('no-earring-prototype.gm','target-geometry-prototype.gm'),
    ('aligned-width-target.png','width-morph-target.png'),
    ('width-morph-baseline.png','target-geometry-baseline.png'),
    ('eye-full-','target-geometry-final-'),
])
