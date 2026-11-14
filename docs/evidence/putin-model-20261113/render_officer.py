"""Offline render of officer_1 with original or candidate atlas; no game process."""
import io
import struct
import sys
from pathlib import Path
from PIL import Image

here = Path(__file__).resolve().parent
root = here.parents[2]
base = root / 'experiments/native-metal/.cache/runtime/RESOURCE'
model = sys.argv[1] if len(sys.argv) > 1 else 'officer_1'
atlas = sys.argv[2] if len(sys.argv) > 2 else None
angle = float(sys.argv[3]) if len(sys.argv) > 3 else -.15
textures = {'officer_1': 'mr01xxxxx', 'barmen_5': 'barmen_5', 'barmen_8': 'barmen_8', 'officer_5': 'mr05x'}
gm_path = Path(sys.argv[4]) if len(sys.argv) > 4 else base / f'MODELS/Characters/{model}.gm'
gm = gm_path.read_bytes()
original_tx = (base / f'Textures/Characters/{textures[model]}.tga.tx').read_bytes()
h = struct.unpack_from('<6I', original_tx)
assert h[1:3] == (1024, 1024) and h[4] == int.from_bytes(b'DXT1', 'little')
original = Image.frombytes('RGBA', (1024, 1024), original_tx[24:24+h[5]], 'bcn', (1, 'DXT1'))
if atlas is None:
    original.save(here / f'{model}-atlas.png')
    texture = original_tx
    output = here / f'{model}-original-preview.png'
else:
    texture_image = Image.open(atlas).convert('RGBA')
    if texture_image.width != 2048: texture_image = texture_image.resize((1024, 1024), Image.Resampling.LANCZOS)
    buf = io.BytesIO(); texture_image.save(buf, format='DDS', pixel_format='DXT1')
    dds = buf.getvalue(); assert dds[84:88] == b'DXT1'
    texture = struct.pack('<6I', 0, texture_image.width, texture_image.height, 1, h[4], len(dds)-128) + dds[128:]
    prefix = 'morph' if len(sys.argv) > 4 else model
    output = here / (f'{prefix}-candidate-preview.png' if angle == -.15 else f'{prefix}-candidate-angle-{angle:g}.png')

src_path = root / 'src/assets/characters/red-beard/prepare.py'
source = src_path.read_text()
source = source.replace("assert sha(gm) == 'dd7ed486cd101e3bba399ec23f597eb87fc4ae748586e5f8862b0864bf4b0412'", "assert len(gm) > 72")
source = source.replace("assert sha(tx) == '919d3be40e12238899ddf2cee4a1734acfc97ada6a07951cdade94b825e4a1fd'", "assert len(tx) > 24")
source = source.replace('side, angle, scale = 512, -.15, 500 / .41', f'side, angle, scale = 512, {angle!r}, 500 / .41')
module = {'__file__': str(src_path), '__name__': 'diagnostic'}
exec(compile(source, str(src_path), 'exec'), module)
def load(data, original=False):
    head = struct.unpack_from('<11i7f', data)
    assert head[4:9] == (1, 1, 0, 5, 1) and head[10] == 1
    obj = 72 + head[2] + 4*head[3] + 4*head[4] + 56*head[5] + 80*head[6] + 108*head[7]
    tris_at = obj + head[8]*104; buff = tris_at + head[9]*6
    typ, size = struct.unpack_from('<2i', data, buff)
    assert typ == 4 and size % 44 == 0
    verts = buff + 8
    triangles = list(struct.iter_unpack('<3H', data[tris_at:buff]))
    vertices = [data[i:i+44] for i in range(verts, verts+size, 44)]
    return head, obj, tris_at, buff, verts, triangles, vertices
module['load_gm'] = load
module['render_portrait'](gm, texture).save(output)
print(output)
