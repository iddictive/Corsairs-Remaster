#!/usr/bin/env python3
"""Project coincident legacy island copies onto pinned native body owners.

Coordinates, model identity, lighting, depth grids and independently posed
coasts stay native. No global atlas is activated and no SAVE is modified.
"""
from __future__ import annotations
from collections import defaultdict
import argparse
import hashlib
import itertools
import json
import math
from pathlib import Path
import resource
import struct
import time

import prepare_seamless_sea_assets as sea
from patch_metal_portobello_ground import HEADER, OBJECT, parse_gm
from prepare_puerto_rico_city import vertices
from runtime_script_patch import atomic_write
from metal_storage import ensure_space

ROOT=Path(__file__).resolve().parents[1]
MANIFEST=sea.OWNER/'legacy-ownership.json'
OUTPUT=ROOT/'experiments/native-metal/.cache/legacy-sea-geometry'
DEFAULT_SOURCE=ROOT/'experiments/native-metal/.cache/runtime'
INI='RESOURCE/INI/legacy_sea_geometry.ini'
# Reflection roots have independent distance fades. Their source membership
# cannot transfer visibility merely because terrain has a complete resident set.
ROLES=('terrain','child:seabed')

def digest(data):return hashlib.sha256(data).hexdigest()
def dependency_hashes():
    paths=[Path(__file__),Path(sea.__file__),sea.OWNER/'recipe.json',sea.OWNER/'manifest.json',
           ROOT/'tools/prepare_puerto_rico_city.py',ROOT/'tools/patch_metal_portobello_ground.py']
    return {p.relative_to(ROOT).as_posix():digest(p.read_bytes()) for p in paths}

def raw_source(root,entry):
    """Use the reviewed city projection before its ordinary atomic delivery."""
    path=sea.source(root,entry['path'])
    if digest(path.read_bytes())==entry['sha256']:return path
    import prepare_puerto_rico_city as city
    row=json.loads(city.MANIFEST.read_bytes())['files'].get(entry['path'])
    candidate=city.OUTPUT/entry['path']
    if row and row['sha256']==entry['sha256'] and candidate.is_file() and digest(candidate.read_bytes())==entry['sha256']:
        return candidate
    raise ValueError('Legacy geometry input changed: '+entry['path'])

def indices(rows):return {i for row in rows for a,b in row['face_ranges'] for i in range(a,b)}
def paired_ranges(pairs):
    result=[]
    for consumer,provider in pairs:
        if result and consumer==result[-1][0]+result[-1][2] and provider==result[-1][1]+result[-1][2]:result[-1][2]+=1
        else:result.append([consumer,provider,1])
    return result
def origin(row):return [float(format(float(v),'.6g'))*50 for v in row['map_anchor_xz']]
def origin_error(row):
    native=[v*50 for v in row['map_anchor_xz']]
    return math.sqrt(sum((a-b)**2 for a,b in zip(origin(row),native)))
def transform(matrix,point):
    return tuple(sum(matrix[k*4+j]*point[k] for k in range(3))+matrix[12+j] for j in range(3))
def projected_bytes(data,keep):
    """The extractor keeps source IDs; original bounds retain depth mapping."""
    result,detail=sea.extract(data,[{'face_ranges':sea.ranges(sorted(keep))}],retain_bounds=True,allow_empty=True)
    old,new=parse_gm(data),parse_gm(result)
    if result[:HEADER.size]!=data[:HEADER.size]:raise ValueError('Legacy header/bounds changed')
    for before,after in zip(old['buffers'],new['buffers']):
        if data[before['payload_offset']:before['payload_offset']+before['size']]!=result[after['payload_offset']:after['payload_offset']+after['size']]:
            raise ValueError('Legacy vertex/COL identities changed')
    return result,detail

