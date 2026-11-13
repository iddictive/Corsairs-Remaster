import sys
from pathlib import Path

root = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(root / 'tools'))
from patch_metal_portobello_ground import parse_gm, _read_vertex

gm = parse_gm(Path(sys.argv[1]).read_bytes())
with open(sys.argv[2], 'w') as out:
    for obj_id, obj in enumerate(gm['objects']):
        buffer_id, count, start, _, base, _ = obj[7:13]
        buffer = gm['buffers'][buffer_id]
        for triangle in gm['triangles'][start:start + count]:
            vertices = [_read_vertex(gm['data'], buffer['payload_offset'], buffer['stride'], base + i) for i in triangle]
            out.write(str(obj_id) + ' ' + ' '.join(str(x) for v in vertices for x in (*v['position'], *v['normal'], *v['uv'])) + '\n')
