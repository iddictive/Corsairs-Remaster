#!/usr/bin/env python3
"""Read installed Storm skies and write bounded, reproducible inspection evidence.

No game/source mutation. Raw TX decoding reuses the installed Pillow raw decoder;
layout validation comes from the repository's canonical mip probe.
"""
from __future__ import annotations

import hashlib
import importlib.util
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, __version__ as pillow_version

ROOT = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
APP = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources')
OUT = Path(__file__).resolve().parent
SKIES = APP / 'RESOURCE/Textures/WEATHER/SKIES'
PROGRAM = APP / 'PROGRAM'
PROBE = ROOT / 'experiments/native-metal/texture_loader_mip_probe.py'
MANAGED = ROOT / 'src/gameplay/PROGRAM/weather/WhrSky.c'
FACES = ('FR', 'LF', 'BK', 'RT', 'UP')


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def uncomment(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\r\n]*', '', text)


def rows(path: Path) -> list[dict]:
    source = path.read_bytes().decode('latin1')
    clean = uncomment(source)
    found = []
    for match in re.finditer(r'Weathers\[n\]\.id\s*=', clean):
        start = match.start()
        next_id = re.search(r'Weathers\[n\]\.id\s*=', clean[match.end():])
        end = match.end() + next_id.start() if next_id else len(clean)
        attrs = {key: value.strip() for key, value in re.findall(
            r'Weathers\[n\]\.([\w.]+)\s*=\s*([^;\r\n]+)', clean[start:end])}
        found.append({'source': str(path), 'id': attrs.get('id'), 'attributes': attrs})
    return found


def source_usage() -> dict:
    process = subprocess.run(['rg', '--json', '-i', '--glob', '*.c', 'skies', str(PROGRAM)],
                             capture_output=True, text=True, check=True)
    references = []
    for line in process.stdout.splitlines():
        event = json.loads(line)
        if event['type'] != 'match':
            continue
        data = event['data']
        references.append({'path': data['path']['text'], 'line': data['line_number'],
                           'text': data['lines'].get('text', '').strip()})
    roots = [str(path) for path in (APP / 'RESOURCE/Textures').rglob('*')
             if path.is_dir() and path.name.lower() == 'skies']
    return {'all_installed_program_skies_references': references,
            'case_insensitive_skies_directories': roots}


def duplicate_groups(textures: list[dict]) -> dict:
    by_hash = defaultdict(list)
    sets = defaultdict(dict)
    for texture in textures:
        by_hash[texture['sha256']].append(texture['relative_path'])
        sets[texture['set']][texture['file_name'].lower()] = texture['sha256']
    by_set = defaultdict(list)
    for name, faces in sets.items():
        by_set[tuple(sorted(faces.items()))].append(name)
    return {'duplicate_texture_groups': [group for group in by_hash.values() if len(group) > 1],
            'identical_full_set_groups': [group for group in by_set.values() if len(group) > 1]}