def provider_masks(root,recipe,inventory):
    """Recipe authority is shared only when actual legacy instances coincide."""
    masks={};groups={};variant_evidence={}
    for body,spec in recipe['bodies'].items():
        base=spec['source_region'];base_origin=origin(inventory['regions'][base]);aligned=[]
        for region in spec['logical_regions']:
            local=spec['local_to_body'][region];point=origin(inventory['regions'][region])
            coincident=max(abs(a-b) for a,b in zip(local[:4],(1,0,0,1)))<=2e-6 and max(abs(point[i]-base_origin[i]-local[4+i]) for i in range(2))<=.05
            groups[region]={'body':body,'shared_instance':base if coincident else region,'legacy_origin':point,'local_to_body':local}
            if coincident:aligned.append(region)
            authority=spec if region==base else spec['regional_ownership'][region]
            for role in ROLES:
                entry=inventory['regions'][region]['files'].get(role)
                if entry is None:continue
                rows=authority.get('extract',{}).get(role)
                masks[(region,role)]=indices(rows) if rows else set(range(parse_gm(raw_source(root,entry).read_bytes())['header'][9]))
        # A separately posed source is a complete native coast, even when the
        # atlas recipe would compose it with another differently placed coast.
        for region in aligned:
            if region==base:continue
            masks[(region,'terrain')]=set()
        for name,patch in recipe.get('variants',{}).items():
            if patch['body']!=body or patch['region'] not in aligned or patch['region']==base:continue
            full={**patch,'local_to_body':spec['local_to_body'][patch['region']]}
            detail=sea.variant(root,inventory,full)
            role=patch['role'];local=indices([{'face_ranges':detail['local_faces']}]);replaced=indices([{'face_ranges':detail['replaced_body_faces']}])
            masks[(base,role)]-=replaced;masks[(patch['region'],role)]|=local
            variant_evidence[name]={'local_faces_sha256':digest(struct.pack('<%dI'%len(local),*sorted(local))),
                                    'local_faces':len(local),'replaced_faces':len(replaced)}
        surface=recipe.get('surface_composition',{}).get(body,{})
        for role,key in (('reflection','reflection_sources'),('child:seabed','seabed_sources')):
            sources=surface.get(key,[base])
            for region in aligned:
                if region not in sources and (region,role) in masks:masks[(region,role)]=set()
    return masks,groups,variant_evidence

def records(root,inventory,region,role):
    entry=inventory['regions'][region]['files'][role];path=raw_source(root,entry);data=path.read_bytes();g=parse_gm(data)
    matrix=[1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.]
    if role.startswith('child:'):
        matrix=next(r['matrix'] for r in inventory['regions'][region]['geometry_children'] if r['name']==role[6:])
    ox,oz=origin(inventory['regions'][region]);matrix=list(matrix);matrix[12]+=ox;matrix[14]+=oz
    for obj in g['objects']:
        for j,triangle in enumerate(vertices(g,obj)):
            yield obj[9]+j,tuple(transform(matrix,v[:3]) for v in triangle)

def cells(triangle,padding):
    center=[sum(v[k] for v in triangle)/3 for k in range(3)]
    return itertools.product(*(range(math.floor(v-padding),math.floor(v+padding)+1) for v in center))
def tolerance(triangle):
    largest=max(abs(x) for p in triangle for x in p)
    # Source correspondence is centimetric; four float ULPs bound the native
    # multiply/add pose roundoff. This is not a visual distance/AA threshold.
    return max(sea.WELD_METRES,4*2.**(math.frexp(largest)[1]-24))
def separation(a,b,limit):
    best=math.inf
    for shift in range(3):
        value=max(sum((a[k][j]-b[(k+shift)%3][j])**2 for j in range(3)) for k in range(3))
        best=min(best,value)
    return math.sqrt(best) if best<=limit*limit else None

def projection_ini(models,origins,provider_paths):
    exposures=sorted({(provider,row['region']) for row in models.values() for provider in row['providers']})
    ini=['[LegacySea]','version=1','scale=50','regions='+str(len(origins)),'models='+str(len(models)),'providers='+str(len(provider_paths)),'exposures='+str(len(exposures))]
    for i,(region,point) in enumerate(origins.items()):
        x,z=[v/50 for v in point];ini+=['',f'[Region{i}]','id='+region,f'mapX={x:.9g}',f'mapZ={z:.9g}']
    for i,(name,row) in enumerate(sorted(models.items())):
        ini+=['',f'[Model{i}]','source='+name[len('RESOURCE/MODELS/'):-3].replace('/','\\'),'projection='+row['projection'][len('RESOURCE/MODELS/'):-3].replace('/','\\')]
    for i,name in enumerate(provider_paths):ini+=['',f'[Provider{i}]','source='+name[len('RESOURCE/MODELS/'):-3].replace('/','\\')]
    for i,(provider,consumer) in enumerate(exposures):ini+=['',f'[Exposure{i}]','provider='+provider,'consumer='+consumer]
    return ('\n'.join(ini)+'\n').encode('ascii')

