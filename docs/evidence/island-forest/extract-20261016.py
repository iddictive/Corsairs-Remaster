"""Retained one-off measurement intake; reads original GM, writes explicit temp output."""
from pathlib import Path
import math
import struct
import sys
repo = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repo / 'experiments/native-metal'))
from island_geometry import load_static_gm
model_root, output = map(Path, sys.argv[1:3])
for name in ('Antigua', 'PortoBello'):
    scene = load_static_gm(model_root / name / (name + '.gm'))
    with (output / (name + '.faces')).open('wb') as stream:
        for indices in scene['indices']:
            vertices = [scene['vertices'][i] for i in indices]
            p = [v['p'] for v in vertices]
            u = [p[1][i] - p[0][i] for i in range(3)]
            v = [p[2][i] - p[0][i] for i in range(3)]
            normal = [u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]]
            length = math.sqrt(sum(n*n for n in normal))
            up = abs(normal[1])/length if sum(v['n'][1] for v in vertices) > 0 and length > 1e-6 else 0
            texture = vertices[0]['texture'].lower().split('.')[0]
            stream.write(struct.pack('<11f?3x', *(c for point in p for c in point), up,
                float(texture.startswith('jungle')), texture.startswith(('jungle', 'rockk'))))
    print(name, len(scene['indices']))
