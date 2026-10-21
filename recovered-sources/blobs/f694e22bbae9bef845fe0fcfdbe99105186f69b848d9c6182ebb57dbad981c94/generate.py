#!/usr/bin/env python3
"""Retained actual-body fake-GPU probe; no real GPU, engine ABI or player proof."""
from pathlib import Path
import hashlib,json,subprocess,tempfile
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
SOURCE=ROOT/'experiments/native-metal/engine/src/libs/geometry/src/geom_static.cpp'
HEADER=ROOT/'experiments/native-metal/engine/src/libs/geometry/src/geom.h'
MARKERS=['void GEOM::ReleaseHullDraws(', 'bool GEOM::UploadLaserMesh(', 'bool GEOM::StageLaserProjection(', 'void GEOM::CommitLaserProjection(', 'void GEOM::DiscardLaserProjection(']
def extract(text,marker):
    start=text.index(marker);begin=text.index('{',start);end=begin+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
def digest(b):return hashlib.sha256(b).hexdigest()
source=SOURCE.read_text();header=HEADER.read_text()
bodies=[extract(source,m) for m in MARKERS]
fields=header[header.index('    struct HullOriginal '):header.index('    std::shared_ptr<const storm_ship_laser::Mesh> worldOriginal;')]
fixture=(HERE/'fixture.cpp').read_text().replace('// PRODUCTION_FIELDS',fields).replace('// PRODUCTION_BODIES','\n'.join(bodies))
(HERE/'probe.cpp').write_text(fixture)
paths=[SOURCE,HEADER,ROOT/'experiments/native-metal/ship_laser_geometry.hpp',ROOT/'experiments/native-metal/ship_hull_geometry.hpp',ROOT/'experiments/native-metal/ship_hull_damage_bridge.hpp']
receipt={'limitation':'Actual method bodies and actual field/type definitions; fake GPU service. Does not establish real renderer buffer ownership, Metal execution, installed app delivery or player acceptance.','before':{'paths':{str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in paths},'methods':{m:digest(b.encode()) for m,b in zip(MARKERS,bodies)},'fields':digest(fields.encode())}}
(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='corsairs-stage-rollback-'))
binary=scratch/'probe'
command=['xcrun','clang++','-std=c++20','-O0','-Wno-implicit-const-int-float-conversion','-I'+str(ROOT/'experiments/native-metal'),str(HERE/'probe.cpp'),'-o',str(binary)]
compiled=subprocess.run(command,text=True,capture_output=True)
log='COMMAND '+json.dumps(command)+'\nCOMPILE EXIT '+str(compiled.returncode)+'\n'+compiled.stdout+compiled.stderr
tested=None
if compiled.returncode==0:
    tested=subprocess.run([str(binary)],text=True,capture_output=True,timeout=30)
    log+='RUN EXIT '+str(tested.returncode)+'\n'+tested.stdout+tested.stderr
current=SOURCE.read_text();current_header=HEADER.read_text();current_fields=current_header[current_header.index('    struct HullOriginal '):current_header.index('    std::shared_ptr<const storm_ship_laser::Mesh> worldOriginal;')]
receipt['after']={'paths':{str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in paths},'methods':{m:digest(extract(current,m).encode()) for m in MARKERS},'fields':digest(current_fields.encode())}
receipt['methods_unchanged']=receipt['before']['methods']==receipt['after']['methods']
receipt['fields_unchanged']=receipt['before']['fields']==receipt['after']['fields']
(HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');(HERE/'probe.log').write_text(log);print(log,flush=True)
subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(scratch)],check=True)
assert compiled.returncode==0 and tested.returncode==0 and receipt['methods_unchanged'] and receipt['fields_unchanged']
