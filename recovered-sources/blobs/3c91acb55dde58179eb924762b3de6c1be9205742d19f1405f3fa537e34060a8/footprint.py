import math,json,pathlib
radius=.13
polygons=[]
for side in range(4):
    for n in range(6):
        x,y=-.5+n*.2,-.5
        if side==1:y,x=x,.5
        if side==2:y=.5
        if side==3:y,x=x,-.5
        r=radius/math.cos(math.pi/8)
        polygons.append([(x+r*math.cos(math.pi/8+k*math.pi/4),y+r*math.sin(math.pi/8+k*math.pi/4)) for k in range(8)])
segments=[(p[n],p[(n+1)%8]) for p in polygons for n in range(8)]
events={x for p in polygons for x,y in p}|{-.5,.5}
for a,b in segments:
    for c,d in segments:
        u=(b[0]-a[0],b[1]-a[1]);v=(d[0]-c[0],d[1]-c[1]);w=(c[0]-a[0],c[1]-a[1]);cross=u[0]*v[1]-u[1]*v[0]
        if abs(cross)<1e-12:continue
        s=(w[0]*v[1]-w[1]*v[0])/cross;t=(w[0]*u[1]-w[1]*u[0])/cross
        if 0<s<1 and 0<t<1:events.add(a[0]+s*u[0])
def intervals(x):
    parts=[]
    for p in polygons:
        ys=[]
        for a,b in zip(p,p[1:]+p[:1]):
            if min(a[0],b[0])<x<max(a[0],b[0]):ys.append(a[1]+(b[1]-a[1])*(x-a[0])/(b[0]-a[0]))
        if len(ys)>=2:parts.append((min(ys),max(ys)))
    merged=[]
    for lo,hi in sorted(parts):
        if merged and lo<=merged[-1][1]+1e-12:merged[-1]=(merged[-1][0],max(hi,merged[-1][1]))
        else:merged.append((lo,hi))
    return merged
area=hole=0
events=sorted(events)
for lo,hi in zip(events,events[1:]):
    parts=intervals((lo+hi)/2)
    area+=(hi-lo)*sum(b-a for a,b in parts)
    if -.5<(lo+hi)/2<.5:
        hole+=(hi-lo)*(1-sum(max(0,min(b,.5)-max(a,-.5)) for a,b in parts))
result=dict(cutter_union_area=area,retained_solid_volume=8-2*area,enclosed_solid_volume=2*hole,anchored_solid_volume=8-2*(area+hole),retained_hollow_volume=3.904-.4*area,enclosed_hollow_total_volume=.4*hole)
pathlib.Path(__file__).with_name('footprint-oracle.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result))
