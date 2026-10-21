from pathlib import Path
import json,hashlib,subprocess,tempfile,sys
root=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs');here=Path(__file__).resolve().parent
source=root/'experiments/native-metal/engine/src/libs/geometry/src/geom_static.cpp';header=root/'experiments/native-metal/engine/src/libs/geometry/src/geom.h'
inputs=[source,header,root/'experiments/native-metal/engine/src/libs/geometry/include/geos.h',root/'experiments/native-metal/ship_laser_geometry.hpp',root/'experiments/native-metal/ship_hull_geometry.hpp',root/'experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp']
before={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
def extract(text,marker):
 start=text.index(marker);begin=text.index('{',start);end=begin+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
markers=['void GEOM::ReleaseHullDraws(', 'bool GEOM::UploadLaserMesh(', 'bool GEOM::StageLaserProjection(', 'void GEOM::CommitLaserProjection(', 'void GEOM::DiscardLaserProjection(', 'bool GEOM::TraceLaserFragment(', 'bool GEOM::CaptureFragmentInput(', 'bool GEOM::ReplaceLaserFragment(']
text=source.read_text();h=header.read_text();fields=h[h.index('    struct HullOriginal '):h.index('    std::shared_ptr<const storm_ship_laser::Mesh> worldOriginal;')]
fixture=(root/'.local-archives/metal-renderer-evidence/phonk-auto-scene-20261016/stage-rollback/fixture.cpp').read_text().replace('int main(){','int stage_main(){').replace('// PRODUCTION_FIELDS',fields).replace('// PRODUCTION_BODIES','\n'.join(extract(text,m) for m in markers))
fixture=fixture.replace('void CommitLaserProjection(uint64_t);', '''bool TraceLaserFragment(uint64_t,uint64_t,uint64_t,const storm_ship_hull::Vec3&,const storm_ship_hull::Vec3&,float&,storm_ship_hull::Vec3&) const;
 bool CaptureFragmentInput(uint64_t,uint64_t,uint64_t,const storm_ship_laser::Plane&,unsigned,storm_ship_laser::FragmentInput&) const;
 bool ReplaceLaserFragment(uint64_t,storm_ship_laser::FragmentProjection&&,std::vector<storm_ship_laser::Fragment>&);
 void CommitLaserProjection(uint64_t);''')
fixture=fixture.replace('live handles at exit=0.\\n",failureCases,allocationCases);','live handles at exit=0.\\n",failureCases,allocationCases);return 0;')
probe=(here/'secondary.cpp').read_text().replace('// PRODUCTION_FIXTURE',fixture);(here/'probe.cpp').write_text(probe)
baseline='--baseline' in sys.argv
scratch=Path(tempfile.mkdtemp(prefix='corsairs-secondary-geometry-'))
command=['xcrun','clang++','-std=c++20','-O1','-g','-DSTORM_METAL_SHIP_HULL_DAMAGE','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(root/'experiments/native-metal'),'-I'+str(root/'experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/include'),str(here/('timber-baseline.cpp' if baseline else 'probe.cpp'))]+[str(root/p) for p in ['experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.engine.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.triangulation.cpp']]+['-o',str(scratch/'probe')]
compiled=subprocess.run(command,text=True,capture_output=True);log=compiled.stdout+compiled.stderr;run=None
if compiled.returncode==0:
 run=subprocess.run([str(scratch/'probe')],text=True,capture_output=True,timeout=45);log+=run.stdout+run.stderr
(here/('timber-baseline.log' if baseline else 'probe.log')).write_text(log);after={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs};receipt={'compile':compiled.returncode,'run':None if run is None else run.returncode,'command':command,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'inputs_before':before,'inputs_after':after,'inputs_unchanged':before==after,'methods':{m:hashlib.sha256(extract(text,m).encode()).hexdigest() for m in markers},'limitation':'Actual methods and fields, fake GPU/allocator. No Metal execution, staging, installed game or player replay.'};(here/('timber-baseline-receipt.json' if baseline else 'receipt.json')).write_text(json.dumps(receipt,indent=2)+'\n');print(log,flush=True)
subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(scratch)],check=True)
assert compiled.returncode==0 and run and run.returncode==0 and before==after
