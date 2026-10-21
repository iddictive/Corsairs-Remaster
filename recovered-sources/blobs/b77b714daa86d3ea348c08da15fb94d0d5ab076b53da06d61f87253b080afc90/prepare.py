#!/usr/bin/env python3
import hashlib, io, json, struct
from pathlib import Path
from PIL import Image, __version__ as pillow_version

HERE = Path(__file__).resolve().parent
SOURCE = HERE / 'source/coast_sand_01_diff_4k.png'
TARGET = HERE / 'prepared/Sandtile.tga.tx'
PAGE = 'https://polyhaven.com/a/coast_sand_01'
DOWNLOAD = 'https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/coast_sand_01/coast_sand_01_diff_4k.png'
LICENSE = 'https://polyhaven.com/license'
def sha(b): return hashlib.sha256(b).hexdigest()
def encode(im):
    out = io.BytesIO()
    padded = Image.new('RGBA', (max(4, im.width), max(4, im.height)))
    padded.paste(im.convert('RGBA'))
    padded.save(out, format='DDS', pixel_format='DXT1')
    d = out.getvalue(); assert d[84:88] == b'DXT1'
    return d[128:]

source_bytes = SOURCE.read_bytes()
source = Image.open(SOURCE).convert('RGBA')
assert source.getextrema()[3] == (255, 255)
tile = source.resize((1024, 1024), Image.Resampling.LANCZOS)
image = Image.new('RGBA', (2048, 1024)); image.paste(tile, (0, 0)); image.paste(tile, (1024, 0))
levels = []; mip = image
while True:
    levels.append(encode(mip))
    if mip.size == (1, 1): break
    mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
data = struct.pack('<6I', 0, image.width, image.height, len(levels), int.from_bytes(b'DXT1', 'little'), len(levels[0])) + b''.join(levels)
TARGET.write_bytes(data)
decoded = Image.frombytes('RGBA', image.size, data[24:24 + len(levels[0])], 'bcn', (1, 'DXT1'))
assert decoded.getextrema()[3] == (255, 255)
meta = {'target': 'Sandtile.tga.tx', 'role': 'seamless diffuse ground/sand', 'source_asset_id': 'coast_sand_01', 'source_page': PAGE, 'download_url': DOWNLOAD, 'license': 'CC0-1.0', 'license_url': LICENSE, 'author': 'Rob Tuytel', 'source_sha256': sha(source_bytes), 'source_dimensions': list(source.size), 'adaptation': 'uniform resize to 1024x1024 then repeat twice horizontally; no crop, repaint, or generative content', 'output_path': str(TARGET.relative_to(HERE)), 'output_sha256': sha(data), 'dimensions': [image.width, image.height], 'format': 'DXT1', 'format_code': 827611204, 'mip_count': len(levels), 'mip_chain_dimensions': [[max(1, image.width >> i), max(1, image.height >> i)] for i in range(len(levels))], 'alpha': 'opaque; source and every decoded mip alpha=255', 'consumer_evidence': 'See inventory.json: exact Sandtile.tga references in GM corpus', 'runtime_status': 'prepared candidate only; not staged, launched, or runtime-visible', 'pillow_version': pillow_version}
(HERE / 'candidate.json').write_text(json.dumps(meta, indent=2, sort_keys=True) + '\n')
print(json.dumps(meta, sort_keys=True))