def main() -> None:
    spec = importlib.util.spec_from_file_location('tx_mip_probe', PROBE)
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)
    init = PROGRAM / 'weather/WhrInit.c'
    init_bytes = init.read_bytes()
    active_calls = re.findall(r'n\s*=\s*Whr_Init(\w+)\(n\)', uncomment(init_bytes.decode('latin1')))
    all_rows = []
    active_rows = []
    init_dir = PROGRAM / 'weather/Init'
    for path in sorted(init_dir.glob('*.c')):
        row_list = rows(path)
        included = path.name.lower() in {f'{name}.c'.lower() for name in active_calls}
        for row in row_list:
            row['active_initialization'] = included
            all_rows.append(row)
    for name in active_calls:
        active_rows.extend(rows(init_dir / f'{name}.c'))
    hour_map = {}
    for row in active_rows:
        a = row['attributes']
        if 'Hour.Min' not in a or a.get('Skip', 'false') == 'true' or a.get('Storm', 'false') == 'true':
            continue
        hour = int(float(a['Hour.Min']))
        if hour == 24:
            continue
        hour_map[str(hour)] = a.get('Sky.Dir', '').strip('"').replace('\\', '/').strip('/')

    per_set = {path.name: [] for path in SKIES.iterdir() if path.is_dir()}
    manifest = {
        'command': f'python3 {Path(__file__).resolve()}',
        'python': subprocess.check_output(['python3', '--version'], text=True).strip(),
        'pillow': pillow_version,
        'repository_head': subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip(),
        'input_root': str(SKIES),
        'decode': 'Pillow Image.frombytes RGBA raw BGRA for format21; RGB thumbnails preserve RGB regardless of alpha; no orientation/color/exposure transform',
        'mip_layout_owner': {'path': str(PROBE), 'sha256': digest(PROBE.read_bytes())},
        'managed_sky_owner': {'path': str(MANAGED), 'sha256': digest(MANAGED.read_bytes())},
        'installed_init': {'path': str(init), 'sha256': digest(init_bytes)},
        'active_init_calls_in_order': active_calls,
        'active_hour_to_sky_dir': dict(sorted(hour_map.items(), key=lambda item: int(item[0]))),
        'all_init_rows_including_inactive_files': all_rows,
        'sets': per_set,
        'textures': [],
        'sheets': [],
    }
    for path in sorted(SKIES.rglob('*')):
        if not path.is_file():
            continue
        data = path.read_bytes()
        flags, width, height, mips, fmt, top_size = probe.HEADER.unpack_from(data)
        levels, required = probe.plan(data[:24], len(data) - 24)
        assert fmt == 21, f'unexpected texture format {fmt}: {path}'
        image = Image.frombytes('RGBA', (width, height), data[24:24 + top_size], 'raw', 'BGRA')
        alpha_range = list(image.getchannel('A').getextrema())
        thumb = image.convert('RGB')
        thumb.thumbnail((400, 400), Image.Resampling.LANCZOS)
        del image
        relative = path.relative_to(SKIES).as_posix()
        row = {'path': str(path), 'relative_path': relative, 'set': path.parent.name,
               'file_name': path.name, 'sha256': digest(data), 'bytes': len(data),
               'width': width, 'height': height, 'flags': flags, 'format': fmt,
               'mips': mips, 'top_level_bytes': top_size, 'mip_sizes': levels,
               'trailing_bytes': len(data) - 24 - required, 'alpha_range': alpha_range}
        manifest['textures'].append(row)
        per_set[path.parent.name].append({'metadata': row, 'thumbnail': thumb})
        del data

    font_path = '/System/Library/Fonts/Supplemental/Arial.ttf'
    font = ImageFont.truetype(font_path, 21)
    small = ImageFont.truetype(font_path, 17)
    groups = [(f'hours-{start:02d}-{start+7:02d}', [f'{h:02d}' for h in range(start, start+8)])
              for start in (1, 9, 17)]
    groups.append(('storms-01-04', [f'storm{h:02d}' for h in range(1,5)]))
    for title, names in groups:
        sheet = Image.new('RGB', (2350, 90 + len(names)*440), (28, 28, 30))
        draw = ImageDraw.Draw(sheet)
        draw.text((15, 10), f'Installed original skies: {title}; all five faces; aspect ratios retained', font=font, fill='white')
        for col, face in enumerate(FACES):
            draw.text((255 + col*415, 50), face, font=font, fill='white')
        for row_index, name in enumerate(names):
            y = 90 + row_index*440
            draw.text((15, y+8), name, font=font, fill='white')
            hours = [hour for hour, directory in manifest['active_hour_to_sky_dir'].items()
                     if directory.lower().endswith('/'+name.lower())]
            draw.text((15, y+39), 'hours: '+(','.join(hours) if hours else 'excluded'), font=small, fill=(200,200,200))
            users = [r for r in active_rows if r['attributes'].get('Sky.Dir','').strip('"').rstrip('\\').lower().endswith('\\'+name.lower())]
            draw.text((15, y+66), f'active rows: {len(users)}', font=small, fill=(200,200,200))
            by_face = {x['metadata']['file_name'].split('_')[1].split('.')[0].upper(): x for x in per_set[name]}
            assert set(by_face) == set(FACES), (name, set(by_face))
            for col, face in enumerate(FACES):
                entry = by_face[face]
                tile = entry['thumbnail']
                x = 255 + col*415
                sheet.paste(tile, (x, y+24))
                meta = entry['metadata']
                draw.text((x, y+3), meta['file_name'], font=small, fill='white')
                draw.text((x, y+405), f"{meta['width']}x{meta['height']} A{meta['alpha_range']}", font=small, fill=(200,200,200))
        destination = OUT / (title+'.jpg')
        sheet.save(destination, quality=92, subsampling=0)
        manifest['sheets'].append({'path': str(destination), 'size': list(sheet.size), 'sets': names,
                                   'sha256': digest(destination.read_bytes())})

    # Avoid retaining in-memory thumbnails in JSON.
    manifest['sets'] = {name: {'files': [x['metadata']['file_name'] for x in entries],
                              'active_hour_slots': [h for h,d in manifest['active_hour_to_sky_dir'].items()
                                                    if d.lower().endswith('/'+name.lower())]}
                        for name, entries in per_set.items()}
    manifest['total_files'] = len(manifest['textures'])
    manifest['total_bytes'] = sum(x['bytes'] for x in manifest['textures'])
    manifest.update(source_usage())
    manifest.update(duplicate_groups(manifest['textures']))
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps({'files': manifest['total_files'], 'bytes': manifest['total_bytes'],
                      'hour_map': manifest['active_hour_to_sky_dir'], 'sheets': manifest['sheets']}, indent=2))


if __name__ == '__main__':
    main()
