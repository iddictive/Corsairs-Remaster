"""CPU reference for the shader's surface/air UV ray interval, not GPU acceptance."""
from pathlib import Path
import math,random
s=(Path(__file__).resolve().parents[3]/'experiments/native-metal/cluster_impact_render.hpp').read_text()
assert 'float3 uv=vo+vr*(near+(float(i)+.5)*step);' in s
assert 'i<16 && sampleVolume' in s

def uv(p,angle,stretch,height,air):
 c,si=math.cos(angle),math.sin(angle)
 return ((c*p[0]-si*p[2])/(1.4*stretch)+.5,(si*p[0]+c*p[2])/(1.4/stretch)+.39,p[1]/height+.10) if air else ((c*p[0]-si*p[2])/(2*stretch)+.5,p[1]/(2.8*height),(si*p[0]+c*p[2])/(2/stretch)+.5)
def interval(o,r,angle,stretch,height,air,lo,hi):
 v=uv(o,angle,stretch,height,air);z=uv((0,0,0),angle,stretch,height,air);raw=uv(r,angle,stretch,height,air);d=tuple(a-b for a,b in zip(raw,z))
 for a,b in zip(v,d):
  safe=b if abs(b)>=1e-6 else (-1e-6 if b<0 else 1e-6)
  aa,bb=-a/safe,(1-a)/safe;lo=max(lo,min(aa,bb));hi=min(hi,max(aa,bb))
 return lo,hi,v,d
# Same authored surface cube; spray/shock envelope previously wasted samples.
for bound,expected in ((2,8),(4,4)):
 o=(0,1,-5);r=(0,0,1);lo,hi=5-bound,5+bound
 valid=lambda t:all(0<=x<=1 for x in uv(tuple(a+b*t for a,b in zip(o,r)),0,1,1,False))
 old=sum(valid(lo+(i+.5)*(hi-lo)/16) for i in range(16));assert old==expected
 a,b,_,_=interval(o,r,0,1,1,False,lo,hi)
 assert sum(valid(a+(i+.5)*(b-a)/16) for i in range(16))==16
random.seed(73);checked=0
for air in (False,True):
 for _ in range(1000):
  o=tuple(random.uniform(-3,3) for _ in range(3));r=tuple(random.uniform(-1,1) for _ in range(3));angle=random.random()*6.28;st=random.uniform(.8,1.2);h=random.uniform(.85,1.15)
  a,b,v,d=interval(o,r,angle,st,h,air,0,20)
  if b<=a:continue
  for i in range(16):
   t=a+(i+.5)*(b-a)/16;p=tuple(x+y*t for x,y in zip(o,r));authored=uv(p,angle,st,h,air);linear=tuple(x+y*t for x,y in zip(v,d))
   assert max(abs(x-y) for x,y in zip(authored,linear))<1e-10
   assert all(-1e-6<=x<=1+1e-6 for x in authored)
  checked+=1
assert checked>100
print(f'PASS: surface valid samples8/4->16 at same16-step ceiling; {checked} randomized air/surface clipped intervals and UV equivalence')
