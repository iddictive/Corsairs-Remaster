#!/usr/bin/env python3
"""Author Storm-native rowboat props and protagonist animation without a DCC.

Frame zero remains each source rig's exact inverse-bind frame. Authored actions
start at frame one. Source rigs and hull are captured, hash-bound and never edited.
Generated resources are cache projections, admitted through prepared().
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from patch_metal_portobello_ground import HEADER, MATERIAL, OBJECT, parse_gm
from runtime_script_patch import atomic_write

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "src/assets/port-rowboat"
LAYOUT = ASSETS / "layout.json"
MANIFEST = ASSETS / "manifest.json"
ORIGINALS = ROOT / "experiments/native-metal/inputs/native/port-rowboat-originals"
OUTPUT = ROOT / "experiments/native-metal/.cache/port-rowboat"
I = (0., 0., 0., 1.)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def add(a, b):
    return tuple(x+y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def scale(a, s):
    return tuple(x*s for x in a)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def unit(a):
    return scale(a, 1/math.sqrt(dot(a, a)))


def qm(a, b):
    x,y,z,w=a; X,Y,Z,W=b
    return (w*X+x*W+y*Z-z*Y, w*Y+y*W+z*X-x*Z,
            w*Z+z*W+x*Y-y*X, w*W-x*X-y*Y-z*Z)


def qi(q):
    return (-q[0], -q[1], -q[2], q[3])


def rotate(q, p):
    return qm(qm(q, (*p, 0.)), qi(q))[:3]


def axis(a, v):
    return (*scale(unit(a), math.sin(v/2)), math.cos(v/2))


def between(a, b):
    a,b=unit(a),unit(b)
    q=(*cross(a,b), 1+dot(a,b))
    if dot(q,q)<1e-12:
        return axis(cross(a, (0.,1.,0.) if abs(a[1])<.9 else (1.,0.,0.)), math.pi)
    return unit(q)


def blend(a, b, t):
    if t<=0:
        return a
    if t>=1:
        return b
    if dot(a,b)<0:
        b=scale(b,-1)
    return unit(add(scale(a,1-t), scale(b,t)))


def rig(data):
    frames,bones,fps=struct.unpack_from("<iif",data)
    parents=list(struct.unpack_from(f"<{bones}i",data,12))
    rest=[struct.unpack_from("<3f",data,12+4*bones+12*i) for i in range(bones)]
    roots=[struct.unpack_from("<3f",data,12+16*bones+12*i) for i in range(frames)]
    offset=12+16*bones+12*frames
    if len(data)!=offset+16*bones*frames:
        raise ValueError("Invalid AN layout")
    def pose(frame):
        return [struct.unpack_from("<4f",data,offset+16*(i*frames+frame)) for i in range(bones)]
    return {"frames":frames,"bones":bones,"fps":fps,"parents":parents,
            "rest":rest,"roots":roots,"pose":pose}


def fk(r, q, root):
    positions=[]; orientations=[]
    for i,parent in enumerate(r["parents"]):
        if i==0:
            positions.append(root); orientations.append(q[i])
        else:
            positions.append(add(positions[parent],rotate(orientations[parent],r["rest"][i])))
            orientations.append(qm(orientations[parent],q[i]))
    return positions,orientations


def reach(r, q, root, chain, end, target, offset=(0.,0.,0.)):
    # CCD uses actual rest lengths and quaternion composition from the Storm
    # parent/local contract. The two main hinges leave twist/detail joints intact.
    for _ in range(50):
        positions,orientations=fk(r,q,root)
        actual=add(positions[end],rotate(orientations[end],offset))
        if math.dist(actual,target)<.0002:
            return
        for joint in reversed(chain):
            positions,orientations=fk(r,q,root)
            actual=add(positions[end],rotate(orientations[end],offset))
            delta=between(sub(actual,positions[joint]),sub(target,positions[joint]))
            parent=r["parents"][joint]
            parent_q=orientations[parent] if parent>=0 else I
            q[joint]=unit(qm(qm(qi(parent_q),delta),orientations[joint]))


def oar(spec, side, phase, mode="row"):
    a=2*math.pi*phase
    theta=spec["oar_sweep"]*math.cos(a)
    beta=spec["oar_lift"]*math.sin(a)
    if (mode=="row_left" and side==-1) or (mode=="row_right" and side==1):
        theta=0.
        beta=spec["oar_feather"]
    ay=theta if side==1 else math.pi-theta
    az=beta if side==1 else -beta
    # CMatrix::BuildMatrix(0,ay,az): column-vector equivalent Ry*Rz.
    q=qm(axis((0.,1.,0.),ay),axis((0.,0.,1.),az))
    pivot=spec["oarlock_right" if side==1 else "oarlock_left"]
    return add(pivot,rotate(q,(spec["oar_grip_x"],0.,0.))),q


def grasp(r,q,root,chain,end,target,orientation,side):
    # Fingers extend along local X and curl around local Z. Align that curl
    # axis with the shaft/rung; solve the palm center, not a wrist-on-wood pose.
    palm=(.055*side,0.,0.)
    reach(r,q,root,chain,end,sub(target,rotate(orientation,palm)))
    positions,g=fk(r,q,root)
    q[end]=qm(qi(g[r["parents"][end]]),orientation)
    positions,g=fk(r,q,root)
    for joint in ([76,77,78,79,86,87,88,89] if side==-1 else [81,82,83,84,91,92,93,94]):
        q[joint]=axis((0.,0.,1.),-.75*side)
    return math.dist(add(positions[end],rotate(g[end],palm)),target)


def row(r, spec, phase, mode="row"):
    q=list(r["pose"](2766))
    root=r["roots"][2766]
    # Upper torso works against the stroke; seated hip, knees and feet retain
    # the existing actor-compatible sitting pose.
    q[2]=qm(q[2],axis((1.,0.,0.),.09*math.cos(2*math.pi*phase)))
    errors=[]
    for side,chain,end in [(-1,[17,22,37,53],65),(1,[18,23,38,54],66)]:
        target,oq=oar(spec,side,phase,mode)
        target=sub(target,spec["seat"])
        desired=qm(oq,axis((0.,1.,0.),-math.pi/2))
        errors.append(grasp(r,q,root,chain,end,target,desired,side))
    return root,q,max(errors)


def climb_contact(phase, stride):
    t=phase%1
    if t<=.6:
        return -stride*t
    u=(t-.6)/.4
    return -stride*t+stride*u*u*(3-2*u)


def climb(r, spec, phase):
    q=list(r["pose"](0)); root=list(r["roots"][0]);root[1]=spec["climb_root_y"]
    errors=[]
    for side,arm,end,leg,foot in [(-1,[17,37],65,[3,8],14),(1,[18,38],66,[4,9],15)]:
        t=phase+(0 if side==-1 else .5)
        y=climb_contact(t,spec["climb_stride"])
        hand=(side*spec["ladder_width"]/2,spec["ladder_rung_phase_y"]+4*spec["ladder_rung_spacing"]+y,spec["ladder_plane_z"])
        toe=(-side*.20,spec["ladder_rung_phase_y"]+spec["ladder_rung_spacing"]+climb_contact(t+.5,spec["climb_stride"]),spec["ladder_plane_z"]-.03)
        errors.append(grasp(r,q,root,arm,end,hand,axis((0.,1.,0.),-math.pi/2*side),side))
        reach(r,q,root,leg,foot,toe)
        p,_=fk(r,q,root)
        errors.append(math.dist(p[foot],toe))
    return root,q,max(errors)


def remap(source, target):
    """Resolve topology/rest coordinates, never assume equal numeric bone IDs."""
    result={0:0}
    for i in range(1,target["bones"]):
        parent=result[target["parents"][i]]
        matches=[j for j in range(1,source["bones"]) if source["parents"][j]==parent and
                 math.dist(source["rest"][j],target["rest"][i])<.0002]
        if len(matches)!=1:
            raise ValueError(f"Unsupported ambiguous protagonist joint {i}: {matches}")
        result[i]=matches[0]
    return result


def animation(source, target, spec, poses):
    mapping=remap(source,target)
    rows=poses["row"]
    climbs=poses["climb"]
    frames=[(target["roots"][0],target["pose"](0))]
    actions={}
    def emit(name,poses):
        first=len(frames)
        for root,q in poses:
            frames.append((root,[q[mapping[i]] for i in range(target["bones"])]))
        actions[name]=[first,len(frames)-1]
    emit("row",[(r,q) for r,q,_ in rows])
    standing=source["pose"](0); standing_root=source["roots"][0]
    seated_root,seated,_=rows[0]
    board=[]
    for i in range(31):
        t=i/30; t=t*t*(3-2*t)
        # Native MODEL owns the ship-to-seat path. The clip only lowers its
        # pelvis from the original standing root to the exact rowing root.
        root=add(scale(standing_root,1-t),scale(seated_root,t))
        board.append((root,[blend(a,b,t) for a,b in zip(standing,seated)]))
    emit("board",board)
    emit("leave",list(reversed(board)))
    emit("climb",[(r,q) for r,q,_ in climbs])
    climb_root,climb_q,_=climbs[0]
    over=[]
    for i in range(31):
        t=i/30; s=t*t*(3-2*t)
        root=add(climb_root,(0.,spec["climb_over_y"]*s,spec["climb_over_z"]*s))
        over.append((root,[blend(a,b,s) for a,b in zip(climb_q,standing)]))
    emit("climb_over",over)
    for mode in ("row_left","row_right"):
        emit(mode,[(r,q) for r,q,_ in poses[mode]])
    count=len(frames)
    data=(struct.pack("<iif",count,target["bones"],25.)+
          struct.pack(f'<{target["bones"]}i',*target["parents"])+
          b"".join(struct.pack("<3f",*p) for p in target["rest"])+
          b"".join(struct.pack("<3f",*p) for p,q in frames)+
          b"".join(struct.pack("<4f",*frames[f][1][b]) for b in range(target["bones"]) for f in range(count)))
    packed=rig(data)
    def angle(a,b):
        return 2*math.acos(min(1.,abs(dot(unit(a),unit(b)))))*180/math.pi
    continuity={}
    for name in ("row","row_left","row_right","climb"):
        first,last=actions[name]
        continuity[name]={"max_adjacent_joint_degrees":max(angle(a,b) for f in range(first,last)
                    for a,b in zip(packed["pose"](f),packed["pose"](f+1))),
                          "seam_joint_degrees":max(angle(a,b) for a,b in zip(packed["pose"](first),packed["pose"](last)))}
    if packed["pose"](0)!=target["pose"](0) or max(v["max_adjacent_joint_degrees"] for v in continuity.values())>15:
        raise ValueError("Authored animation bind/continuity failure")
    p,_=fk(packed,packed["pose"](1),packed["roots"][1])
    return data,actions,{"row_contact_error_max_m":max(x[2] for mode in ("row","row_left","row_right") for x in poses[mode]),
                         "climb_contact_error_max_m":max(x[2] for x in climbs),
                         "bind_frame_preserved":frames[0][1]==target["pose"](0),
                         "row_root_loop_error_m":math.dist(rows[0][0],rows[-1][0]),
                         "row_quaternion_loop_error":max(1-abs(dot(a,b)) for a,b in zip(rows[0][1],rows[-1][1])),
                         "serialized_continuity":continuity,
                         "row_hull_pelvis_xyz":add(p[0],spec["seat"]),
                         "bone_map":mapping}


def box(center,size):
    points=[add(center,(x*size[0]/2,y*size[1]/2,z*size[2]/2)) for x,y,z in
            [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    faces=[]
    for ids in [(0,3,2,1),(4,5,6,7),(0,4,7,3),(1,2,6,5),(3,7,6,2),(0,1,5,4)]:
        for tri in [(ids[0],ids[1],ids[2]),(ids[0],ids[2],ids[3])]:
            p=[points[i] for i in tri]
            normal=unit(cross(sub(p[1],p[0]),sub(p[2],p[0])))
            faces.append([(*v,*normal,0xffffffff,float(j==1),float(j==2)) for j,v in enumerate(p)])
    return faces


def model(template,faces):
    g=parse_gm(template);h=list(g["header"]);obj=list(g["objects"][0])
    points=[v[:3] for tri in faces for v in tri]
    lo=[min(v[i] for v in points) for i in range(3)];hi=[max(v[i] for v in points) for i in range(3)]
    c=scale(add(lo,hi),.5); radius=max(math.dist(v,c) for v in points)
    h[1]=2;h[6:11]=[0,0,1,len(faces),1];h[11:18]=[*sub(hi,lo),*c,radius]
    obj[3:7]=[*c,radius];obj[7:13]=[0,len(faces),0,len(points),0,0]
    obj[13:21]=[-1]*8;obj[-1]=len(faces)
    raw=b"".join(struct.pack("<6fI2f",*v) for tri in faces for v in tri)
    metadata_end=g["materials_offset"]+h[5]*MATERIAL.size
    return (HEADER.pack(*h)+template[HEADER.size:metadata_end]+OBJECT.pack(*obj)+
            b"".join(struct.pack("<3H",i,i+1,i+2) for i in range(0,len(points),3))+
            struct.pack("<2i",0,len(raw))+raw)


def paths():
    return tuple(json.loads(MANIFEST.read_text())["files"])


def script_initializer(spec, source):
    """Single recipe projects into the native controller's script attributes."""
    values={"version":1,"hull":r"Ships\BrigQeen1\BrigQeen1_boat",
            "oar":r"PortTravel\Oar","bench":r"PortTravel\Bench",
            "ladder":r"PortTravel\LadderSegment","hullTexturePath":"",
            "seatYaw":spec["seat_yaw"],"oarGripX":spec["oar_grip_x"],
            "oarSweep":spec["oar_sweep"],"oarLift":spec["oar_lift"],
            "oarFeather":spec["oar_feather"],
            "rowPeriod":spec["row_period_seconds"],"climbStride":spec["climb_stride"],
            "climbRootY":spec["climb_root_y"],"climbOverY":spec["climb_over_y"],
            "climbOverZ":spec["climb_over_z"],"ladderRungSpacing":spec["ladder_rung_spacing"],
            "ladderWidth":spec["ladder_width"],"ladderRungPhaseY":spec["ladder_rung_phase_y"],
            "ladderPlaneZ":spec["ladder_plane_z"],"boardStartX":0.,"boardStartY":0.,"boardStartZ":0.}
    values.update({"boardEndRootDelta"+a:v for a,v in zip("XYZ",sub(source["roots"][2766],source["roots"][0]))})
    for prefix,key in [("seat","seat"),("oarRight","oarlock_right"),("oarLeft","oarlock_left")]:
        values.update({prefix+axis:value for axis,value in zip("XYZ",spec[key])})
    values.update({f"animations.{name}":f"PortRowboat_{name}" for name in spec["supported_animations"]})
    text="// Generated by tools/prepare_port_rowboat.py from canonical layout.json.\nvoid PortTravel_InitRig(ref rig)\n{\n"
    for key,value in values.items():
        literal='"'+value.replace('\\','\\\\')+'"' if isinstance(value,str) else repr(value)
        text+=f"    rig.{key} = {literal};\n"
    return (text+"}\n").encode("ascii")


