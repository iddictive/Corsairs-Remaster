from pathlib import Path
import hashlib,json,re,shlex,subprocess,tempfile
from retain_inputs import retain
root=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs');here=Path(__file__).resolve().parent
source=root/'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp'
text=source.read_text()
tracked_headers=['experiments/native-metal/ship_fragment_contact.hpp','experiments/native-metal/ship_laser_geometry.hpp','experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.hpp','experiments/native-metal/ship_debris_bridge.hpp','experiments/native-metal/ship_hull_damage_bridge.hpp']
header_hashes={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in tracked_headers}
def extract(marker):
 start=text.index(marker);end=text.index('{',start)+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
markers=['std::vector<SHIP_CUT_FRAGMENTS::Group> SHIP_CUT_FRAGMENTS::Groups(', 'SHIP_CUT_FRAGMENTS::Piece *SHIP_CUT_FRAGMENTS::Find(', 'bool SHIP_CUT_FRAGMENTS::Adopt(', 'SHIP_CUT_FRAGMENTS::Hit SHIP_CUT_FRAGMENTS::Trace(', 'bool SHIP_CUT_FRAGMENTS::Queue(', 'bool SHIP_CUT_FRAGMENTS::Crush(', 'bool SHIP_CUT_FRAGMENTS::Impact(', 'bool SHIP_CUT_FRAGMENTS::Position(', 'bool SHIP_CUT_FRAGMENTS::Beam(', 'void SHIP_CUT_FRAGMENTS::LimitBodies(', 'void SHIP_CUT_FRAGMENTS::Chips(', 'void SHIP_CUT_FRAGMENTS::PollDamage(', 'void SHIP_CUT_FRAGMENTS::Blast(', 'void SHIP_CUT_FRAGMENTS::AdvanceCutFragments(']
helpers=['float fragmentScale(', 'CMatrix fragmentPose(', 'bool fragmentPolygon(']
cls=extract('class SHIP_CUT_FRAGMENTS final').replace('{','{\n public:',1)+';'
scaffold=(here/'scaffold.cpp').read_text()
probe=scaffold.replace('// EXTRACT_CLASS',cls).replace('// EXTRACT_HELPERS','\n'.join(extract(m) for m in helpers)).replace('// EXTRACT_METHODS','\n'.join(extract(m) for m in markers))
(here/'probe.cpp').write_text(probe)
ninja=(root/'experiments/native-metal/.cache/build/build.ninja').read_text()
m=re.search(r'^build .*?/ship_phonk_laser.cpp\.o:.*?\n(?P<body>(?:  .*\n)+)',ninja,re.M);assert m
includes=shlex.split(dict(re.findall(r'^  (\w+) = (.*)$',m['body'],re.M))['INCLUDES'])+['-I'+str(source.parent)]
scratch=Path(tempfile.mkdtemp(prefix='corsairs-secondary-scene-'))
cmd=['clang++','-std=c++20','-O1','-g','-w','-DSTORM_METAL_SHIP_HULL_DAMAGE','-fsanitize=address,undefined','-fno-omit-frame-pointer',*includes,'-I'+str(root/'experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/include'),str(here/'probe.cpp'),*[str(root/p) for p in ['experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.engine.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.triangulation.cpp']],'-o',str(scratch/'probe')]
manifest,cmd=retain('final-'+hashlib.sha256(text.encode()).hexdigest()[:16],here/'probe.cpp',cmd,header_hashes,[source])
c=subprocess.run(cmd,text=True,capture_output=True,timeout=60);log=c.stdout+c.stderr;r=None
if c.returncode==0:r=subprocess.run([str(scratch/'probe')],text=True,capture_output=True,timeout=45);log+=r.stdout+r.stderr
(here/'probe.log').write_text(log)
receipt={'retained_inputs_manifest':str(manifest.relative_to(here)),'retained_inputs_manifest_sha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),'compile':c.returncode,'run':None if r is None else r.returncode,'method_count':len(markers),'helper_count':len(helpers),'verdict':'component checks passed' if c.returncode==0 and r and r.returncode==0 else 'failed','command':cmd,'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'source_end_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'methods':{m:hashlib.sha256(extract(m).encode()).hexdigest() for m in markers+helpers},'headers':header_hashes,'headers_end':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in tracked_headers},'limitations':'Extracted actual scene class/methods/helpers with public probe access. Real CMatrix/CVECTOR/contact/prepareFragment/traceFragment; mocked Core/SHIP/MODEL/GEOS transaction/native services. No renderer, build/stage/app/SAVE/player acceptance.'}
(here/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(log)
subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(scratch)],check=True,timeout=30)
assert c.returncode==0 and r and r.returncode==0
