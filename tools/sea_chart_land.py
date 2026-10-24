"""Offline continental fill in the physical atlas frame; no runtime meshing."""
import math
from pathlib import Path
import struct
import subprocess

from patch_metal_portobello_ground import parse_gm
from prepare_puerto_rico_city import clip, vertices, lerp, metadata, patch_model
from prepare_seamless_sea_assets import OUTPUT, sha, source

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT/'experiments/native-metal/engine/src/libs/geometry/third_party/clipper2'
UNITS = 32768


def polygons(mode, paths, clips=None):
    """Clipper2 uses an integer grid finer than the native chart float grid."""
    cpp = ROOT/'tools/seamless_sea_chart.cpp'
    sources = [cpp, VENDOR/'src/clipper.engine.cpp', VENDOR/'src/clipper.triangulation.cpp']
    headers = sorted((VENDOR/'include/clipper2').glob('*.h'))
    compiler = subprocess.check_output(['/usr/bin/c++', '--version']).splitlines()[0]
    key = sha(b''.join(p.read_bytes() for p in sources+headers)+compiler+b'-std=c++17 -O2')
    directory = OUTPUT/'chart'
    directory.mkdir(exist_ok=True)
    executable = directory/('polygons-'+key[:16])
    if not executable.is_file():
        subprocess.run(['/usr/bin/c++', '-std=c++17', '-O2', '-I'+str(VENDOR/'include'),
                        *map(str,sources), '-o', str(executable)], check=True, timeout=120)
    content = struct.pack('<I',len(paths))
    for path in paths:
        content += struct.pack('<I',len(path))+b''.join(struct.pack('<2q',*p) for p in path)
    if clips is not None:
        content+=struct.pack('<I',len(clips))
        for path in clips:
            content+=struct.pack('<I',len(path))+b''.join(struct.pack('<2q',*p) for p in path)
    identity = sha(mode.encode()+content+key.encode())
    source, target = directory/(identity+'.input'), directory/(identity+'.polygons')
    if not target.is_file():
        source.write_bytes(content)
        subprocess.run([str(executable), mode, str(source), str(target)], check=True, timeout=120)
    raw = target.read_bytes()
    count, = struct.unpack_from('<I',raw);offset=4
    result=[]
    for _ in range(count):
        size, = struct.unpack_from('<I',raw,offset);offset+=4
        result.append([struct.unpack_from('<2q',raw,offset+i*16) for i in range(size)])
        offset+=size*16
    if offset!=len(raw):
        raise ValueError('Trailing chart polygon data')
    return result


def mainland_surface(files, prepared):
    """Use projected native render triangles, retaining waterline provenance."""
    tree=prepared['model_trees']['Mainland']['terrain']
    tx,tz=prepared['atlas_frames']['bodies']['Mainland']['body_to_atlas'][4:]
    tx/=50;tz/=50
    paths=[];water=set();heights={}

    def visit(entry):
        data=files['RESOURCE/MODELS/SeamlessSeaChart/'+Path(entry['path']).name]
        g=parse_gm(data)
        for obj in g['objects']:
            if not obj[2]&1:
                continue
            for triangle in vertices(g,obj):
                poly=clip(list(triangle),1,0.,True)
                points=[(round((p[0]+tx)*UNITS),round((p[2]+tz)*UNITS)) for p in poly]
                if len(points)<3:
                    continue
                area=sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1]))
                if not area:
                    continue
                if area<0:
                    points.reverse()
                paths.append(points)
                for p in poly:
                    key=(round((p[0]+tx)*UNITS),round((p[2]+tz)*UNITS))
                    heights[key]=max(heights.get(key,-math.inf),p[1])
                    if abs(p[1])<1e-7:
                        water.add(key)
        for child in entry.get('children',[]):
            visit(child)
    visit(tree)
    return polygons('union',paths),water,heights


def signed_area(path):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(path,path[1:]+path[:1]))/2


