#!/usr/bin/env python3
"""Project the native San Juan town into Puerto Rico's sea geometry.

Stock GM/BSP inputs are captured once in the native input owner. The small
coast replacement shares its render/collision triangles; stock partitions are
only pruned, never deformed. No engine, SAVE, fort or locator is rewritten.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path
import struct

from patch_metal_portobello_ground import HEADER, LABEL, MATERIAL, OBJECT, parse_gm, _name
from runtime_script_patch import atomic_write

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "src/assets/islands/puerto-rico"
LAYOUT = ASSETS / "layout.json"
MANIFEST = ASSETS / "manifest.json"
ORIGINALS = ROOT / "experiments/native-metal/inputs/native/puerto-rico-city-originals"
OUTPUT = ROOT / "experiments/native-metal/.cache/puerto-rico-city"
ISLAND = "RESOURCE/MODELS/Islands/PuertoRico/"
TOWN = "RESOURCE/MODELS/Locations/Town_PuertoRico/Town/"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def scale_measurements(source_root):
    """Report native unscaled GM bounds, including rigged character buffers."""
    names = (TOWN+"SanJuan.gm", "RESOURCE/MODELS/Characters/citiz_6.gm",
             "RESOURCE/MODELS/Characters/pirate_7.gm",
             "RESOURCE/MODELS/Ships/Brig1/Brig1.gm",
             "RESOURCE/MODELS/Ships/Frigate1/frigate1.gm",
             "RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm")
    result={}
    for name in names:
        captured=ORIGINALS/name
        source=captured if captured.exists() else source_root/name
        data=source.read_bytes()
        if source.is_symlink():
            raise ValueError(f"Linked scale input: {name}")
        if not captured.exists():
            captured.parent.mkdir(parents=True,exist_ok=True)
            atomic_write(captured,data)
        h=HEADER.unpack_from(data)
        offset=(HEADER.size+h[2]+h[3]*4+h[4]*4+h[5]*MATERIAL.size+
                h[6]*80+h[7]*LABEL.size+h[8]*OBJECT.size+h[9]*6)
        descriptors=[struct.unpack_from("<2i",data,offset+i*8) for i in range(h[10])]
        offset+=h[10]*8
        points=[]
        for typ,size in descriptors:
            stride=36+(typ&3)*8+(typ>>2)*8
            if size%stride or offset+size>len(data):
                raise ValueError(f"Invalid scale buffer: {name}")
            points.extend(struct.unpack_from("<3f",data,i) for i in range(offset,offset+size,stride))
            offset+=size
        low=[min(p[i] for p in points) for i in range(3)]
        high=[max(p[i] for p in points) for i in range(3)]
        result[name]={"sha256":sha(data),"captured":captured.relative_to(ROOT).as_posix(),
                      "bounds":[low,high],"dimensions_xyz":[b-a for a,b in zip(low,high)],
                      "header_dimensions_xyz":list(h[11:14]),"vertices":len(points)}
    return result


def layout():
    result = json.loads(LAYOUT.read_text())
    if result["version"] != 1:
        raise ValueError("Unsupported city layout")
    return result


def paths():
    return tuple(json.loads(MANIFEST.read_text())["files"])


def prepared():
    spec = json.loads(MANIFEST.read_text())
    if (spec["layout_sha256"] != sha(LAYOUT.read_bytes()) or
            spec["generator_sha256"] != sha(Path(__file__).read_bytes()) or
            spec.get("blocks_sha256") != sha(Path(__file__).with_name("puerto_rico_city_blocks.py").read_bytes())):
        raise ValueError("Puerto Rico recipe changed; prepare its candidate first")
    result = {}
    for name, row in spec["files"].items():
        path = OUTPUT / name
        if path.is_symlink() or not path.is_file() or sha(path.read_bytes()) != row["sha256"]:
            raise ValueError(f"Puerto Rico output missing or changed: {name}")
        result[name] = path
    return result


def metadata(data, children=(), texture_names=None):
    """Append native child labels without relocating any existing name index."""
    g = parse_gm(data)
    h = list(g["header"])
    strings = bytearray(g["strings"])
    names = list(struct.unpack_from(f"<{h[3]}i", data, HEADER.size + h[2]))
    identities = {_name(strings, index).casefold(): index for index in reversed(names)}

    def add(value):
        # GEOM resolves names to string pointers, then compares label-group
        # pointer identities. Equal text at a second offset is a different group.
        key=value.casefold()
        if key in identities:
            return identities[key]
        index = len(strings)
        strings.extend(value.encode("cp1251") + b"\0")
        names.append(index)
        identities[key]=index
        return index

    textures = list(struct.unpack_from(f"<{h[4]}i", data, g["texture_table_offset"]))
    for index, value in (texture_names or {}).items():
        textures[index] = add(value)
    labels_start = g["materials_offset"] + h[5] * MATERIAL.size + h[6] * 80
    labels_end = labels_start + h[7] * LABEL.size
    records = []
    for name, matrix in children:
        records.append(LABEL.pack(add("geometry"), add(name), 0, *matrix,
                                  0, 0, 0, 0, 0., 0., 0., 0.))
    old_material_start = g["materials_offset"]
    h[2], h[3], h[7] = len(strings), len(names), h[7] + len(records)
    return (HEADER.pack(*h) + strings + struct.pack(f"<{len(names)}i", *names) +
            struct.pack(f"<{len(textures)}i", *textures) +
            data[old_material_start:labels_end] + b"".join(records) + data[labels_end:])


def vertices(g, obj):
    buffer = g["buffers"][obj[7]]
    stride = buffer["stride"]
    fmt = struct.Struct("<6fI" + "f" * ((stride - 28) // 4))
    for tri in g["triangles"][obj[9]:obj[9] + obj[8]]:
        yield tuple(fmt.unpack_from(g["data"], buffer["payload_offset"] +
                                    (index + obj[11]) * stride) for index in tri)


def lerp(a, b, t):
    result = [x + (y - x) * t for x, y in zip(a, b)]
    result[6] = sum(round(((a[6] >> shift) & 255) * (1 - t) +
                          ((b[6] >> shift) & 255) * t) << shift
                    for shift in (0, 8, 16, 24))
    return tuple(result)


def clip(poly, axis, edge, positive):
    result = []
    if not poly:
        return result
    for a, b in zip(poly[-1:] + poly[:-1], poly):
        da, db = a[axis] - edge, b[axis] - edge
        ina, inb = (da >= -1e-7, db >= -1e-7) if positive else (da <= 1e-7, db <= 1e-7)
        if ina != inb:
            result.append(lerp(a, b, da / (da - db)))
        if inb:
            result.append(b)
    return result


def area(tri):
    a, b, c = tri
    return abs((b[0]-a[0])*(c[2]-a[2])-(b[2]-a[2])*(c[0]-a[0]))


def weight(x, z, spec):
    x0, z0, x1, z1 = spec["town_bounds"]
    d = max(x0-x, x-x1, z0-z, z-z1, 0.) / spec["blend_margin"]
    d = max(0., min(1., d))
    return 1. - d*d*(3.-2.*d)


def morph(vertex, spec):
    v = list(vertex)
    x, y, z = v[:3]
    if y < 0:
        return vertex
    w = weight(x, z, spec)
    delta = .01
    wx = (weight(x+delta,z,spec)-weight(x-delta,z,spec))/(2*delta)
    wz = (weight(x,z+delta,spec)-weight(x,z-delta,spec))/(2*delta)
    nx, ny, nz = v[3:6]
    n = ((1-w)*nx+y*wx*ny, ny, (1-w)*nz+y*wz*ny)
    length = math.sqrt(sum(t*t for t in n))
    v[1] = y*(1-w)
    if length > 1e-12:
        v[3:6] = [t/length for t in n]
    return tuple(v)


def patch_triangles(tri, spec):
    x0,z0,x1,z1 = spec["town_bounds"]
    margin, step = spec["blend_margin"], spec["blend_step"]
    cuts = []
    for axis, low, high in ((0,x0-margin,x1+margin),(2,z0-margin,z1+margin)):
        first, last = min(v[axis] for v in tri), max(v[axis] for v in tri)
        edges = [first, last]
        for i in range(math.ceil((high-low)/step)+1):
            value = min(high,low+i*step)
            if first+1e-6 < value < last-1e-6:
                edges.append(value)
        # Explicit inner/outer boundaries make the flat pad and return exact.
        for value in (low,low+margin,high-margin,high):
            if first+1e-6 < value < last-1e-6:
                edges.append(value)
        cuts.append(sorted(set(edges)))
    for a,b in zip(cuts[0],cuts[0][1:]):
        for c,d in zip(cuts[1],cuts[1][1:]):
            poly = list(tri)
            for axis,edge,positive in ((0,a,True),(0,b,False),(2,c,True),(2,d,False)):
                poly = clip(poly,axis,edge,positive)
            for i in range(1,len(poly)-1):
                piece = (poly[0],poly[i],poly[i+1])
                if area(piece) > 1e-7:
                    yield tuple(morph(v,spec) for v in piece)


def prune_bsp(data, g, removed):
    """Only delete face references; every retained plane/partition stays valid."""
    if not (g["header"][1] & 2):
        return data, 0
    start = g["parsed_end"]
    count, _, triangles = struct.unpack_from("<3i",data,start)
    node_start = start+12
    result = bytearray(data)
    stack, visited, pruned = [0], set(), 0
    while stack:
        index = stack.pop()
        if index in visited or not 0 <= index < count:
            raise ValueError("Invalid stock BSP topology")
        visited.add(index)
        offset = node_start+index*24
        bits = struct.unpack_from("<I",data,offset+16)[0]
        child, left, right, n = bits & ((1<<22)-1), (bits>>23)&1, (bits>>28)&3, (bits>>24)&15
        faces = [int.from_bytes(data[offset+20+j*3:offset+23+j*3],"little") for j in range(n)]
        if any(face >= triangles for face in faces):
            raise ValueError("Invalid stock BSP face")
        retained = [face for face in faces if face not in removed]
        pruned += n-len(retained)
        if len(retained) != n:
            struct.pack_into("<I",result,offset+16,(bits & ~(15<<24)) | (len(retained)<<24))
            for j, face in enumerate(retained):
                result[offset+20+j*3:offset+23+j*3] = face.to_bytes(3,"little")
        if left:
            stack.append(child)
        if right:
            stack.append(child+right)
    return bytes(result), pruned


def terrain_candidate(data, spec):
    g = parse_gm(data)
    result = bytearray(data)
    removed, patch, counts = set(), {}, defaultdict(int)
    x0,z0,x1,z1 = spec["town_bounds"]
    m = spec["blend_margin"]
    bounds = x0-m,z0-m,x1+m,z1+m
    for index,obj in enumerate(g["objects"]):
        name = _name(g["strings"],obj[1])
        material = g["materials"][obj[12]]
        texture = g["textures"][material[10]].lower()
        oldtown = name.lower() == "town"
        keep = []
        local = []
        for j,(indices,tri) in enumerate(zip(g["triangles"][obj[9]:obj[9]+obj[8]],vertices(g,obj))):
            selected = False
            if texture in spec["terrain_textures"]:
                polygon = list(tri)
                for axis,edge,positive in ((0,bounds[0],True),(0,bounds[2],False),(2,bounds[1],True),(2,bounds[3],False)):
                    polygon=clip(polygon,axis,edge,positive)
                selected = any(area((polygon[0],polygon[k],polygon[k+1]))>1e-7 for k in range(1,len(polygon)-1))
            if oldtown or selected:
                removed.add(obj[9]+j)
                counts["town" if oldtown else "terrain"] += 1
                if selected and not oldtown:
                    local.extend(patch_triangles(tri,spec))
            else:
                keep.append(indices)
        if local:
            patch[index] = (obj,local)
        if len(keep) != obj[8]:
            row = list(obj)
            row[8] = len(keep)
            if not keep:
                row[2] &= ~17
            OBJECT.pack_into(result,g["objects_offset"]+index*OBJECT.size,*row)
            for j,tri in enumerate(keep):
                struct.pack_into("<3H",result,g["triangles_offset"]+(obj[9]+j)*6,*tri)
    if counts["town"] not in (0, 2755) or not 1 <= counts["terrain"] <= 40:
        raise ValueError(f"Unexpected Puerto Rico roles: {dict(counts)}")
    result, refs = prune_bsp(bytes(result),g,removed)
    counts["pruned_bsp_refs"] = refs
    return result, patch, dict(counts)


def plane(tri):
    a,b,c = tri
    u,v = tuple(b[i]-a[i] for i in range(3)),tuple(c[i]-a[i] for i in range(3))
    n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    length=math.sqrt(sum(x*x for x in n))
    if length < 1e-10:
        raise ValueError("Degenerate generated terrain triangle")
    n=tuple(x/length for x in n)
    return n,sum(n[i]*a[i] for i in range(3))


def collision(triangles):
    """Small triangle-plane BSP; native traversal orders both sides of a ray."""
    points=[tuple(v[:3]) for tri in triangles for v in tri]
    positions=[tuple(points[3*i:3*i+3]) for i in range(len(triangles))]
    planes=[plane(tri) for tri in positions]
    node_count=0

    def build(ids, depth):
        nonlocal node_count
        node_count+=1
        if depth >= 220 or node_count > 100000:
            raise ValueError("Local terrain BSP exceeds bounded construction")
        choices=ids if len(ids)<12 else [ids[round(j*(len(ids)-1)/11)] for j in range(12)]
        best=None
        for candidate in choices:
            n,pd=planes[candidate]
            left,right,middle=[],[],[]
            split=0
            for face in ids:
                ds=[sum(n[i]*v[i] for i in range(3))-pd for v in positions[face]]
                low,high=min(ds),max(ds)
                if low>=-1e-4 and high<=1e-4:
                    middle.append(face)
                else:
                    if low < -1e-4:
                        left.append(face)
                    if high > 1e-4:
                        right.append(face)
                    split += low < -1e-4 and high > 1e-4
            score=4*split+abs(len(left)-len(right))-.5*len(middle)
            if best is None or score<best[0]:
                best=score,n,pd,left,right,middle
        _,n,pd,left,right,middle=best
        faces=middle[:15]
        left.extend(middle[15:])
        if not faces:
            raise ValueError("BSP split lost its pivot")
        return n,pd,faces,(build(left,depth+1) if left else None),(build(right,depth+1) if right else None)

    tree=build(list(range(len(triangles))),0)
    slots=[]

    def reserve(tree):
        if tree is None:
            return None
        index=len(slots)
        slots.extend(bytearray(24) for _ in range(1+max(0,math.ceil((len(tree[2])*3-4)/24))))
        return index

    def store(index, tree):
        n,pd,faces,left,right=tree
        children=[reserve(left),reserve(right)]
        li,ri=children
        base=li if li is not None else (ri-1 if ri is not None else 0)
        right_delta=ri-base if ri is not None else 0
        if right_delta>3:
            raise ValueError("Native BSP child offset overflow")
        bits=base | ((li is not None)<<23) | (len(faces)<<24) | (right_delta<<28)
        struct.pack_into("<4fI",slots[index],0,*n,pd,bits)
        payload=b"".join(face.to_bytes(3,"little") for face in faces)
        slots[index][20:24]=payload[:4].ljust(4,b"\0")
        for offset in range(4,len(payload)):
            slots[index+1+(offset-4)//24][(offset-4)%24]=payload[offset]
        for child,subtree in zip(children,(left,right)):
            if subtree:
                store(child,subtree)

    store(reserve(tree),tree)
    verts=b"".join(struct.pack("<3f",*p) for p in points)
    indices=b"".join((i).to_bytes(3,"little") for i in range(len(points)))
    return struct.pack("<3i",len(slots),len(points),len(triangles))+b"".join(slots)+verts+indices


def patch_model(template, groups):
    g=parse_gm(template)
    h=list(g["header"])
    objects, buffers, indices, triangles=[],[],[],[]
    positions=[]
    cumulative=0
    for obj,faces in groups.values():
        typ=g["buffers"][obj[7]]["type"]
        fmt=struct.Struct("<6fI"+"f"*((g["buffers"][obj[7]]["stride"]-28)//4))
        raw=[]
        rounded=[]
        for face in faces:
            tri=[]
            for v in face:
                packed=fmt.pack(*v)
                raw.append(packed)
                tri.append(fmt.unpack(packed))
            if area(tri)>1e-7:
                rounded.append(tuple(tri))
            else:
                del raw[-3:]
        count=len(rounded)*3
        if count>=60000:
            raise ValueError("Local terrain vertex capacity")
        row=list(obj)
        row[7],row[8],row[9],row[10],row[11]=len(buffers),len(rounded),len(indices),count,0
        row[13:21]=[-1]*8
        cumulative+=len(rounded)
        row[-1]=cumulative
        objects.append(OBJECT.pack(*row))
        buffers.append((typ,b"".join(raw)))
        indices.extend((i,i+1,i+2) for i in range(0,count,3))
        triangles.extend(rounded)
        positions.extend(v[:3] for tri in rounded for v in tri)
    low=[min(v[i] for v in positions) for i in range(3)]
    high=[max(v[i] for v in positions) for i in range(3)]
    center=[(a+b)/2 for a,b in zip(low,high)]
    radius=max(math.dist(v,center) for v in positions)
    h[1]=2
    h[6:11]=[0,0,len(objects),len(indices),len(buffers)]
    h[11:18]=[*(b-a for a,b in zip(low,high)),*center,radius]
    metadata_end=g["materials_offset"]+h[5]*MATERIAL.size
    return (HEADER.pack(*h)+template[HEADER.size:metadata_end]+b"".join(objects)+
            b"".join(struct.pack("<3H",*tri) for tri in indices)+
            b"".join(struct.pack("<2i",typ,len(raw)) for typ,raw in buffers)+
            b"".join(raw for _,raw in buffers)+collision(triangles))


def matrix(spec=None):
    if spec is None:
        return (1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.)
    angle=math.radians(spec["yaw_degrees"])
    c,s=math.cos(angle),math.sin(angle)
    x,y,z=spec["translation"]
    return (c,0.,-s,0.,0.,1.,0.,0.,s,0.,c,0.,x,y,z,1.)


def protected(root, spec=None):
    spec=layout() if spec is None else spec
    for name,expected in spec["protected"].items():
        path=root/name
        if path.is_symlink() or not path.is_file() or sha(path.read_bytes())!=expected:
            raise ValueError(f"Protected Puerto Rico asset changed: {name}")


def reflection_terrain(terrain):
    """Reflect the actual coast, without its underwater child or unused BSP."""
    g=parse_gm(terrain)
    h=list(g["header"])
    start=g["materials_offset"]+h[5]*MATERIAL.size+h[6]*80
    if h[7] != 1:
        raise ValueError("Unexpected primary island child layout")
    label=LABEL.unpack_from(terrain,start)
    if (_name(g["strings"],label[0]),_name(g["strings"],label[1])) != ("geometry","seabed"):
        raise ValueError("Unexpected primary island seabed owner")
    h[1] &= ~2
    h[7]=0
    return HEADER.pack(*h)+terrain[HEADER.size:start]+terrain[start+LABEL.size:g["parsed_end"]]


def prepare(source_root, review=False):
    spec=layout()
    source_root=source_root.resolve()
    protected(source_root,spec)
    stock={}
    for name,expected in spec["inputs"].items():
        source=ORIGINALS/name
        if not source.exists():
            original=source_root/name
            data=original.read_bytes()
            if original.is_symlink() or sha(data)!=expected:
                raise ValueError(f"Stock city input missing or changed: {name}")
            source.parent.mkdir(parents=True,exist_ok=True)
            atomic_write(source,data)
        data=source.read_bytes()
        if source.is_symlink() or sha(data)!=expected:
            raise ValueError(f"Captured city input changed: {name}")
        stock[name]=data
    shadow=source_root/"RESOURCE/Textures"/Path(spec["shadow_texture"].replace("\\","/")+".tx")
    if not shadow.is_file():
        raise ValueError("Native San Juan ground texture missing")
    children={}
    for role,name in (("city","SanJuan.gm"),("citybed","SanJuan_sb.gm"),
                      ("citysigns","SanJuan_signsA.gm"),("citymarket","SanJuan_rinok.gm")):
        data=stock[TOWN+name]
        g=parse_gm(data)
        replacements={i:spec["shadow_texture"] for i,t in enumerate(g["textures"]) if t.lower()=="shadow.tga"}
        children[role]=metadata(data,texture_names=replacements) if replacements else data
    from puerto_rico_city_blocks import build
    extension=build(stock,spec)
    modules={}
    for role,data in extension["modules"].items():
        g=parse_gm(data)
        replacements={i:spec["shadow_texture"] for i,t in enumerate(g["textures"]) if t.lower()=="shadow.tga"}
        modules[role]=metadata(data,texture_names=replacements) if replacements else data
    outputs=dict(extension["files"])
    metrics={"extension":extension["metrics"]}
    original=stock[ISLAND+"PuertoRico.gm"]
    terrain,groups,counts=terrain_candidate(original,spec)
    if counts["town"] != 2755:
        raise ValueError("Puerto Rico source town mismatch")
    blend=patch_model(original,groups)
    counts["blend_triangles"]=parse_gm(blend)["header"][9]
    counts["blend_bytes"]=len(blend)
    for stem in ("PuertoRico","PuertoRico_refl"):
        labels=([(role,matrix(spec)) for role in children]+[("cityblend",matrix())]+
                extension["children"])
        receiver=terrain if stem=="PuertoRico" else reflection_terrain(terrain)
        outputs[ISLAND+stem+".gm"]=metadata(receiver,labels)
        outputs[ISLAND+stem+"_cityblend.gm"]=blend
        for role,data in children.items():
            outputs[ISLAND+stem+"_"+role+".gm"]=data
        for role,data in modules.items():
            outputs[ISLAND+stem+"_"+role+".gm"]=data
        metrics[stem]={**counts,"terrain_source":"PuertoRico.gm",
                       "reflection_matches_primary":stem=="PuertoRico_refl"}
    files={}
    for name,data in outputs.items():
        target=OUTPUT/name
        if target.is_symlink():
            raise ValueError(f"Linked city output: {name}")
        target.parent.mkdir(parents=True,exist_ok=True)
        if not target.exists() or target.read_bytes()!=data:
            atomic_write(target,data)
        row={"sha256":sha(data),"consumer":"PuertoRico sea island/SEA_REFLECTION2 MODEL geometry child"}
        if name in spec["inputs"]:
            row["original_sha256"]=spec["inputs"][name]
        files[name]=row
    record={"version":1,"layout_sha256":sha(LAYOUT.read_bytes()),
            "generator_sha256":sha(Path(__file__).read_bytes()),
            "blocks_sha256":sha(Path(__file__).with_name("puerto_rico_city_blocks.py").read_bytes()),
            "files":files}
    if review:
        atomic_write(MANIFEST,(json.dumps(record,indent=2)+"\n").encode())
    elif not MANIFEST.exists() or json.loads(MANIFEST.read_text())!=record:
        raise ValueError("Unreviewed Puerto Rico outputs; inspect then freeze with --review")
    atomic_write(OUTPUT/"metrics.json",(json.dumps(metrics,indent=2)+"\n").encode())
    print(json.dumps(metrics,indent=2))
    return outputs


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root",type=Path,required=True)
    parser.add_argument("--review",action="store_true",help="Freeze exact candidate hashes after recipe edits")
    parser.add_argument("--measure-scale",action="store_true",help="Report and retain exact stock scale inputs")
    args=parser.parse_args()
    if args.measure_scale:
        print(json.dumps(scale_measurements(args.source_root.resolve()),indent=2))
        return
    prepare(args.source_root,args.review)


if __name__=="__main__":
    main()
