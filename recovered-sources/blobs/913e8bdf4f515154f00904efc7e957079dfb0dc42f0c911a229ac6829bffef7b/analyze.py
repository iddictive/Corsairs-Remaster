"""CPU-only inventory of an existing capture; no replay or game access."""
import collections
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
BASE = ROOT / 'experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520'
inputs = {}

def inventory(relative):
    path = BASE / relative
    raw = path.read_bytes()
    inputs[str(path.relative_to(ROOT))] = hashlib.sha256(raw).hexdigest()
    text, decoder, pos, candidates = raw.decode(), json.JSONDecoder(), 0, []
    while True:
        pos = text.find('{', pos)
        if pos < 0:
            break
        try:
            value, length = decoder.raw_decode(text[pos:])
        except ValueError:
            pos += 1
            continue
        pos += length
        if 'children' in value:
            candidates.append(value)
    value = max(candidates, key=lambda row: len(row['children']))
    assert len(value['children']) == value['totalCount']
    return value['children']

apis = inventory('reflection-aggregation-fit/api-inventory.jsonl')
encoders = inventory('reflection-aggregation-fit/encoder-inventory.jsonl')
buffers = inventory('captured-bounds-fit/buffer-inventory.jsonl')
calls, copies, lengths = collections.Counter(), [], collections.Counter()
for row in apis:
    statement = ' '.join(str(v['value']) for v in row.get('values', []) if v)
    calls[statement.split(':', 1)[0]] += 1
    if 'copyFromTexture:' in statement:
        copies.append(statement)
    if any(word in statement for word in ('setVertexBytes:', 'setFragmentBytes:')):
        match = re.search(r'length:(\d+)', statement)
        if match:
            lengths[int(match[1])] += 1

byte_sum = 0
for row in buffers:
    match = re.fullmatch(r'([\d.]+) (bytes|KiB|MiB|GiB)', row['values'][1]['value'])
    assert match
    byte_sum += float(match[1]) * {'bytes': 1, 'KiB': 1024, 'MiB': 1024**2, 'GiB': 1024**3}[match[2]]

labels = collections.Counter()
for row in encoders:
    label = re.sub(r' \(\d+\)', '', row['values'][0]['value'].strip('"'))
    if label.startswith('MTL'):
        label = re.sub(r' \d+$', '', label)
    labels[label] += 1

result = {
    'scope': 'Retained 012905 captured frame inventories, CPU-only; not current timing or resident-cache occupancy',
    'api_count': len(apis), 'encoder_count': len(encoders), 'buffer_count': len(buffers),
    'encoder_labels': dict(labels), 'native_calls': dict(calls.most_common()),
    'texture_copies': copies,
    'inline_uniform_length_histogram': dict(lengths),
    'inline_uniform_explicit_bytes': sum(size * count for size, count in lengths.items()),
    'buffer_inventory_approx_bytes': byte_sum,
    'buffer_inventory_approx_MiB': byte_sum / 1024**2,
    'raw_cache_limits_from_current_source': {'bytes': 512 * 1024**2, 'entries': 8192},
    'negative_findings': [
        'One command-buffer commit and no explicit wait in captured commands. Encoder inventory does not measure CPU resource creation/update calls.',
        '68 render encoders include 48 point-cube faces; not one encoder per draw.',
        'Four texture copies, not draw-count multiplied scene copying.',
        'Captured entire 1494-buffer inventory is approximately148MiB; below512MiB raw-cache byte budget. Does not measure stale cache occupancy.',
        'Nine local-weather segment/integration pairs follow view changes; approximately8.895% reported shader cost, insufficient to claim2x.',
    ],
    'inputs_sha256': inputs,
}
print(json.dumps(result, indent=2))