def prepare(root):
    started=time.monotonic();dependencies=dependency_hashes();recipe=json.loads((sea.OWNER/'recipe.json').read_bytes());inventory=json.loads((sea.OWNER/'manifest.json').read_bytes())
    masks,groups,variants=provider_masks(root,recipe,inventory);files={};models={};originals={};totals={}
    pose_errors={region:origin_error(row) for region,row in inventory['regions'].items()};largest_pose_error=max(pose_errors.values())
    for role in ROLES:
        providers=defaultdict(list)
        for region in inventory['regions']:
            if (region,role) not in masks:continue
            for face,triangle in records(root,inventory,region,role):
                if face not in masks[(region,role)]:continue
                cell=next(cells(triangle,0));providers[cell].append((region,face,triangle))
        removed_total=0
        for region in inventory['regions']:
            if (region,role) not in masks:continue
            entry=inventory['regions'][region]['files'][role];keep=[];removed=defaultdict(list);pairs=defaultdict(list);max_delta=0.
            for face,triangle in records(root,inventory,region,role):
                owner=None
                if face not in masks[(region,role)]:
                    bound=tolerance(triangle)+pose_errors[region]+largest_pose_error
                    for cell in cells(triangle,bound):
                        for other,other_face,candidate in providers.get(cell,()):
                            if other==region:continue
                            limit=max(tolerance(triangle),tolerance(candidate))+pose_errors[region]+pose_errors[other]
                            delta=separation(triangle,candidate,limit)
                            if delta is not None:owner=(other,other_face);max_delta=max(max_delta,delta);break
                        if owner:break
                if owner:removed[owner[0]].append(face);pairs[owner[0]].append((face,owner[1]))
                else:keep.append(face)
            count=sum(len(v) for v in removed.values());removed_total+=count
            if not count:continue
            source=raw_source(root,entry);content,detail=projected_bytes(source.read_bytes(),keep)
            output='RESOURCE/MODELS/LegacySea/'+entry['path'][len('RESOURCE/MODELS/'):]
            ensure_space(len(content),OUTPUT);atomic_write(OUTPUT/output,content)
            files[output]={'sha256':digest(content),'bytes':len(content)}
            models[entry['path']]={'projection':output,'source_sha256':entry['sha256'],'region':region,'role':role,
                                  'keep':sea.ranges(keep),'providers':{k:sea.ranges(v) for k,v in removed.items()},
                                  'correspondences':{k:paired_ranges(v) for k,v in pairs.items()},'maximum_world_vertex_delta':max_delta,**detail}
            originals[entry['path']]=entry['sha256']
        totals[role]=removed_total
    provider_paths=sorted({inventory['regions'][region]['files'][role]['path'] for region,role in masks})
    origins={region:origin(row) for region,row in inventory['regions'].items()}
    data=projection_ini(models,origins,provider_paths);atomic_write(OUTPUT/INI,data);files[INI]={'sha256':digest(data),'bytes':len(data)}
    if dependency_hashes()!=dependencies:raise ValueError('Legacy geometry sources changed during preparation')
    result={'version':1,'dependencies':dependencies,'files':files,'models':models,'source_files':originals,'legacy_instances':groups,'variants':variants,'origins':origins,'provider_paths':provider_paths,'native_origin_serialization_error_metres':pose_errors,
            'removed_faces':totals,'preparation_seconds':time.monotonic()-started,'peak_rss_bytes':resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
            'policy':'Recipe-owned providers at standard legacy poses only; independent coasts protected; original bounds/VB/COL/node/BSP IDs retained; no atlas activation'}
    atomic_write(MANIFEST,(json.dumps(result,indent=2)+'\n').encode());return result

def prepared():
    frozen=json.loads(MANIFEST.read_bytes())
    if frozen['dependencies']!=dependency_hashes():raise RuntimeError('Legacy geometry inputs changed; run prepare_legacy_sea_geometry.py before delivery')
    for relative,row in frozen['models'].items():
        target=OUTPUT/row['projection']
        if target.is_file() and digest(target.read_bytes())==frozen['files'][row['projection']]['sha256']:continue
        entry={'path':relative,'sha256':row['source_sha256']};data=raw_source(DEFAULT_SOURCE,entry).read_bytes();content,_=projected_bytes(data,indices([{'face_ranges':row['keep']}]))
        if digest(content)!=frozen['files'][row['projection']]['sha256']:raise RuntimeError('Legacy geometry projection changed: '+relative)
        atomic_write(target,content)
    ini=projection_ini(frozen['models'],frozen['origins'],frozen['provider_paths'])
    if digest(ini)!=frozen['files'][INI]['sha256']:raise RuntimeError('Legacy geometry admission metadata changed')
    if not (OUTPUT/INI).is_file() or (OUTPUT/INI).read_bytes()!=ini:atomic_write(OUTPUT/INI,ini)
    result={name:OUTPUT/name for name in frozen['files']}
    for name,path in result.items():
        if not path.is_file() or digest(path.read_bytes())!=frozen['files'][name]['sha256']:raise RuntimeError('Legacy geometry preparation missing: '+name)
    return result

def paths():
    return tuple(json.loads(MANIFEST.read_bytes())['files'])

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source-root',type=Path,default=DEFAULT_SOURCE);args=parser.parse_args()
    result=prepare(args.source_root.resolve());print('Legacy sea:',len(result['models']),'projected GMs;',sum(result['removed_faces'].values()),'copied faces removed;',round(result['preparation_seconds'],3),'seconds')
