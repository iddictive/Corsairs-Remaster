#!/usr/bin/env python3
"""Import the head geometry of the pinned, untransformed GLB bust as depth intervals.

Usage: python3 tools/prepare_nuclear_head.py model.glb output.hpp
The header is the retained geometry source. Materials/UVs/textures are discarded.
Native render evidence: docs/evidence/nuclear-smoke-face/.
"""
import array
import hashlib
import json
from pathlib import Path
import struct
import sys

SOURCE_SHA256 = 'e9f96665fdf59fd35c542ad7fc0f076749b7519aeae7c4dba6d055935a61e787'


def prepare(source: Path, output: Path):
    data = source.read_bytes()
    if hashlib.sha256(data).hexdigest() != SOURCE_SHA256:
        raise ValueError('Unexpected bust source; review geometry bounds before importing another model')
    magic, version, length = struct.unpack_from('<III', data)
    if (magic, version, length) != (0x46546C67, 2, len(data)):
        raise ValueError('Expected binary glTF 2.0')
    size, kind = struct.unpack_from('<II', data, 12)
    if kind != 0x4E4F534A:
        raise ValueError('Expected GLB JSON chunk')
    doc = json.loads(data[20:20 + size])
    offset = 20 + size
    size, kind = struct.unpack_from('<II', data, offset)
    if kind != 0x004E4942 or doc.get('extensionsRequired'):
        raise ValueError('Expected plain GLB binary geometry')
    blob = data[offset + 8:offset + 8 + size]
    if any(any(k in node for k in ('matrix', 'translation', 'rotation', 'scale', 'skin'))
           for node in doc['nodes']):
        raise ValueError('Import requires untransformed geometry')
    if len(doc['meshes']) != 1 or len(doc['meshes'][0]['primitives']) != 1:
        raise ValueError('Expected one bust mesh')
    primitive = doc['meshes'][0]['primitives'][0]
    if primitive.get('mode', 4) != 4:
        raise ValueError('Expected triangles')

    def accessor(index, fmt, components, component_type):
        a = doc['accessors'][index]
        view = doc['bufferViews'][a['bufferView']]
        if a['componentType'] != component_type or 'sparse' in a or 'byteStride' in view:
            raise ValueError('Expected tightly packed accessor')
        result = array.array(fmt)
        start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
        result.frombytes(blob[start:start + a['count'] * components * result.itemsize])
        if sys.byteorder != 'little':
            result.byteswap()
        return result

    vertices = accessor(primitive['attributes']['POSITION'], 'f', 3, 5126)
    indices = accessor(primitive['indices'], 'I', 1, 5125)
    width, height = 96, 128
    # Crop shoulders/neck. Bounds belong to this authored bust, not the renderer.
    x0, x1, y0, y1, z_scale = -.30, .30, -.18, .50, .35
    front, back = [-1e9] * (width * height), [1e9] * (width * height)
    for i in range(0, len(indices), 3):
        points = [vertices[j * 3:j * 3 + 3] for j in indices[i:i + 3]]
        if max(p[1] for p in points) < y0 or min(p[1] for p in points) > y1:
            continue
        projected = [((x - x0) / (x1 - x0) * width,
                      (y1 - y) / (y1 - y0) * height, z / z_scale) for x, y, z in points]
        (ax, ay, az), (bx, by, bz), (cx, cy, cz) = projected
        denominator = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
        if abs(denominator) < 1e-9:
            continue
        for y in range(max(0, int(min(ay, by, cy))), min(height - 1, int(max(ay, by, cy))) + 1):
            for x in range(max(0, int(min(ax, bx, cx))), min(width - 1, int(max(ax, bx, cx))) + 1):
                a = ((by - cy) * (x + .5 - cx) + (cx - bx) * (y + .5 - cy)) / denominator
                b = ((cy - ay) * (x + .5 - cx) + (ax - cx) * (y + .5 - cy)) / denominator
                c = 1 - a - b
                if min(a, b, c) < 0:
                    continue
                z = a * az + b * bz + c * cz
                pixel = y * width + x
                front[pixel] = max(front[pixel], z)
                back[pixel] = min(back[pixel], z)
    def encode(value):
        return round(max(0, min(255, (value * .5 + .5) * 255)))
    text = ('// Geometry-only head intervals; imported by tools/prepare_nuclear_head.py.\n'
            '// Source: https://app.threedee.design/community/generations/free-3d-bust-of-putin\n'
            f'// GLB SHA256: {hashlib.sha256(data).hexdigest()}\n'
            '#pragma once\n#include <cstdint>\nnamespace storm_metal {\n'
            f'inline constexpr unsigned nuclearHeadWidth={width}, nuclearHeadHeight={height};\n')
    for name, values in (('Front', front), ('Back', back)):
        text += f'inline constexpr uint8_t nuclearHead{name}[]={{\n'
        for row in range(height):
            text += '    ' + ','.join(str(encode(v)) for v in values[row * width:(row + 1) * width]) + ',\n'
        text += '};\n'
    output.write_text(text + '}\n')
    print(f'Imported {len(indices) // 3} triangles into {width}x{height} front/back geometry intervals')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    prepare(Path(sys.argv[1]), Path(sys.argv[2]))
