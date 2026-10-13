#!/usr/bin/env python3
"""Prepare one skinned character variant; never stage or touch player state."""
from __future__ import annotations

import hashlib
import io
import json
import math
import struct
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parent
FACE_ID = 2006


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load_gm(data, original=True):
    head = list(struct.unpack_from('<11i7f', data))
    assert head[0] == 825110581 and head[4:11] == [1, 1, 0, 5, 1, 4506 if original else 4602, 1]
    names_end = 72 + head[2]
    texture_at = names_end + head[3] * 4
    object_at = texture_at + head[4] * 4 + head[5] * 56 + head[6] * 80 + head[7] * 108
    triangles_at = object_at + head[8] * 104
    buffer_at = triangles_at + head[9] * 6
    typ, size = struct.unpack_from('<2i', data, buffer_at)
    assert typ == 4 and size == (4267 if original else 4317) * 44
    vertices_at = buffer_at + 8
    triangles = list(struct.iter_unpack('<3H', data[triangles_at:buffer_at]))
    vertices = [data[i:i + 44] for i in range(vertices_at, vertices_at + size, 44)]
    return head, object_at, triangles_at, buffer_at, vertices_at, triangles, vertices


def component(triangles, seed):
    adjacent = {}
    for a, b, c in triangles:
        for i, j, k in ((a, b, c), (b, a, c), (c, a, b)):
            adjacent.setdefault(i, set()).update((j, k))
    found, queue = {seed}, [seed]
    while queue:
        for i in adjacent[queue.pop()]:
            if i not in found:
                found.add(i)
                queue.append(i)
    return sorted(found)


