#!/usr/bin/env python3
"""Read-only frozen-wave LOD discontinuity falsifier, using original TGA heights.
Tree equations mirror SEA::CalculateLOD/BuildTree; frustum culling is omitted to
keep the same fixed visible test patch. This proves a source discontinuity, not
that every observed runtime pop has this cause. No physics or runtime writes.
"""
import math
from pathlib import Path
ROOT=Path(__file__).parent/'.cache/runtime/RESOURCE/Sea'
frames=[(ROOT/f'sea{i:04d}.tga').read_bytes()[18::4] for i in (0,10)]
def tree(cx):
    out={};center=17.92*(int(cx/17.92)-256);centerz=-17.92*256
    def build(tx,tz,level):
        size=131072>>level;s=size*.07;x=center+tx*s;z=centerz+tz*s
        far=max((xx-cx)**2+zz**2 for xx in (x,x+s) for zz in (z,z+s))
        near=max(x-cx,0,cx-x-s)**2+max(z,0,-z-s)**2
        def lod(d):return max(4,int(.5*math.log2(.5*d))) if d else 4
        lo,hi=lod(near),lod(far)
        if size<=128 or hi-lo<=1:out[(round(x,6),round(z,6),size)]=lo;return
        for dz in range(2):
            for dx in range(2):build(tx*2+dx,tz*2+dz,level+1)
    build(0,0,0);return out

def height(x,z):
    value=0
    for data,scale in zip(frames,(.4,2.)):
        x1,z1=x*scale,z*scale;ix,iz=math.floor(x1),math.floor(z1);fx,fz=x1-ix,z1-iz
        a,b,c,d=[data[((iz+dz)&127)*128+((ix+dx)&127)]/255 for dx,dz in ((0,0),(1,0),(0,1),(1,1))]
        value+=a+fx*(b-a)+fz*(c-a)+fx*fz*(d+a-b-c)
    return value

def rendered(x,z,originx,originz,lod):
    step=.07*(1<<lod);gx=math.floor((x-originx)/step);gz=math.floor((z-originz)/step);a=originx+gx*step;b=originz+gz*step;fx=(x-a)/step;fz=(z-b)/step
    h00,h10,h01,h11=[height(a+dx*step,b+dz*step) for dx,dz in ((0,0),(1,0),(0,1),(1,1))]
    return h00+(fx*(h10-h00)+fz*(h11-h10) if fx>=fz else fz*(h01-h00)+fx*(h11-h01))
previous=tree(.001);found=False
for i in range(2,17920):
    cam=i*.001;current=tree(cam)
    for key,old in previous.items():
        new=current.get(key)
        if new is None or new==old:continue
        x,z,size=key
        if max(abs(x),abs(z))>400:continue
        best=(0,None)
        for u in (.19,.37,.53,.71):
            for v in (.23,.41,.67,.83):
                px,pz=x+u*size*.07,z+v*size*.07
                before=rendered(px,pz,x,z,old);after=rendered(px,pz,x,z,new)
                if abs(after-before)>best[0]:best=(abs(after-before),(px,pz,before,after))
        if best[0]>.001:
            print(f'Frozen frame: camera {cam-.001:.3f}->{cam:.3f}; patch={key}; LOD {old}->{new}; height jump={best[0]:.6f}; sample={best[1]}')
            assert tree(cam)==current,'static camera must preserve topology'
            print('PASS source discontinuity reproduced; static camera topology stable. Runtime cause remains to correlate.')
            found=True;break
    if found:break
    previous=current
assert found,'No LOD transition reproduced in bounded camera sweep'

# Independently exercise the patched collapse rule, including the actual frozen
# height input. Both render positions feed the unchanged SSE wave sampler.
def morph(x,z,cx,cz,level):
    for level in range(level,20):
        next_level=2**(level+1);end=next_level/math.sqrt(.5)
        weight=(math.hypot(x-cx,z-cz)-end*.75)/(end*.25)
        if weight<=0:break
        weight=min(weight,1);step=.07*next_level;fine=step*.5
        # Inputs are exact lattice vertices here; unlike Python bankers rounding,
        # C++ round's halfway rule cannot be reached by these integer coordinates.
        tx=math.floor(round(x/fine)*.5)*step;tz=math.floor(round(z/fine)*.5)*step
        x+=(tx-x)*weight;z+=(tz-z)*weight
        if weight<1:break
    return x,z

def optimized_morph(x,z,cx,cz,level):
    for level in range(level,20):
        next_level=2**(level+1);end=next_level/math.sqrt(.5);start=end*.75
        distance_squared=(x-cx)**2+(z-cz)**2
        if distance_squared<=start*start:break
        weight=1 if distance_squared>=end*end else (math.sqrt(distance_squared)-start)/(end-start)
        step=.07*next_level;fine=step*.5
        tx=math.floor(round(x/fine)*.5)*step;tz=math.floor(round(z/fine)*.5)*step
        x+=(tx-x)*weight;z+=(tz-z)*weight
        if weight<1:break
    return x,z

for level in (4,5,6,8,10):
    for x,z,cx,cz in ((17.5,-9.25,0,0),(123.75,66.5,-20,12),(2048,-1024,13,-7)):
        reference=morph(x,z,cx,cz,level)
        optimized=optimized_morph(x,z,cx,cz,level)
        assert math.dist(reference,optimized)<1.e-6,(level,reference,optimized)
for level in (4,5,6,8,10):
    step=.07*2**level;x,z=11*step,3*step
    threshold=2**(level+1)/math.sqrt(.5);cx=x-math.sqrt(threshold**2-z*z)
    tx,tz=10*step,2*step
    epsilon=1.e-6
    before=morph(x,z,cx+epsilon,0,level)
    after=morph(tx,tz,cx-epsilon,0,level+1)
    gap=math.dist(before,after);height_gap=abs(height(*before)-height(*after))
    assert gap<1.e-4 and height_gap<1.e-4,(level,gap,height_gap)
    assert morph(x,z,cx,0,level)==morph(x,z,cx,0,level),'static camera repeat'
    # Shared even vertices must follow the identical trajectory at either LOD,
    # including a later morph into the next coarser grid.
    for shift in (0,threshold*.5,threshold):
        assert math.dist(morph(tx,tz,cx-shift,0,level),morph(tx,tz,cx-shift,0,level+1))<1.e-5
    print('morph level',level,'position gap',gap,'actual height gap',height_gap)
# New quarter-root origin is an integer multiple of every visible level grid;
# crossing the old 17.92-m boundary no longer changes any world-space lattice.
chunk=131072*.07/4
origin=lambda camera:chunk*(math.floor(camera/chunk)-2)
assert origin(17.919)==origin(17.921)
for level in range(4,15):
    step=.07*2**level
    assert abs((origin(chunk+.001)-origin(chunk-.001))/step-round(chunk/step))<1.e-8
assert chunk>1600,'root recenter keeps wave-cutoff region inside geometry coverage'
print('PASS morph threshold continuity, shared-edge ancestors, static camera, old/new grid boundaries')
