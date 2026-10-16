"""Retained one-off measurement intake; reads original GM, writes explicit temp output."""
from pathlib import Path
import math
import struct
import sys
from collections import Counter
repo = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(repo / 'experiments/native-metal'))
from island_geometry import load_static_gm, _reader
model_root, output = map(Path, sys.argv[1:3])
for name in ('Antigua', 'PortoBello'):
    path = model_root / name / (name + '.gm')
    scene = load_static_gm(path)
    read, skip, position = _reader(path.read_bytes())
    header = read('11i7f')
    skip(header[2]+header[3]*4+header[4]*4+header[5]*struct.calcsize('<2i4f8i')+header[6]*80+header[7]*struct.calcsize('<3i16f4i4f'))
    objects = [read('3i4f6i8i4ii') for _ in range(header[8])]
    print(name, 'original object flags', dict(Counter(hex(obj[2]) for obj in objects)))
    assert all(obj[2] & 1 and obj[2] & 16 for obj in objects), 'these CPU exports assume all authored primary objects visible and collidable'
    roles = ('town', 'fort', 'house', 'roof', 'wall', 'brick', 'road', 'street', 'paving')
    settlement = Counter()
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
            built = any(role in texture for role in roles)
            if built:
                settlement[texture] += 1
            stream.write(struct.pack('<11f?3x', *(c for point in p for c in point), up,
                -1.0 if built else float(texture.startswith('jungle')), texture.startswith(('jungle', 'rockk'))))
    print(name, len(scene['indices']), 'settlement triangles', dict(settlement))