def prepare_gm(original):
    assert sha(original) == '901358b19edd12162360a09c89dd6a5da19365373425eb8a059d83435cb5c423'
    head, obj_at, tri_at, buff_at, vert_at, triangles, vertices = load_gm(original)
    blind = component(triangles, 344)
    healthy = component(triangles, 303)
    assert len(blind) == len(healthy) == 41 and set(blind).isdisjoint(healthy)
    assert all(struct.unpack_from('<f', vertices[i])[0] > 0.025 for i in blind)
    # Independent eye spheres share one UV patch. Map only the character's left
    # eye to the author-generated second eye in a proven unused 48px square.
    source_bounds = (0.64973176, 0.64291704, 0.7426780, 0.7409735)
    for i in blind:
        v = bytearray(vertices[i])
        u, t = struct.unpack_from('<2f', v, 36)
        u, t = u % 1, t % 1
        assert source_bounds[0] - 1e-6 <= u <= source_bounds[2] + 1e-6
        assert source_bounds[1] - 1e-6 <= t <= source_bounds[3] + 1e-6
        u = (276 + (u - source_bounds[0]) / (source_bounds[2] - source_bounds[0]) * 40) / 1024
        t = (948 + (t - source_bounds[1]) / (source_bounds[3] - source_bounds[1]) * 40) / 1024 - 1
        struct.pack_into('<2f', v, 36, u, t)
        vertices[i] = bytes(v)
    assert all(vertices[i] == original[vert_at + i * 44:vert_at + (i + 1) * 44] for i in healthy)

    # A separate coarse beard shell rests over existing facial beard texture.
    # The original jaw, lips and neck are left byte-identical. All shell vertices
    # reuse the chin's exact packed head influence; topology stays one material.
    template = struct.unpack('<3ffI3fI2f', vertices[3184])
    assert template[3] == 1.0
    start = len(vertices)
    rings = ((1.628, .048, .041), (1.603, .058, .054),
             (1.559, .043, .046), (1.525, .018, .023))
    points = []
    for row, (y, rx, rz) in enumerate(rings):
        for col in range(12):
            a = math.tau * col / 12
            rough = 1 + (0.04 * math.sin(col * 2.4 + row))
            x = .010 + rx * math.sin(a) * rough
            z = .089 + rz * math.cos(a) * rough
            # A closed shell's rear stays under the jaw/neck instead of wrapping
            # hair onto clothing. The front carries the visible coarse taper.
            taper_y = y + (0.008 * math.cos(col * 2.8) if row == 3 else 0)
            nx, nz = math.sin(a), math.cos(a)
            previous, following = rings[max(0, row - 1)], rings[min(3, row + 1)]
            ny = -((following[1] - previous[1]) * nx * nx + (following[2] - previous[2]) * nz * nz) / (following[0] - previous[0])
            length = math.sqrt(nx * nx + nz * nz + ny * ny)
            # Sample only the authored coarse hair island, never lips or skin.
            u = .52 + .05 * math.sin(a)
            t = (.72, .77, .82, .88)[row] - 1
            points.append((x, taper_y, z))
            vertices.append(struct.pack('<3ffI3fI2f', x, taper_y, z, template[3],
                            template[4], nx / length, ny / length, nz / length,
                            template[8], u, t))
    added = []
    for row in range(3):
        for col in range(12):
            a = start + row * 12 + col
            b = start + row * 12 + (col + 1) % 12
            c, d = a + 12, b + 12
            added.extend(((a, c, b), (b, c, d)))
    # Winding must point away from the shell's central axis.
    for tri in added:
        a, b, c = [points[i - start] for i in tri]
        ab, ac = [b[j] - a[j] for j in range(3)], [c[j] - a[j] for j in range(3)]
        normal = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
        radial = (sum(p[0] for p in (a, b, c)) / 3 - .010, 0,
                  sum(p[2] for p in (a, b, c)) / 3 - .089)
        assert sum(normal[j] * radial[j] for j in range(3)) > 0

    # Close both ends so a low or tilted camera cannot see an open tube.
    for row, normal_y in ((0, 1.0), (3, -1.0)):
        center = len(vertices)
        y = rings[row][0]
        vertices.append(struct.pack('<3ffI3fI2f', .010, y, .089, template[3],
                        template[4], 0, normal_y, 0, template[8], .52,
                        (.72, .77, .82, .88)[row] - 1))
        for col in range(12):
            a = start + row * 12 + col
            b = start + row * 12 + (col + 1) % 12
            added.append((center, a, b) if row == 0 else (center, b, a))

    texture_name = b'RedBeard.tga\0'
    old_names_end = 72 + head[2]
    old_offsets_end = old_names_end + head[3] * 4
    old_texture_end = old_offsets_end + 4
    head[2] += len(texture_name)
    head[3] += 1
    head[9] += len(added)
    prefix = (struct.pack('<11i7f', *head) + original[72:old_names_end] + texture_name
              + original[old_names_end:old_offsets_end] + struct.pack('<i', head[2] - len(texture_name))
              + struct.pack('<i', head[2] - len(texture_name)))
    middle = bytearray(original[old_texture_end:tri_at])
    new_obj = obj_at - old_texture_end
    old_obj = list(struct.unpack_from('<3i4f6i8i4ii', middle, new_obj))
    old_obj[8] += len(added)
    old_obj[10] += 50
    struct.pack_into('<3i4f6i8i4ii', middle, new_obj, *old_obj)
    result = (prefix + middle + original[tri_at:buff_at]
              + b''.join(struct.pack('<3H', *tri) for tri in added)
              + struct.pack('<2i', 4, len(vertices) * 44) + b''.join(vertices)
              + original[vert_at + 4267 * 44:])
    assert len(added) == 96 and len(vertices) == 4317
    # The appended shell remains entirely inside the shipped object's bounds.
    cx, cy, cz, radius = old_obj[3:7]
    assert all(sum((p[j] - (cx, cy, cz)[j]) ** 2 for j in range(3)) <= radius * radius for p in points)
    return result, {'original_vertices': 4267, 'added_beard_vertices': 50,
                    'original_triangles': 4506, 'added_beard_triangles': 96,
                    'blind_eye_vertices': blind, 'healthy_eye_vertices': healthy,
                    'beard_head_influence': {'weight': template[3], 'packed_bone_ids': template[4]},
                    'blind_eye_atlas_rect': [272, 944, 320, 992]}