def arc(path, start, finish):
    return (path[start:]+path[:start])[:(finish-start)%len(path)+1]


def inland_boundary(stock, native):
    """Replace only the stock mainland's Caribbean arc with native backland.

    The physical surface fixes the northern/eastern ends; keep the authored
    Pacific/western outline, inland lake holes and quest geography unchanged.
    The arc containing the western extremum is unambiguously the retained side.
    """
    coast=max(native,key=lambda p:abs(signed_area(p)))
    north=max(range(len(coast)),key=lambda i:coast[i][1])
    east=max(range(len(coast)),key=lambda i:coast[i][0])
    # The western/southern arc bounds native land on its inland side.
    candidates=[arc(coast,north,east),list(reversed(arc(coast,east,north)))]
    back=min(candidates,key=lambda p:sum(math.dist(a,b) for a,b in zip(p,p[1:])))
    original=max(stock,key=lambda p:abs(signed_area(p)))
    nearest=lambda p:min(range(len(original)),key=lambda i:math.dist(p,original[i]))
    a,b=nearest(back[0]),nearest(back[-1])
    candidates=[arc(original,b,a),list(reversed(arc(original,a,b)))]
    retained=min(candidates,key=lambda p:min(v[0] for v in p))
    boundary=back+retained
    if signed_area(boundary)<0:boundary.reverse()
    holes=[p for p in stock if signed_area(p)<0]
    result=polygons('difference',polygons('union',[boundary,*holes]),native)
    overlap=polygons('intersection',result,native)
    overlap_area=sum(signed_area(p) for p in overlap)/(UNITS*UNITS)
    # The native band and inland fill may share an edge, never surface area.
    if abs(overlap_area)>.01:
        raise ValueError(f'Continental fill overlaps physical coast: {overlap_area}')
    return result,back


def weights(p, triangle):
    a,b,c=triangle
    cross=lambda a,b:a[0]*b[1]-a[1]*b[0]
    edge=lambda a,b:(b[0]-a[0],b[2]-a[2])
    denominator=cross(edge(a,b),edge(a,c))
    if abs(denominator)<1e-9:return None
    q=(p[0]-a[0],p[1]-a[2])
    b_=cross(q,edge(a,c))/denominator
    c_=cross(edge(a,b),q)/denominator
    return 1-b_-c_,b_,c_


def interpolate(p, triangle):
    w=weights(p,triangle)
    if w is None:raise ValueError('Vertical mainland texture projection')
    result=[sum(w[i]*triangle[i][k] for i in range(3)) for k in range(len(triangle[0]))]
    result[0],result[2]=p
    result[6]=sum(max(0,min(255,round(sum(w[i]*((triangle[i][6]>>s)&255) for i in range(3)))))<<s
                  for s in (0,8,16,24))
    return result


def closest_point(p, triangle):
    w=weights(p,triangle)
    if w is not None and min(w)>=0:return p
    candidates=[]
    for a,b in zip(triangle,triangle[1:]+triangle[:1]):
        dx,dz=b[0]-a[0],b[2]-a[2];length=dx*dx+dz*dz
        t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[2])*dz)/length)) if length else 0.
        candidates.append((a[0]+t*dx,a[2]+t*dz))
    return min(candidates,key=lambda q:math.dist(p,q))


