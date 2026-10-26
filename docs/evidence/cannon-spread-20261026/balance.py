"""Replay the hull-box approximation used to select launch-spread parameters.

Run with Python 3 and an installed app (optional first argument). This is an
analytic balance measurement, not a mesh-collision or gameplay regression test.
"""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

app = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('/Applications/Corsairs Iddictive Remaster.app')
ships = []
inputs = []
for name in ('Sloop1', 'Brig1', 'Frigate1', 'Manowar1'):
    path = next(p for p in (app / 'Contents/Resources/RESOURCE/MODELS/Ships' / name).iterdir()
                if p.suffix.lower() == '.gm' and p.stem.lower() == name.lower())
    data = path.read_bytes()
    h = struct.unpack_from('<11i7f', data)
    beam, height, length = h[11:14]
    top = h[15] + height / 2
    ships.append((name, beam, length, top))
    inputs.append(dict(ship=name, sha256=hashlib.sha256(data).hexdigest(),
                       dimensions=h[11:14], center=h[14:17], visible_top=top))


def target_plane_y(distance, target_y, delta):
    speed, gravity, height_scale, muzzle_y = 130.0, 9.81, .7, 5.0
    dy = target_y - muzzle_y
    disc = speed**4 - gravity * (gravity * distance**2 + 2 * dy * speed**2)
    angle = math.atan((speed**2 - math.sqrt(disc)) / (gravity * distance)) + delta
    raw = math.atan2(dy, distance)
    s, c = math.sin(raw), math.cos(raw)
    a, b, c2 = c*c + height_scale*s*s, (1-height_scale)*s*c, s*s + height_scale*c*c
    vx = speed * (a*math.cos(angle) + b*math.sin(angle))
    vy = speed * (b*math.cos(angle) + c2*math.sin(angle))
    ax, ay = -gravity*b, -gravity*c2
    time = 2*distance / (vx + math.sqrt(vx*vx + 2*ax*distance))
    return muzzle_y + vy*time + .5*ay*time*time


rows = []
for accuracy in (.05, .075, .1, .125, .15, .2, .256, .4, .61, .787, 1.2):
    k = .5 + .7 * (max(.2, min(1.2, accuracy)) - .2)
    yaw_half = math.radians(6*k)
    pitch_half = math.radians(7.5*accuracy)
    elevation_scale = 1.05 + 1.45*(.05/accuracy)**2
    totals = [0.0]*4
    cases = []
    for distance in (100, 300, 500, 700, 1000):
        for name, beam, length, top in ships:
            epsilon = 1e-5
            derivative = (target_plane_y(distance, top/2, epsilon)
                          - target_plane_y(distance, top/2, -epsilon)) / (2*epsilon)
            for orientation, width in enumerate((length, beam)):
                yaw = math.atan(width/(2*distance))
                baseline = min(1, yaw/yaw_half)*min(1, (top/2)/(derivative*pitch_half))
                candidate = min(1, yaw/(yaw_half*.5))*min(1, (top/2)/(derivative*pitch_half*elevation_scale))
                totals[orientation*2] += baseline
                totals[orientation*2+1] += candidate
                cases.append(dict(distance=distance, ship=name,
                                  orientation=('broadside', 'end-on')[orientation],
                                  baseline=baseline, candidate=candidate))
    rows.append(dict(accuracy=accuracy, elevation_scale=elevation_scale,
                     pooled_percent=[100*x/20 for x in totals], cases=cases))

print(json.dumps(dict(conditions=dict(speed=130, gravity=9.81, height_scale=.7,
    muzzle_y=5, aim='visible hull-box midpoint', epsilon=1e-5,
    excluded=['mesh geometry', 'speed jitter', 'ship motion', 'roll', 'battery geometry',
              'auto gunner offset', 'ammo-specific speed/height/range']),
    inputs=inputs, measurements=rows), indent=2))
