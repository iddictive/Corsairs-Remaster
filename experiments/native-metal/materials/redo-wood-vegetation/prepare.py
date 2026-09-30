#!/usr/bin/env python3
"""Prepare source-only wood candidates; never writes to a game runtime."""

import hashlib
import io
import json
import struct
import urllib.request
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent
STORM = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-storm/.cache/runtime/RESOURCE/Textures')
METAL = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/.cache/runtime/RESOURCE/Textures')
ASSET = 'wooden_planks'
PAGE = 'https://polyhaven.com/a/wooden_planks'
URL = 'https://dl.polyhaven.org/file/ph-assets/Textures/png/2k/wooden_planks/wooden_planks_diff_2k.png'
LICENSE = 'https://polyhaven.com/license'
AUTHOR = 'Charlotte Baglioni (photography); Dario Barresi (processing)'
TARGETS = {
    'woodDamU1.tga.tx': {
        'size': (256, 128),
        'consumers': [
            'Locations/decks/deckLowVSBig/deckLowVSBig.gm',
            'Locations/decks/deckLowVSMedium/DeckLowVSMedium.gm',
            'Locations/decks/deckMediumVSBig/deckMediumVSBig.gm',
            'Locations/decks/deckQuestFull/DeckMediumVSBig.gm',
        ],
        'decision': '2x1 square tile layout preserves the original 2:1 UV aspect and board scale; uniform source tile resize, no crop or stretch.',
    },
    'woodRoundU1.tga.tx': {
        'size': (256, 256),
        'consumers': [
            'Locations/decks/deckBig/deckBig.gm',
            'Locations/decks/deckLow/deckLow.gm',
            'Locations/decks/deckLowVSBig/deckLowVSBig.gm',
            'Locations/decks/deckLowVSMedium/DeckLowVSMedium.gm',
            'Locations/decks/deckLowVSMedium/DeckLowVSMedium_locators.gm',
            'Locations/decks/deckMedium/deckMedium.gm',
            'Locations/decks/deckMedium/deckMedium_locators.gm',
            'Locations/decks/deckMediumVSBig/deckMediumVSBig.gm',
            'Locations/decks/deckQuestFull/DeckMediumVSBig.gm',
        ],
        'decision': 'Uniform square resize to the original 1:1 UV aspect; no crop, repack, or nonuniform stretch.',
    },
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def decode(data):
    _, w, h, _, fmt, level = struct.unpack('<6I', data[:24])
    assert fmt == int.from_bytes(b'DXT1', 'little')
    return Image.frombytes('RGBA', (w, h), data[24:24 + level], 'bcn', (1, 'DXT1'))

def encode_level(image):
    w, h = image.size
    padded = Image.new('RGBA', (max(4, w), max(4, h)))
    padded.paste(image)
    out = io.BytesIO()
    padded.save(out, format='DDS', pixel_format='DXT1')
    payload = out.getvalue()[128:]
    assert len(payload) == ((w + 3) // 4) * ((h + 3) // 4) * 8
    assert Image.frombytes('RGBA', padded.size, payload, 'bcn', (1, 'DXT1')).getextrema()[3] == (255, 255)
    return payload

def encode(image, count):
    levels = []
    mip = image
    for _ in range(count):
        levels.append(encode_level(mip))
        mip = mip.resize((max(1, mip.width // 2), max(1, mip.height // 2)), Image.Resampling.LANCZOS)
    return struct.pack('<6I', 0, image.width, image.height, count, int.from_bytes(b'DXT1', 'little'), len(levels[0])) + b''.join(levels)

def make_image(source, size):
    w, h = size
    tile = source.resize((h, h), Image.Resampling.LANCZOS)
    out = Image.new('RGBA', size)
    for x in range(0, w, h):
        out.paste(tile, (x, 0))
    return out

def main():
    print('No accepted wood/vegetation replacement: all evaluated low-resolution mappings fail identity-preservation criteria; see manifest.json.')
    return
    (ROOT / 'cache').mkdir(exist_ok=True)
    (ROOT / 'prepared').mkdir(exist_ok=True)
    (ROOT / 'previews').mkdir(exist_ok=True)
    source_path = ROOT / 'cache' / 'wooden_planks_diff_2k.png'
    if not source_path.exists():
        urllib.request.urlretrieve(URL, source_path)
    source_bytes = source_path.read_bytes()
    source = Image.open(source_path).convert('RGBA')
    assert source.size == (2048, 2048) and source.getextrema()[3] == (255, 255)
    records = []
    for target, spec in TARGETS.items():
        original_path = STORM / target
        original_bytes = original_path.read_bytes()
        metal_bytes = (METAL / target).read_bytes()
        original = decode(original_bytes)
        header = struct.unpack('<6I', original_bytes[:24])
        assert original_bytes == metal_bytes
        image = make_image(source, spec['size'])
        data = encode(image, header[3])
        output = ROOT / 'prepared' / target
        output.write_bytes(data)
        decoded = decode(data)
        assert decoded.size == spec['size'] and decoded.getextrema()[3] == (255, 255)
        original.save(ROOT / 'previews' / (target.replace('.tga.tx', '-original.png')))
        decoded.save(ROOT / 'previews' / (target.replace('.tga.tx', '-replacement.png')))
        records.append({
            'target': target,
            'original': {'path': str(original_path), 'sha256': sha(original_bytes), 'dimensions': list(original.size), 'format': 'DXT1', 'mip_count': header[3], 'consumer_evidence': spec['consumers']},
            'source': {'page': PAGE, 'direct_map_url': URL, 'author': AUTHOR, 'license': 'CC0-1.0', 'license_url': LICENSE, 'sha256': sha(source_bytes), 'dimensions': list(source.size), 'map': 'diffuse-only'},
            'output': {'path': str(output.relative_to(ROOT)), 'sha256': sha(data), 'dimensions': list(image.size), 'format': 'DXT1', 'mip_count': header[3], 'mip_chain': [[max(1, image.width >> i), max(1, image.height >> i)] for i in range(header[3])], 'alpha': 'opaque; source and all decoded mips alpha=255', 'color_space': 'sRGB diffuse; no linearization'},
            'role': 'opaque wood/deck diffuse',
            'adaptation_uv_decision': spec['decision'],
            'consumer_evidence': 'exact GM texture-table basename/path token; mapped in wood-decks-hulls inventory',
            'runtime_status': 'prepared lane candidate only; not staged or runtime-visible',
        })
    (ROOT / 'manifest.json').write_text(json.dumps({'schema': 1, 'family': 'wood/decks/hulls', 'source_runtime': str(STORM), 'comparison_runtime': str(METAL), 'records': records, 'vegetation': {'disposition': 'keep-original', 'evidence': 'Mapped vegetation atlases are consumer-specific alpha/sprite assets; no explicit reusable source preserves exact silhouettes, atlas UV layout, alpha-test semantics, and identity.'}}, indent=2) + '\n')
    print(json.dumps({'source_sha256': sha(source_bytes), 'outputs': [(r['target'], r['output']['sha256']) for r in records]}, indent=2))

if __name__ == '__main__':
    main()