def prepare_mainland(source_root, files, prepared):
    """Retain stock inland relief/materials; attach it behind the native coast.

    This replaces the monolithic stock backdrop (which also embeds obsolete
    miniatures). It never changes native coastal vertices or physical frames.
    The same generated child supplies chart render and collision geometry.
    """
    frame=prepared['atlas_frames']['bodies']['Mainland']['body_to_atlas']
    if any(abs(a-b)>1e-9 for a,b in zip(frame[:4],(1.,0.,0.,1.))):
        raise ValueError('Continental fill requires the declared unrotated Mainland frame')
    path='RESOURCE/MODELS/WorldMap/islands/mein.gm'
    original=source(source_root,path).read_bytes();g=parse_gm(original)
    rows=[];subjects=[]
    for obj in g['objects']:
        texture=g['textures'][g['materials'][obj[12]][10]].casefold()
        if texture not in ('continent1.tga','continent2.tga','continent3.tga'):continue
        for triangle in vertices(g,obj):
            poly=clip(list(triangle),1,0.,True)
            for i in range(1,len(poly)-1):
                face=(poly[0],poly[i],poly[i+1])
                points=[(round(v[0]*UNITS),round(v[2]*UNITS)) for v in face]
                if abs(signed_area(points))<1:continue
                if signed_area(points)<0:points.reverse()
                rows.append((obj,face));subjects.append(points)
    stock=polygons('union',subjects)
    native,water,heights=mainland_surface(files,prepared)
    fill,back=inland_boundary(stock,native)
    clipped=polygons('clip-mesh',subjects,fill)
    gap=polygons('difference',fill,stock)
    missing=polygons('triangulate',gap)
    # Source texture lookup is offline. Keep each material's native UV map;
    # new connective land extends the nearest stock continental surface.
    buckets={};step=32.
    mainland_faces={piece[0][0] for piece in clipped}
    for index,(obj,tri) in enumerate(rows):
        # continent1 also owns the actual Mexico/Yucatan mainland. Excluding
        # its whole material projected this join from distant continents.
        # Geometry membership excludes unrelated islands without losing land.
        if index not in mainland_faces:continue
        for x in range(math.floor(min(v[0] for v in tri)/step),math.floor(max(v[0] for v in tri)/step)+1):
            for z in range(math.floor(min(v[2] for v in tri)/step),math.floor(max(v[2] for v in tri)/step)+1):
                buckets.setdefault((x,z),set()).add(index)

    def nearest(p):
        key=(math.floor(p[0]/step),math.floor(p[1]/step));found=set()
        for r in range(128):
            for x in range(-r,r+1):
                for z in ({-r,r} if abs(x)!=r else range(-r,r+1)):
                    found.update(buckets.get((key[0]+x,key[1]+z),()))
            if found:break
        if not found:raise ValueError('Continental texture source outside finite search')
        def distance(index):
            tri=rows[index][1];w=weights(p,tri)
            if w is not None and min(w)>=0:return 0.
            best=math.inf
            for a,b in zip(tri,tri[1:]+tri[:1]):
                dx,dz=b[0]-a[0],b[2]-a[2];length=dx*dx+dz*dz
                t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[2])*dz)/length)) if length else 0.
                best=min(best,(p[0]-a[0]-t*dx)**2+(p[1]-a[2]-t*dz)**2)
            return best
        return rows[min(found,key=lambda i:(distance(i),i))]

    # A position has one relief height, including material/triangle boundaries.
    # Selecting and clamping relief per connective face split shared vertices
    # by up to eleven map units. Retain captured stock heights at the join;
    # outside that surface sample its nearest edge instead of extrapolating.
    stock_heights={}
    for piece in clipped:
        triangle=rows[piece[0][0]][1]
        for x,z in piece[1:]:
            height=interpolate((x/UNITS,z/UNITS),triangle)[1]
            stock_heights[(x,z)]=max(stock_heights.get((x,z),-math.inf),height)
    relief={}

    # Use native backland heights on the shared join, then return smoothly to
    # authored stock relief over four map units (200 native metres).
    segments={};blend=4.
    for a,b in zip(back,back[1:]):
        for x in range(math.floor(min(a[0],b[0])/UNITS/blend)-1,math.floor(max(a[0],b[0])/UNITS/blend)+2):
            for z in range(math.floor(min(a[1],b[1])/UNITS/blend)-1,math.floor(max(a[1],b[1])/UNITS/blend)+2):
                segments.setdefault((x,z),[]).append((a,b))
    tx,tz=prepared['atlas_frames']['bodies']['Mainland']['body_to_atlas'][4:];tx/=50;tz/=50

    def finish(v):
        x,z=v[0],v[2];grid_key=(round(x*UNITS),round(z*UNITS))
        # Distinct clipping-grid points can collapse onto one GM float vertex.
        key=struct.pack('<2f',x-tx,z-tz)
        if key in relief:
            v[1]=relief[key];v[0]-=tx;v[2]-=tz
            return tuple(v)
        if grid_key in stock_heights:v[1]=stock_heights[grid_key]
        else:
            triangle=nearest((x,z))[1]
            v[1]=max(0,interpolate(closest_point((x,z),triangle),triangle)[1])
        best=blend;height=None
        for a,b in segments.get((math.floor(x/blend),math.floor(z/blend)),()):
            ax,az=a[0]/UNITS,a[1]/UNITS;dx,dz=(b[0]-a[0])/UNITS,(b[1]-a[1])/UNITS
            length=dx*dx+dz*dz
            t=max(0,min(1,((x-ax)*dx+(z-az)*dz)/length)) if length else 0.
            distance=math.hypot(x-ax-t*dx,z-az-t*dz)
            if distance<best:
                best=distance;height=heights.get(a,0)*(1-t)+heights.get(b,0)*t
        if height is not None:
            weight=best/blend;weight=weight*weight*(3-2*weight)
            v[1]=height*(1-weight)+max(0,v[1])*weight
        relief[key]=v[1]
        v[0]-=tx;v[2]-=tz
        return tuple(v)

    groups={};count=0
    def add(obj,face):
        nonlocal count
        # Keep the native upward winding and bounded 16-bit render buffers.
        if signed_area([(v[0],v[2]) for v in face])>0:face.reverse()
        key=(obj[12],count//18000)
        groups.setdefault(key,(obj,[]))[1].append(tuple(finish(v) for v in face));count+=1
    for piece in clipped:
        index=piece[0][0];obj,triangle=rows[index]
        add(obj,[interpolate((x/UNITS,z/UNITS),triangle) for x,z in piece[1:]])
    for piece in missing:
        p=(sum(x for x,z in piece)/3/UNITS,sum(z for x,z in piece)/3/UNITS)
        obj,triangle=nearest(p)
        face=[interpolate((x/UNITS,z/UNITS),triangle) for x,z in piece]
        add(obj,face)
    walls=[]
    for polygon in fill:
        for a,b in zip(polygon,polygon[1:]+polygon[:1]):
            a=(a[0]/UNITS-tx,0.,a[1]/UNITS-tz);b=(b[0]/UNITS-tx,0.,b[1]/UNITS-tz)
            if math.dist(a,b)<1e-5:continue
            c=(b[0],.5,b[2]);d=(a[0],.5,a[2]);walls.extend(((a,b,c),(a,c,d)))
    model=patch_model(original,groups,collision_faces=walls)
    # Stock UV/material slots remain byte-for-byte; only qualify their paths.
    textures={i:'RESOURCE\\Textures\\WorldMap\\geometry\\'+name for i,name in enumerate(g['textures'])}
    for name in textures.values():source(source_root,name+'.tx')
    model=metadata(model,texture_names=textures)
    root='RESOURCE/MODELS/SeamlessSeaChart/Mainland.gm'
    destination='RESOURCE/MODELS/SeamlessSeaChart/Mainland_inland.gm'
    identity=[1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.]
    files[root]=metadata(files[root],children=[('inland',identity)])
    files[destination]=model
    return {'source':{'path':path,'sha256':sha(original)},'generator_sha256':sha(Path(__file__).read_bytes()),
            'polygon_tool_sha256':sha((ROOT/'tools/seamless_sea_chart.cpp').read_bytes()),
            'stock_triangles':g['header'][9],'retained_triangles':len(clipped),'connective_triangles':len(missing),
            'output_triangles':count,'collision_walls':len(walls),'native_overlap_area_map_units':0,
            'native_coast_unchanged':True,'asset':destination}
