"""Read-only order-of-magnitude hole-count calculation; capacity is unresolved.

The 1000m3 table is explicitly a scalable example, not measured Manowar capacity.
No game process, GPU, configuration or SAVE mutation is performed.
"""
import hashlib
import json
import math
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
APP = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources')
SAVE = Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs/SAVE/Кампот/Кампот AutoSave 5')
sys.path.insert(0, str(ROOT / 'tools'))
import compact_native_save as codec

codec.MATERIAL = {b'ship'}
save_bytes = SAVE.read_bytes()
parsed = codec.Save(save_bytes)


def value(node, key):
    raw = node.scalar(key) if node else None
    return raw.decode('utf8', 'replace') if raw is not None else None


matches = []
for node in parsed.roots:
    material = node.child(b'hullmaterial')
    if value(material, b'model') == 'Manowar1':
        meta = codec.selected(material, (b'metaa', b'metab'), codec.revision(material))
        matches.append({
            'ship_type': value(node, b'type'), 'hp': value(node, b'hp'),
            'crew': value(node.child(b'crew'), b'quantity'),
            'immersion': value(node, b'immersion'),
            'waterplane': value(meta, b'waterplane'),
            'flood_branch_present': node.child(b'phonkflood') is not None,
        })
assert len(matches) == 1
saved = matches[0]
ships_path = APP / 'PROGRAM/ships/ships_init.c'
ships = ships_path.read_bytes().decode('latin1')
ship = re.split(r'makeref\(refShip,ShipsTypes\[SHIP_MANOWAR\]\);', ships)[1].split('makeref(refShip,ShipsTypes[', 1)[0]


def number(field):
    return float(re.search(r'refship\.' + field + r'\s*=\s*([.\d-]+)', ship, re.I).group(1))


optimal = number('MaxCrew')
assert 'refShip.OptCrew = sti(refShip.MaxCrew);' in ships
gm_path = APP / 'RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm'
gm = gm_path.read_bytes()
header = struct.unpack_from('<11i7f', gm)
bottom = header[15] - header[12] / 2
draft = max(1.5, float(saved['immersion']) + number('WaterLine') - bottom)
flow_per_area = .62 * math.sqrt(2 * 9.81 * 1)
example_capacity = 1000.
table = []
for label, crew, repair in (
    ('minimum crew; same Repair50', number('MinCrew'), .5),
    ('full crew; same Repair50', optimal, .5),
    ('minimum crew; poor Repair10', number('MinCrew'), .1),
    ('full crew; excellent Repair100', optimal, 1.),
):
    pump = example_capacity * .005 * min(1, max(0, crew / optimal)) * min(1, max(0, repair))
    holes = {}
    for diameter in (.1, .2):
        area = math.pi * diameter ** 2 / 4
        count = pump / (flow_per_area * area)
        holes[str(diameter)] = {'continuous_balance': count, 'whole_holes_below_balance': math.floor(count)}
    table.append({'case': label, 'crew': crew, 'repair': repair, 'pump_m3_per_game_second': pump, 'circular_hole_diameter_metres': holes})
result = {
    'classification': 'Analytical example; actual Manowar flood capacity unresolved',
    'input_sha256': {str(SAVE): hashlib.sha256(save_bytes).hexdigest(), str(ships_path): hashlib.sha256(ships_path.read_bytes()).hexdigest(), str(gm_path): hashlib.sha256(gm).hexdigest()},
    'saved_Manowar1': saved,
    'installed_default_Manowar': {'optimal_crew': optimal, 'minimum_crew': number('MinCrew'), 'base_hp': number('HP'), 'waterline': number('WaterLine')},
    'gm_hull_bottom_y': bottom,
    'draft_for_saved_immersion': draft,
    'capacity_equation': 'max(100, originalWaterplaneArea * draft * 0.35)',
    'capacity_per_square_metre_waterplane': draft * .35,
    'unresolved': 'Saved waterplane is0 and PhonkFlood absent. Exact original area requires native BuildContour collision tracing; no native replay authorized.',
    'assumptions': {'example_capacity_m3': example_capacity, 'opening_head_metres': 1, 'round_hole_diameters_metres': [.1, .2], 'fixed_head': True, 'actual_cannon_caliber_to_hole_diameter_claim': False},
    'example_table': table,
    'scaling': 'Multiply counts by actual_capacity/1000; depth h changes counts by1/sqrt(h).',
    'count_equation': 'N = capacity * .005 * clamp(crew/optimalCrew) * clamp(Repair) / (.62 * pi * (diameter/2)^2 * sqrt(2*9.81*depth))',
    'limits': ['Balance of water flow only, not shots-to-kill.', 'Each actual geometric opening contributes its area; overlaps and above-water holes do not equal shot count.', 'Hull HP loss, structural material/waterplane/keel failure, new impacts, changing draft/waves and crew loss can sink the ship independently.', '830 is installed default optimal crew; generated RealShips may carry modified values.'],
}
(OUT / 'manowar-calculation.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({k: v for k, v in result.items() if k != 'input_sha256'}, indent=2))
