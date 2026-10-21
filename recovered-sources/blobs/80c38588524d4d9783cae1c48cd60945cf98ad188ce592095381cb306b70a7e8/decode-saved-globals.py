"""Read-only independent SAVE-format evidence decoder; no engine state mutation."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

source = Path(sys.argv[1])
raw = source.read_bytes()
size, packed = struct.unpack_from('<II', raw, 40)
data = zlib.decompress(raw[48:48 + packed])
assert len(data) == size
offset = 0


def vdword():
    global offset
    value = data[offset]
    offset += 1
    if value == 254:
        value = struct.unpack_from('<H', data, offset)[0]
        offset += 2
    elif value == 255:
        value = struct.unpack_from('<I', data, offset)[0]
        offset += 4
    return value


def string():
    global offset
    length = vdword()
    value = data[offset:offset + length]
    offset += length
    return value.rstrip(b'\0').decode('utf-8', errors='replace') if length else None


def attributes():
    children = vdword()
    vdword()
    string()
    for _ in range(children):
        attributes()


def value(kind):
    global offset
    if kind in (6, 7):
        offset += 4
    elif kind == 12:
        offset += 8
    elif kind == 8:
        string()
    elif kind == 9:
        offset += 8
        attributes()
    elif kind in (10, 11):
        position = offset
        target = vdword()
        result = {'target': target, 'payload_offset': position}
        if target != 0xffffffff:
            result['element'] = vdword()
            if kind == 11:
                result['attribute_path'] = string()
        return result
    else:
        raise ValueError((kind, offset))


program = string()
for _ in range(vdword()):
    string()
segments = [string() for _ in range(vdword())]
variables = []
for ordinal in range(vdword()):
    name = string()
    kind, elements = struct.unpack_from('<II', data, offset)
    offset += 8
    values = [value(kind) for _ in range(elements)]
    item = {'ordinal': ordinal, 'name': name, 'type': kind, 'elements': elements}
    if kind in (10, 11):
        item['references'] = values
    variables.append(item)
assert offset == len(data), (offset, len(data))
for variable in variables:
    for reference in variable.get('references', []):
        target = reference['target']
        reference['target_name'] = variables[target]['name'] if target < len(variables) else None
print(json.dumps({'save_sha256': hashlib.sha256(raw).hexdigest(), 'program': program,
                  'segments': segments, 'bytes': len(data), 'variables': variables}, indent=2))
