#!/usr/bin/env python3
"""Render orthographic source/candidate silhouettes using the PN helper equations.

Read-only GM intake; writes only the explicitly named plot/metrics destination.
This is geometry evidence, not an in-game acceptance screenshot.
"""
import argparse
import json
import math
from collections import defaultdict
from pathlib import Path
from island_geometry import load_static_gm

def add(a,b): return tuple(x+y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def mul(a,k): return tuple(x*k for x in a)
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def length(a): return math.sqrt(dot(a,a))
def unit(a): return mul(a,1/length(a)) if length(a)>.00001 else (0,1,0)
def refinable(texture): return Path(texture.lower()).stem in {'jungle_gray','rockk1','rockk2','rockk3'}

def candidate(scene):
    groups={};edges=defaultdict(int)
    selected={i for tri in scene['indices'] if all(refinable(scene['vertices'][j]['texture']) for j in tri) for i in tri}
    for i in selected:
        v=scene['vertices'][i]
        key=tuple(v['p']);g=groups.setdefault(key,{'normals':[],'materials':set(),'boundary':False})
        g['normals'].append(unit(v['n']));g['materials'].add(v['texture'])
    for tri in scene['indices']:
        if not all(i in selected for i in tri):continue
        points=[tuple(scene['vertices'][i]['p']) for i in tri]
        for a,b in zip(points,points[1:]+points[:1]):edges[tuple(sorted((a,b)))]+=1
    for (a,b),count in edges.items():
        if count!=2:groups[a]['boundary']=groups[b]['boundary']=True
    for g in groups.values():
        g['normal']=unit(tuple(sum(n[i] for n in g['normals']) for i in range(3)))
        g['hard']=any(dot(n,g['normal'])<.75 for n in g['normals'])
    curved={};max_move=0
    for a,b in edges:
        edge=sub(b,a);extent=length(edge);straight=mul(add(a,b),.5);ga,gb=groups[a],groups[b]
        if min(a[1],b[1])<=32 or extent<48 or ga['boundary'] or gb['boundary'] or ga['hard'] or gb['hard'] or len(ga['materials'])!=1 or ga['materials']!=gb['materials']:
            curved[(a,b)]=straight;continue
        delta=mul(sub(mul(gb['normal'],dot(edge,gb['normal'])),mul(ga['normal'],dot(edge,ga['normal']))),.125)
        distance=length(delta);limit=min(16,extent*.0625)
        if distance>limit:delta=mul(delta,limit/distance)
        point=add(straight,delta);point=(point[0],max(min(point[1],max(a[1],b[1])),min(a[1],b[1])),point[2])
        max_move=max(max_move,length(sub(point,straight)));curved[(a,b)]=point
    split={edge:length(sub(point,mul(add(*edge),.5)))>=.05 for edge,point in curved.items()}
    output=[];changed=0
    for tri in scene['indices']:
        if not all(i in selected for i in tri):
            output.append(tuple(tuple(scene['vertices'][i]['p']) for i in tri));continue
        a,b,c=[tuple(scene['vertices'][i]['p']) for i in tri]
        edge_keys=[tuple(sorted((x,y))) for x,y in ((a,b),(b,c),(c,a))]
        mask=sum(bit for bit,key in zip((1,2,4),edge_keys) if split[key])
        if not mask:output.append((a,b,c));continue
        changed+=1;ab,bc,ca=[curved[key] for key in edge_keys]
        templates={
            1:((a,ab,c),(ab,b,c)),
            2:((a,b,bc),(a,bc,c)),
            3:((a,ab,c),(ab,bc,c),(ab,b,bc)),
            4:((a,b,ca),(b,c,ca)),
            5:((a,ab,ca),(ab,b,c),(ab,c,ca)),
            6:((a,b,ca),(b,bc,ca),(bc,c,ca)),
            7:((a,ab,ca),(ab,b,bc),(ca,bc,c),(ab,bc,ca)),
        }
        output.extend(templates[mask])
    return output,{'source_triangles':len(scene['indices']),'render_triangles':len(output),'refined_faces':changed,'maximum_new_midpoint_displacement':max_move}

def silhouette(triangles,low,high,bins=2400):
    profile=[float('-inf')]*bins
    for tri in triangles:
        for a,b in zip(tri,tri[1:]+tri[:1]):
            if a[0]>b[0]:a,b=b,a
            start=max(0,math.ceil((a[0]-low)/(high-low)*(bins-1)))
            end=min(bins-1,math.floor((b[0]-low)/(high-low)*(bins-1)))
            for index in range(start,end+1):
                x=low+(high-low)*index/(bins-1);t=(x-a[0])/(b[0]-a[0]) if b[0]!=a[0] else 0
                profile[index]=max(profile[index],a[1]+t*(b[1]-a[1]))
    return profile

def main():
    parser=argparse.ArgumentParser();parser.add_argument('gm',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
    scene=load_static_gm(args.gm);after,stats=candidate(scene)
    before=[tuple(tuple(scene['vertices'][i]['p']) for i in tri) for tri in scene['indices']]
    low=min(p[0] for t in before for p in t);high=max(p[0] for t in before for p in t)
    a=silhouette(before,low,high);b=silhouette(after,low,high)
    diffs=[abs(x-y) if math.isfinite(x) and math.isfinite(y) else 0 for x,y in zip(a,b)]
    peak=max(range(len(diffs)),key=diffs.__getitem__);stats['maximum_silhouette_shift']=diffs[peak]
    center=low+(high-low)*peak/(len(a)-1);zoom=max(1000,(high-low)/12)
    zl,zh=center-zoom/2,center+zoom/2;za=silhouette(before,zl,zh,1200);zb=silhouette(after,zl,zh,1200)
    valid=[v for v in za+zb if math.isfinite(v)];bottom=min(valid)-15;top=max(valid)+15
    def line(values,left,right,ymin,ymax):
        return ' '.join(f'{80+1040*i/(len(values)-1):.2f},{500-360*(v-ymin)/(ymax-ymin):.2f}' for i,v in enumerate(values) if math.isfinite(v))
    title=f'{args.gm.stem}: actual orthographic mountain silhouette, zoom X={zl:.0f}..{zh:.0f}'
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="620" viewBox="0 0 1200 620">
<rect width="1200" height="620" fill="#101820"/><g fill="#e6edf2" font-family="sans-serif"><text x="60" y="45" font-size="22">{title}</text>
<text x="60" y="80" font-size="16">Orange: original mesh. Cyan: adaptive cubic PN edge subdivision. Geometry-only comparison.</text>
<text x="60" y="110" font-size="16">Y={bottom:.1f}..{top:.1f}; maximum whole-island silhouette change {max(diffs):.2f} world units</text></g>
<polyline points="{line(za,zl,zh,bottom,top)}" fill="none" stroke="#ffab66" stroke-width="3"/>
<polyline points="{line(zb,zl,zh,bottom,top)}" fill="none" stroke="#55dce0" stroke-width="2"/>
<text x="60" y="565" font-family="sans-serif" font-size="16" fill="#e6edf2">{stats['source_triangles']:,} → {stats['render_triangles']:,} triangles; {stats['refined_faces']:,} faces refined; original corners and shore band unchanged</text>
</svg>'''
    args.output.write_text(svg);args.output.with_suffix('.json').write_text(json.dumps(stats,indent=2)+'\n')
    # Standalone scientific plot, not an edited game screenshot.
    from PIL import Image,ImageDraw,ImageFont
    image=Image.new('RGB',(1200,620),'#101820');draw=ImageDraw.Draw(image)
    font=ImageFont.load_default(size=20);small=ImageFont.load_default(size=16)
    draw.text((40,25),title,fill='#e6edf2',font=font)
    draw.text((40,65),'Orange: original. Cyan: PN subdivision. Geometry-only orthographic projection.',fill='#e6edf2',font=small)
    draw.text((40,95),f'Y={bottom:.1f}..{top:.1f}; whole-island maximum silhouette shift {max(diffs):.2f} units',fill='#e6edf2',font=small)
    for values,color,width in ((za,'#ffab66',3),(zb,'#55dce0',2)):
        points=[(80+1040*i/(len(values)-1),500-360*(v-bottom)/(top-bottom)) for i,v in enumerate(values) if math.isfinite(v)]
        draw.line(points,fill=color,width=width)
    draw.text((40,555),f"{stats['source_triangles']:,} -> {stats['render_triangles']:,} triangles; original corners and shore band unchanged",fill='#e6edf2',font=small)
    image.save(args.output.with_suffix('.png'));print(json.dumps(stats))
if __name__=='__main__':main()