def prepared():
    spec=json.loads(MANIFEST.read_text())
    if spec["layout_sha256"]!=sha(LAYOUT.read_bytes()) or spec["generator_sha256"]!=sha(Path(__file__).read_bytes()):
        raise ValueError("Port rowboat recipe changed; regenerate first")
    result={}
    for name,row in spec["files"].items():
        path=OUTPUT/name
        if not path.is_file() or path.is_symlink() or sha(path.read_bytes())!=row["sha256"]:
            raise ValueError(f"Missing/changed port rowboat resource: {name}")
        result[name]=path
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root",type=Path,required=True)
    args=parser.parse_args()
    spec=json.loads(LAYOUT.read_text()); inputs={};outputs={};checks={}
    def capture(name,consumer):
        dest=ORIGINALS/name
        data=dest.read_bytes() if dest.is_file() else (args.source_root/name).read_bytes()
        if dest.is_symlink(): raise ValueError("Linked original input")
        if not dest.exists():
            dest.parent.mkdir(parents=True,exist_ok=True);atomic_write(dest,data)
        inputs[name]={"path":dest.relative_to(ROOT).as_posix(),"sha256":sha(data),"bytes":len(data),"consumer":consumer}
        return data
    hull=capture(spec["hull"],"PortTravel live rowboat hull and authoring prop material")
    if sha(hull)!=spec["hull_sha256"]: raise ValueError("Unexpected hull input")
    source=rig(capture("RESOURCE/animation/man.an","PortRowboat_man protagonist123-joint source rig"))
    poses={mode:[row(source,spec,i/60,mode) for i in range(61)] for mode in ("row","row_left","row_right")}
    poses["climb"]=[climb(source,spec,i/60) for i in range(61)]
    def save(name,data):
        path=OUTPUT/name;path.parent.mkdir(parents=True,exist_ok=True);atomic_write(path,data)
        outputs[name]={"sha256":sha(data),"bytes":len(data),"consumer":"Native PortTravel player model/rowboat"}
    for name in spec["supported_animations"]:
        target=source if name=="man" else rig(capture(f"RESOURCE/animation/{name}.an",f"PortRowboat_{name} protagonist rig"))
        data,actions,check=animation(source,target,spec,poses)
        if check["row_contact_error_max_m"]>.012 or check["climb_contact_error_max_m"]>.025:
            raise ValueError(f"Unreachable authored contact: {check}")
        out=f"PortRowboat_{name}"
        save(f"RESOURCE/animation/{out}.an",data)
        text=f"animation = {out}.an\n\n"
        for action,(first,last) in actions.items():
            text+=f"[{action}]\nspeed = 1.0\nloop = {'true' if action in ('row','row_left','row_right','climb') else 'false'}\ntype = normal\nstart_time = {first}\nend_time = {last}\ndata = \"Blend time\", \"0.0\"\n\n"
        save(f"RESOURCE/animation/{out}.ani",text.encode("ascii"));checks[name]=check
    save("RESOURCE/MODELS/PortTravel/Oar.gm",model(hull,box((.55,0.,0.),(2.26,.046,.046))+box((1.49,0.,0.),(.55,.035,.18))))
    save("RESOURCE/MODELS/PortTravel/Bench.gm",model(hull,box((0.,spec["bench_top"]-.025,0.),(1.20,.05,.30))))
    half=spec["ladder_width"]/2;spacing=spec["ladder_rung_spacing"]
    save("RESOURCE/MODELS/PortTravel/LadderSegment.gm",model(hull,box((-half,0.,0.),(.035,spacing,.035))+box((half,0.,0.),(.035,spacing,.035))+box((0.,0.,0.),(spec["ladder_width"],.035,.05))))
    save("PROGRAM/SEA_AI/PortTravelRig.c",script_initializer(spec,source))
    manifest={"version":1,"layout_sha256":sha(LAYOUT.read_bytes()),"generator_sha256":sha(Path(__file__).read_bytes()),
              "inputs":inputs,"files":outputs,"rig":spec,"checks":checks,
              "acceptance":"CPU contact/bind/loop checks only; native whole-frame rowing and ladder replay required."}
    atomic_write(MANIFEST,(json.dumps(manifest,indent=2,sort_keys=True)+"\n").encode())
    print(json.dumps({"files":len(outputs),"bytes":sum(r["bytes"] for r in outputs.values()),
                      "checks":{name:{k:v for k,v in row.items() if k!="bone_map"} for name,row in checks.items()}},sort_keys=True))


if __name__=="__main__":
    main()
