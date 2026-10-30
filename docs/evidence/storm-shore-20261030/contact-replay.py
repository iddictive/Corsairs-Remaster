"""Frozen numeric diagnosis; run from repository root. No game, writes or GPU.

The original run retained aggregate stdout only, not the241 per-sample rows.
This exact procedure reconstructs those rows in memory from authored TGA inputs.
It compares the scalar foam query at a displaced water vertex with that water
vertex's represented height, not with a rendered triangle or the user's scene.
"""
from pathlib import Path
import math,json
frames=[(Path('experiments/native-metal/.cache/runtime/RESOURCE/Sea')/f'sea{i:04}.tga').read_bytes()[18::4] for i in range(64)]
def normal(f,x,z):
 dx=(f[z*128+((x-1)&127)]-f[z*128+((x+1)&127)])/510;dz=(f[((z-1)&127)*128+x]-f[((z+1)&127)*128+x])/510;l=math.sqrt(dx*dx+1+dz*dz)
 return int(dx/l*32767.5)/32767.5,int(dz/l*32767.5)/32767.5
def sample(x,z,t):
 h=nx=ny=nz=0
 for scale,amp,speed,move in ((.075,64,4.25,8),(.5,9,8,12)):
  u,v=(x+move*t)*scale,z*scale;ix,iz=math.floor(u),math.floor(v);fx,fz=u-ix,v-iz;phase=(speed*t+(10 if scale==.5 else 0))%64;j=int(phase);dt=phase-j
  ah=ax=az=0
  for xx,zz,w in ((0,0,(1-fx)*(1-fz)),(1,0,fx*(1-fz)),(0,1,(1-fx)*fz),(1,1,fx*fz)):
   xx=(ix+xx)&127;zz=(iz+zz)&127
   f,g=frames[j],frames[(j+1)%64];ah+=w*((1-dt)*f[zz*128+xx]+dt*g[zz*128+xx])/255
   a,b=normal(f,xx,zz);c,d=normal(g,xx,zz);ax+=w*(a+(c-a)*dt);az+=w*(b+(d-b)*dt)
  h+=amp*ah;nx+=amp*scale*ax;nz+=amp*scale*az;ny+=math.sqrt(max(0,1-ax*ax-az*az))
 l=math.sqrt(nx*nx+ny*ny+nz*nz);return h,nx/l,nz/l
rows=[]
for index in range(241):
 t=index/30
 # Follow a real SEA vertex: horizontal motion and its exact represented height.
 h,nx,nz=sample(84,100,t);x,z=84-nx*12.75,100-nz*12.75
 old=sample(x,z,t)[0]+.2;new=h+.2
 rows.append((t,old-h,new-h))
old_under=[r for r in rows if r[1]<0];old_above=[r for r in rows if r[1]>0]
crossings=sum((a[1]<0)!=(b[1]<0) for a,b in zip(rows,rows[1:]))
print(json.dumps(dict(profile='authored storm64/9, scales.075/.5, PosShift4.25',samples=len(rows),seconds=8,old_min_clearance=min(r[1] for r in rows),old_max_clearance=max(r[1] for r in rows),old_surface_crossings=crossings,old_submerged_samples=len(old_under),new_clearance_min=min(r[2] for r in rows),new_clearance_max=max(r[2] for r in rows),first_submerged=old_under[0] if old_under else None)))
