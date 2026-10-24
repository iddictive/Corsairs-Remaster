#!/usr/bin/env python3
"""Task-local mathematical dart calibration; never starts or edits the game.

Reads shipped Manowar triangles/51 cannon locators. Projects the actual root
hull triangles along the mean terminal trajectory into a 2cm silhouette.
Analytical flight and independent uniform launch perturbations match AIBalls.
This is NOT native collision/runtime acceptance: small per-shot tangent changes,
waves, ship roll/sway, rigging, hull damage, AI bort delta and shot timing omitted.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import sys

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'experiments/native-metal'))
from island_geometry import load_static_gm

G = 9.81
HM = .40  # cannon1 * absolute per-good dart .40
BASE_CAP = .60
CAP = .5 * math.asin(math.sin(2 * BASE_CAP) / 2)
PIXEL = .02
MODEL = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm')


def muzzles(data):
    h = struct.unpack_from('<11i7f', data)
    strings = data[72:72+h[2]]
    off = 72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]
    result = []
    for i in range(h[7]):
        row = struct.unpack_from('<3i16f4i4f', data, off+108*i)
        group = strings[row[0]:].split(b'\0')[0]
        if group == b'cannonr':
            result.append(row[15:18])
    return sorted(result, key=lambda p: p[2])


def solve(v, distance, dy):
    b = G*dy-v*v
    disc = b*b-G*G*(distance*distance+dy*dy)
    t = math.sqrt((-b-math.sqrt(disc))/(G*G/2))
    return math.atan2(dy+.5*G*t*t, distance)


def curve(muzzle, target, speed, angle, yaw_delta):
    dx, dy, dz = [target[i]-muzzle[i] for i in range(3)]
    raw = math.atan2(dy, math.hypot(dx, dz))
    s, c = math.sin(raw), math.cos(raw)
    a, b, cc = c*c+HM*s*s, (1-HM)*s*c, s*s+HM*c*c
    yaw = math.atan2(dx, dz)+yaw_delta
    vx = speed*(a*math.cos(angle)+b*math.sin(angle))
    vy = speed*(b*math.cos(angle)+cc*math.sin(angle))
    ax, ay = -G*b, -G*cc
    return (vx*math.sin(yaw), vy, vx*math.cos(yaw)), (ax*math.sin(yaw), ay, ax*math.cos(yaw))


def crossing(muzzle, velocity, acceleration, plane):
    dx = plane-muzzle[0]
    # Stable small root of .5*a*t*t+v*t-dx=0.
    t = 2*dx/(velocity[0]+math.sqrt(velocity[0]**2+2*acceleration[0]*dx))
    pos = [muzzle[i]+velocity[i]*t+.5*acceleration[i]*t*t for i in range(3)]
    return t, pos


def silhouette(scene, slope):
    ymin, ymax, zmin, zmax = -8., 18., -40., 40.
    im = Image.new('1', (round((zmax-zmin)/PIXEL)+1, round((ymax-ymin)/PIXEL)+1))
    draw = ImageDraw.Draw(im)
    for tri in scene['indices']:
        points = []
        for index in tri:
            x,y,z = scene['vertices'][index]['p']
            points.append(((z-zmin)/PIXEL, (y-slope*x-ymin)/PIXEL))
        draw.polygon(points, fill=1)
    return im, (ymin, zmin)


def experiment(scene, guns, fire_range, shots, seed, bort_delta):
    speed = 2*1.15*math.sqrt(fire_range*G/math.sin(2*BASE_CAP))
    advertised = 2*1.15**2*fire_range
    # Full broadside must remain reachable even at maximum script target Y.
    # Native CalcMaxFireDistance applied to each actual muzzle/target height.
    upper_y = 8.6 + .5*4.3*.3
    reachable = []
    for rank, muzzle in enumerate(guns):
        z = -35.790977478027344+rank/(len(guns)-1)*(33.69902420043945+35.790977478027344)
        radial = speed*math.cos(CAP)*(speed*math.sin(CAP)+math.sqrt((speed*math.sin(CAP))**2+2*G*(muzzle[1]-upper_y)))/G
        reachable.append(muzzle[0]+math.sqrt(radial*radial-(z-muzzle[2])**2))
    distance = min(reachable)-.01
    central = tuple(sum(p[i] for p in guns)/len(guns) for i in range(3))
    aim = (distance, 8.6, -.5*(35.790977478027344-33.69902420043945))
    angle = solve(speed, math.hypot(distance-central[0], aim[2]-central[2]), aim[1]-central[1])
    vel, acc = curve(central, aim, speed, angle, 0.)
    nominal_t, _ = crossing(central, vel, acc, distance)
    slope = (vel[1]+acc[1]*nominal_t)/(vel[0]+acc[0]*nominal_t)
    im, (ymin,zmin) = silhouette(scene,slope)
    image_pixels = im.load()
    rng = random.Random(seed)
    samples = []
    for n in range(shots):
        rank = n % len(guns)
        if rank == 0:
            # One common offset/target Y per ordered broadside.
            y = 8.6+(rng.random()-.5)*4.3*.3
            phi = rng.random()*2*math.pi
            offset_x, offset_z = bort_delta*math.sin(phi), bort_delta*math.cos(phi)
        muzzle = guns[rank]
        # Max Cannoners + skill + RakingFire -> full root-hull axis span.
        z = -35.790977478027344+rank/(len(guns)-1)*(33.69902420043945+35.790977478027344)
        target = (distance+offset_x,y,z+offset_z)
        base = solve(speed,math.hypot(target[0]-muzzle[0],target[2]-muzzle[2]),y-muzzle[1])
        samples.append((muzzle,target,base,rng.random()-.5,rng.random()-.5,rng.random()-.5))
    result = []
    for canonical_multiplier in (0.,.12,.14,.15,.16,.18,.20,.25,.28,.30,.32,.35,.40,.50,1.):
        multiplier = canonical_multiplier*2116/advertised
        hit = below = 0
        for muzzle,target,base,u,v,w in samples:
            velocity,acceleration = curve(muzzle,target,speed+v*.05*10*math.radians(12)*multiplier,
                                          base+w*.05*math.radians(15)*multiplier,
                                          u*.5*math.radians(12)*multiplier)
            t,p = crossing(muzzle,velocity,acceleration,distance)
            iz = round((p[2]-zmin)/PIXEL)
            iy = round((p[1]-ymin)/PIXEL)
            # A water contact before the target is a miss on this flat-sea oracle.
            if p[1] < 0:
                below += 1
                continue
            if 0 <= iz < im.width and 0 <= iy < im.height and image_pixels[iz,iy]:
                hit += 1
        result.append({'canonical_dispersion':canonical_multiplier,'effective_dispersion':multiplier,'hit_fraction':hit/shots,'hit_count':hit,
                       'water_before_target_fraction':below/shots})
    apex_t = -vel[1]/acc[1]
    nominal_reachable = sum(solve(speed,math.hypot(distance-g[0],(-35.790977478027344+i/(len(guns)-1)*(33.69902420043945+35.790977478027344))-g[2]),8.6-g[1]) <= CAP for i,g in enumerate(guns))
    return {'cannon_fire_range':fire_range,'advertised_equal_height_range_m':advertised,
            'common_ai_bort_delta_m':bort_delta,
            'distance_all_guns_reachable_at_target_band_ceiling_m':distance,'nominal_reachable_guns':nominal_reachable,
            'maximum_target_band_y':upper_y,'speed_m_per_ball_second':speed,
            'nominal_mean_muzzle':central,'nominal_elevation_rad':angle,'nominal_elevation_deg':math.degrees(angle),
            'cap_rad':CAP,'cap_deg':math.degrees(CAP),'effective_height_multiply':HM,
            'nominal_flight_ball_seconds':nominal_t,'nominal_flight_game_seconds':nominal_t/2,
            'nominal_apex_world_y':central[1]+vel[1]*apex_t+.5*acc[1]*apex_t**2,
            'terminal_dy_dx':slope,'samples':shots,'results':result}


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--shots',type=int,default=51000)
    args=ap.parse_args()
    data=MODEL.read_bytes()
    scene=load_static_gm(MODEL)
    guns=muzzles(data)
    assert len(guns)==51
    output={'oracle':'analytical warped trajectories + projected real GM triangle silhouette; NOT engine hits',
            'model_sha256':hashlib.sha256(data).hexdigest(),'triangle_count':len(scene['indices']),
            'guns':guns,'seed':20261021,'pixel_m':PIXEL,
            'accuracy':.05,'azimuth_unscaled_halfwidth_deg':3.,
            'elevation_unscaled_halfwidth_deg':.375,
            'cases':[experiment(scene,guns,r,args.shots,20261021,delta) for delta in (0.,4.4) for r in (800,1600)]}
    print(json.dumps(output,indent=2))


if __name__=='__main__':
    main()