def prepare_tx(source):
    im = Image.open(io.BytesIO(source)).convert('RGBA')
    assert im.width == im.height and im.getextrema()[3] == (255, 255)
    # Native UV assembly, not repaint: preserve the authored master, extract its
    # milky-eye sprite and restore its accidental overlap with other samplers.
    assert im.size == (1254, 1254)
    eye = im.crop((365, 1130, 444, 1214)).resize((48, 48), Image.Resampling.LANCZOS)
    im = im.resize((1024, 1024), Image.Resampling.LANCZOS)
    unpacked = im.copy()
    original = Image.open(ROOT / 'original.png').convert('RGBA')
    assert original.size == im.size
    restore = (296, 920, 363, 1000)
    destination = (272, 944, 320, 992)
    im.paste(original.crop(restore), restore)
    im.paste(eye, destination)
    protected = im.copy()
    for rect in (restore, destination):
        protected.paste(unpacked.crop(rect), rect)
    assert ImageChops.difference(protected.convert('RGB'), unpacked.convert('RGB')).getbbox() is None
    im.save(ROOT / 'packed.png')
    # Match shipped Hugtorp: opaque DXT1, five mip levels, 24byte Storm header.
    chunks = []
    for _ in range(5):
        output = io.BytesIO()
        im.save(output, format='DDS', pixel_format='DXT1')
        dds = output.getvalue()
        assert dds[84:88] == b'DXT1'
        chunks.append(dds[128:])
        im = im.resize((im.width // 2, im.height // 2), Image.Resampling.LANCZOS)
    data = struct.pack('<6I', 0, 1024, 1024, 5, int.from_bytes(b'DXT1', 'little'), len(chunks[0])) + b''.join(chunks)
    restored = Image.frombytes('RGBA', (1024, 1024), chunks[0], 'bcn', (1, 'DXT1'))
    assert restored.getextrema()[3] == (255, 255)
    return data


def render_portrait(gm, tx):
    """Compile a stock-shaped portrait from the frozen skinned GM/TX bytes."""
    assert sha(gm) == 'dd7ed486cd101e3bba399ec23f597eb87fc4ae748586e5f8862b0864bf4b0412'
    assert sha(tx) == '919d3be40e12238899ddf2cee4a1734acfc97ada6a07951cdade94b825e4a1fd'
    _, _, _, _, _, triangles, raw_vertices = load_gm(gm, original=False)
    vertices = [struct.unpack('<3ffI3fI2f', vertex) for vertex in raw_vertices]
    header = struct.unpack_from('<6I', tx)
    texture = Image.frombytes('RGBA', (header[1], header[2]), tx[24:24 + header[5]], 'bcn', (1, 'DXT1')).convert('RGB')
    pixels = texture.load()
    side, angle, scale = 512, -.15, 500 / .41
    cosine, sine = math.cos(angle), math.sin(angle)
    projected = []
    for v in vertices:
        x, y, z, _, _, nx, ny, nz, _, u, t = v
        x, z = x * cosine + z * sine, -x * sine + z * cosine
        nx, nz = nx * cosine + nz * sine, -nx * sine + nz * cosine
        projected.append(((x + .004) * scale + side / 2, 490 - (y - 1.465) * scale, z, nx, ny, nz, u, t))
    image = Image.new('RGB', (side, side))
    output = image.load()
    depth = [-float('inf')] * (side * side)
    for y in range(side):
        for x in range(side):
            light = max(0, 1 - ((x / side - .34) ** 2 + (y / side - .32) ** 2))
            output[x, y] = (int(10 + 10 * light), int(18 + 13 * light), int(26 + 18 * light))
    for ids in triangles:
        a, b, c = [projected[i] for i in ids]
        x0, x1 = max(0, math.floor(min(a[0], b[0], c[0]))), min(side - 1, math.ceil(max(a[0], b[0], c[0])))
        y0, y1 = max(0, math.floor(min(a[1], b[1], c[1]))), min(side - 1, math.ceil(max(a[1], b[1], c[1])))
        if x0 > x1 or y0 > y1:
            continue
        denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(denominator) < 1e-8:
            continue
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                wa = ((b[1] - c[1]) * (x + .5 - c[0]) + (c[0] - b[0]) * (y + .5 - c[1])) / denominator
                wb = ((c[1] - a[1]) * (x + .5 - c[0]) + (a[0] - c[0]) * (y + .5 - c[1])) / denominator
                wc = 1 - wa - wb
                z = wa * a[2] + wb * b[2] + wc * c[2]
                if wa < 0 or wb < 0 or wc < 0 or z <= depth[y * side + x]:
                    continue
                depth[y * side + x] = z
                u, t = [sum(w * v[i] for w, v in ((wa, a), (wb, b), (wc, c))) for i in (6, 7)]
                txp, typ = (u % 1) * texture.width - .5, (t % 1) * texture.height - .5
                ix, iy = math.floor(txp), math.floor(typ)
                fx, fy = txp - ix, typ - iy
                color = [sum(pixels[(ix + dx) % texture.width, (iy + dy) % texture.height][channel] * weight
                             for dx, dy, weight in ((0, 0, (1 - fx) * (1 - fy)), (1, 0, fx * (1 - fy)),
                                                   (0, 1, (1 - fx) * fy), (1, 1, fx * fy)))
                         for channel in range(3)]
                normal = [sum(w * v[i] for w, v in ((wa, a), (wb, b), (wc, c))) for i in (3, 4, 5)]
                length = math.sqrt(sum(n * n for n in normal))
                lighting = .72 + .28 * max(0, sum(n * light / max(length, 1e-8) for n, light in zip(normal, (-.35, .25, .9028))))
                output[x, y] = tuple(max(0, min(255, round(value * lighting))) for value in color)
    # Same frame role as the stock portrait; no text or synthetic face pixels.
    for offset, color in ((0, (67, 48, 27)), (1, (213, 171, 104)), (2, (184, 144, 85)), (3, (106, 76, 40))):
        for i in range(offset, side - offset):
            output[i, offset] = output[i, side - 1 - offset] = color
            output[offset, i] = output[side - 1 - offset, i] = color
    return image.convert('RGBA')


def encode_portrait(master, size):
    im = master.resize((size, size), Image.Resampling.LANCZOS)
    mips, chunks = (1 if size == 64 else int(math.log2(size)) + 1), []
    for _ in range(mips):
        chunks.append(im.tobytes('raw', 'BGRA'))
        im = im.resize((max(1, im.width // 2), max(1, im.height // 2)), Image.Resampling.LANCZOS)
    return struct.pack('<6I', 0, size, size, mips, 22 if size == 64 else 21, len(chunks[0])) + b''.join(chunks)


def main():
    original = (ROOT / 'original.gm').read_bytes()
    source = (ROOT / 'generated.png').read_bytes()
    original_png = (ROOT / 'original.png').read_bytes()
    original_tx = (ROOT / 'original.tga.tx').read_bytes()
    assert sha(original_tx) == 'dfd982ca735f3e73a93785fbb37901d6b50ff4f8508a9829df45759b9519a66d'
    assert sha(original_png) == '599913ee6ff0952dd75cad2300d28863f3237c1f46fd558146e5d18e30f4b6b4'
    gm, properties = prepare_gm(original)
    tx = prepare_tx(source)
    (ROOT / 'RedBeard.gm').write_bytes(gm)
    (ROOT / 'RedBeard.tga.tx').write_bytes(tx)
    files = {
        'RESOURCE/MODELS/Characters/RedBeard.gm': {
            'source': 'original.gm', 'source_sha256': sha(original),
            'prepared': 'RedBeard.gm', 'sha256': sha(gm),
            'consumer': 'Great Recipe RedBeard: dream, captive, duel, real captain',
            'animation': 'man (existing Hugtorp skeleton and packed head weights)',
            'preparation': 'Append same-material head-weighted beard shell; remap only left eye component; preserve all original geometry/weights and all other UVs.',
            **properties,
        },
        'RESOURCE/Textures/Characters/RedBeard.tga.tx': {
            'source': 'generated.png', 'source_sha256': sha(source),
            'prepared': 'RedBeard.tga.tx', 'sha256': sha(tx),
            'consumer': 'RESOURCE/MODELS/Characters/RedBeard.gm',
            'dimensions': [1024, 1024], 'source_dimensions': list(Image.open(io.BytesIO(source)).size),
            'packed_atlas': 'packed.png', 'packed_atlas_sha256': sha((ROOT / 'packed.png').read_bytes()),
            'supporting_inputs': {'original.png': sha(original_png), 'original.tga.tx': sha(original_tx)},
            'format': 'DXT1', 'mip_count': 5, 'alpha': 'opaque',
            'generation': 'built-in imagegen edit of original Hugtorp atlas',
            'preparation': 'Uniform square downsample; extract generated blind-eye sprite into unused272,944..320,992; restore former accidental296,920..363,1000 footprint from original; all remaining pixels unchanged by packing. Five opaqueDXT1mips matching Hugtorp.',
        },
    }
    portrait = render_portrait(gm, tx)
    portrait.save(ROOT / 'portrait.png')
    portrait_source = (ROOT / 'portrait.png').read_bytes()
    for size in (64, 128, 256):
        name = f'face_{FACE_ID}-{size}.tga.tx'
        data = encode_portrait(portrait, size)
        (ROOT / name).write_bytes(data)
        files[f'RESOURCE/Textures/INTERFACES/PORTRAITS/{size}/face_{FACE_ID}.tga.tx'] = {
            'source': 'portrait.png', 'source_sha256': sha(portrait_source),
            'prepared': name, 'sha256': sha(data),
            'consumer': 'RedBeard FaceId2006: spyglass, character UI, cargo transfer and list portraits',
            'dimensions': [size, size], 'source_dimensions': [512, 512],
            'format': 'X8R8G8B8' if size == 64 else 'A8R8G8B8',
            'mip_count': 1 if size == 64 else int(math.log2(size)) + 1,
            'alpha': 'opaque', 'face_id': FACE_ID,
            'model_sha256': sha(gm), 'diffuse_sha256': sha(tx),
            'generation': 'Deterministic CPU3Dscene from finalRedBeardGM and decodednativeTX; no raster repaint.',
            'preparation': 'Uniform head/shoulders portrait downsample; stock native rawBGRA portrait format and mip counts.',
            'picture_group': 'FACE128_2006',
        }
    (ROOT / 'manifest.json').write_text(json.dumps({'version': 1, 'files': files}, indent=2) + '\n')
    print(json.dumps({name: row['sha256'] for name, row in files.items()}))


if __name__ == '__main__':
    main()
