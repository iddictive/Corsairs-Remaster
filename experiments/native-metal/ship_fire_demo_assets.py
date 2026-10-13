#!/usr/bin/env python3
"""Read installed ship assets into the offscreen fire demo; never alter game data."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from PIL import Image
from island_geometry import load_static_gm


IDENTITY = [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.]


def transform(p, matrix, direction=False):
    return [sum(p[j] * matrix[j * 4 + i] for j in range(3)) +
            (0. if direction else matrix[12 + i]) for i in range(3)]


def multiply(local, parent):
    return [sum(local[row * 4 + k] * parent[k * 4 + col] for k in range(4))
            for row in range(4) for col in range(4)]


def labels(path):
    data = path.read_bytes()
    h = struct.unpack_from('<11i7f', data)
    strings = data[72:72 + h[2]]
    offset = 72 + h[2] + h[3] * 4 + h[4] * 4 + h[5] * 56 + h[6] * 80
    def name(index):
        return strings[index:].split(b'\0', 1)[0].decode('cp1251')
    result = []
    for i in range(h[7]):
        row = struct.unpack_from('<3i16f4i4f', data, offset + i * 108)
        result.append((name(row[0]), name(row[1]), list(row[3:19])))
    return result


def decode_texture(path, output):
    data = path.read_bytes()
    flags, width, height, _, format_, size = struct.unpack_from('<6I', data)
    if flags or not 0 < width <= 16384 or not 0 < height <= 16384:
        raise ValueError(f'unsupported texture header: {path.name}')
    payload = data[24:24 + size]
    if len(payload) != size:
        raise ValueError(f'truncated texture: {path.name}')
    formats = {int.from_bytes(b'DXT1', 'little'): (1, 'DXT1'),
               int.from_bytes(b'DXT3', 'little'): (2, 'DXT3'),
               int.from_bytes(b'DXT5', 'little'): (3, 'DXT5')}
    if format_ in formats:
        image = Image.frombytes('RGBA', (width, height), payload, 'bcn', formats[format_])
    elif format_ in (21, 22):
        image = Image.frombytes('RGBA', (width, height), payload, 'raw', 'BGRA')
        if format_ == 22:
            image.putalpha(255)
    elif format_ == 23:
        image = Image.frombytes('RGB', (width, height), payload, 'raw', 'BGR;16')
    else:
        raise ValueError(f'unsupported texture format {format_}: {path.name}')
    image.save(output)


def export(resource, ship, output):
    output.mkdir(parents=True, exist_ok=True)
    model_root = resource / 'MODELS' / 'Ships' / ship
    hull_textures = resource / 'Textures' / 'Ships' / ship / 'Hull1'
    texture_index = {p.name.casefold(): p for p in resource.joinpath('Textures').rglob('*.tx')}
    vertices, indices, draws, burns, cloth_points, inputs = [], [], [], [], [], []
    textures, visited = {}, set()

    def texture(name):
        if name not in textures:
            source = hull_textures / (name + '.tx')
            if not source.is_file() and name.casefold() == 'fonar.tga':
                source = hull_textures / 'fonar_night.tga.tx'
            if not source.is_file():
                source = texture_index.get((name + '.tx').casefold())
            if source is None or not source.is_file():
                raise ValueError(f'missing authored texture: {name}')
            target = output / f'texture-{len(textures)}.png'
            decode_texture(source, target)
            inputs.append({'path': str(source), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest()})
            textures[name] = target.name
        return textures[name]

    def add_sail(corners, node, group):
        if len(corners) not in (3, 4):
            return
        start = len(indices)
        if len(corners) == 3:
            a, b, c = corners
            normal = cross([b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)])
            for p, uv in zip(corners, ((0., 0.), (1., 0.), (0., 1.))):
                indices.append(len(vertices)); vertices.append((*p, *normal, *uv))
        else:
            # These are authored labels, not SAILONE's reordered hardPoints.
            # SetLabel maps sail3 -> hardPoints[3], sail4 -> hardPoints[2].
            a, b, c, d = corners
            normal = cross([b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)])
            base, grid = len(vertices), 12
            for y in range(grid + 1):
                v = y / grid
                for x in range(grid + 1):
                    u = x / grid
                    p = [(1-v)*((1-u)*a[i]+u*b[i])+v*((1-u)*d[i]+u*c[i]) for i in range(3)]
                    p[2] += math.sin(u * math.pi) * math.sin(v * math.pi) * .8
                    vertices.append((*p, *normal, u, v))
            for y in range(grid):
                for x in range(grid):
                    k = base + y * (grid + 1) + x
                    indices.extend((k, k+1, k+grid+1, k+1, k+grid+2, k+grid+1))
        center = [sum(p[i] for p in corners)/len(corners) for i in range(3)]
        radius = max(math.sqrt(sum((p[i]-center[i])**2 for i in range(3))) for p in corners)
        cloth_points.append({'position': center, 'radius': min(6., max(1.2, radius*.55)),
                             'seed': 101.+len(cloth_points)*17.31, 'node': node,
                             'group': group, 'drawIndex': len(draws)})
        draws.append({'firstIndex': start, 'indexCount': len(indices)-start,
                      'texture': texture('parus_common.tga'), 'cloth': True})

    def visit(path, world):
        if path in visited:
            raise ValueError(f'repeated geometry node: {path.name}')
        visited.add(path)
        scene = load_static_gm(path)
        inputs.append({'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
        base, first = len(vertices), len(indices)
        for vertex in scene['vertices']:
            p, n = transform(vertex['p'], world), transform(vertex['n'], world, True)
            vertices.append((*p, *n, *vertex['uv']))
        indices.extend(base + i for tri in scene['indices'] for i in tri)
        for draw in scene['draws']:
            draws.append({'firstIndex': first + draw['first'], 'indexCount': draw['triangles']*3,
                          'texture': texture(draw['texture']), 'cloth': False})
        sails = {}
        for group, name, matrix in labels(path):
            if group == 'geometry' and name != 'path':
                child = model_root / f'{ship}_{name}.gm'
                if not child.is_file():
                    raise ValueError(f'missing geometry child: {child.name}')
                visit(child, multiply(matrix, world))
            elif group == 'fireplace':
                burns.append({'position': transform(matrix[12:15], world), 'radius': 2.2,
                              'seed': len(burns)*17.31})
            elif name in ('sail1', 'sail2', 'sail3', 'sail4'):
                sails.setdefault(group, {})[name] = transform(matrix[12:15], world)
        for group, corners in sails.items():
            add_sail([corners[k] for k in sorted(corners)], path.stem, group)

    visit(model_root / f'{ship}.gm', IDENTITY)
    (output / 'vertices.bin').write_bytes(b''.join(struct.pack('<8f', *v) for v in vertices))
    (output / 'indices.bin').write_bytes(struct.pack(f'<{len(indices)}I', *indices))
    document = {'ship': ship, 'vertices': 'vertices.bin', 'indices': 'indices.bin',
                'draws': draws, 'bounds': {'min': [min(v[i] for v in vertices) for i in range(3)],
                                         'max': [max(v[i] for v in vertices) for i in range(3)]},
                'burnPoints': burns, 'clothPoints': cloth_points, 'inputs': inputs,
                'note': 'Original installed meshes and textures; demo cloth uses authored sail corners.'}
    (output / 'ship.json').write_text(json.dumps(document, indent=2) + '\n')
    print(f'{ship}: {len(vertices)} vertices, {len(indices)//3} triangles, {len(draws)} draws, {len(burns)} fire anchors')


def cross(a, b):
    n = [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
    length = math.sqrt(sum(v*v for v in n))
    return [v/max(length, 1e-8) for v in n]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--resource', type=Path, required=True)
    parser.add_argument('--ship', default='FrigateQueen1')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    export(args.resource, args.ship, args.output)
