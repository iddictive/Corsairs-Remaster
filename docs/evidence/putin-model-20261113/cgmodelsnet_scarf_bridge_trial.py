"""Try matching the bridge material to both scarf surfaces."""
import math
import sys
from pathlib import Path
import numpy as np
from PIL import Image

here = Path(__file__).resolve().parent
atlas = np.array(Image.open(here / 'cgmodelsnet-body-scarf-fullring-atlas.png').convert('RGB'))
stock = np.array(Image.open(here / 'officer_5-imagegen.png').convert('RGB').resize((1024, 1024), Image.Resampling.LANCZOS))
fabric = stock[716:736, 665:735]
y, x = np.mgrid[0:128, 0:1024]
atlas[:128, 1024:] = fabric[(y // 2) % 20, (x // 2) % 70]
out = here / 'cgmodelsnet-scarf-bridge-atlas.png'
Image.fromarray(atlas).save(out)
renderer = here / 'render_officer.py'
for angle, label in [(-.15, 'front'), (1., 'profile'), (-1., 'other-profile'), (-math.pi / 2, 'true-profile')]:
    source = renderer.read_text()
    for old in ("f'{prefix}-candidate-preview.png'", "f'{prefix}-candidate-angle-{angle:g}.png'"):
        assert source.count(old) == 1
        source = source.replace(old, repr(f'cgmodelsnet-scarf-bridge-{label}.png'))
    sys.argv = [str(renderer), 'officer_5', str(out), str(angle), str(here / 'cgmodelsnet-body-scarf-fullring.gm')]
    exec(compile(source, str(renderer), 'exec'), {'__file__': str(renderer), '__name__': '__main__'})
