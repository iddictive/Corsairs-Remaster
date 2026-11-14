"""Offline diagnostic only: render imagegen atlas on unmodified Hugtorp GM."""
import io
import struct
import sys
from pathlib import Path

from PIL import Image

here = Path(__file__).resolve().parent
root = here.parents[2]
source = (root / 'src/assets/characters/red-beard/prepare.py').read_text()
source = source.replace("assert sha(gm) == 'dd7ed486cd101e3bba399ec23f597eb87fc4ae748586e5f8862b0864bf4b0412'", "assert sha(gm) == '901358b19edd12162360a09c89dd6a5da19365373425eb8a059d83435cb5c423'")
source = source.replace("assert sha(tx) == '919d3be40e12238899ddf2cee4a1734acfc97ada6a07951cdade94b825e4a1fd'", "assert len(tx) > 24")
source = source.replace('load_gm(gm, original=False)', 'load_gm(gm, original=True)')
module = {'__file__': str(root / 'src/assets/characters/red-beard/prepare.py'), '__name__': 'diagnostic'}
exec(compile(source, module['__file__'], 'exec'), module)
atlas = Path(sys.argv[1]) if len(sys.argv) > 1 else here / 'imagegen-atlas-candidate.png'
im = Image.open(atlas).convert('RGBA').resize((1024, 1024), Image.Resampling.LANCZOS)
buf = io.BytesIO()
im.save(buf, format='DDS', pixel_format='DXT1')
dds = buf.getvalue()
assert dds[84:88] == b'DXT1'
data = struct.pack('<6I', 0, 1024, 1024, 1, int.from_bytes(b'DXT1', 'little'), len(dds)-128) + dds[128:]
gm = (root / 'src/assets/characters/red-beard/original.gm').read_bytes()
module['render_portrait'](gm, data).save(atlas.with_name(atlas.stem + '-preview.png'))
