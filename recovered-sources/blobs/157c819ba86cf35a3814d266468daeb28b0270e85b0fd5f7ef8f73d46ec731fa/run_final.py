from pathlib import Path
import subprocess,json,hashlib
repo=Path.cwd();out=Path(__file__).resolve().parent;native=repo/'experiments/native-metal';objects=native/'.cache/build/native-engine/storm/libs/geometry/CMakeFiles/geometry.dir';links=[objects/'src/world_cut_planar.cpp.o',objects/'third_party/clipper2/src/clipper.engine.cpp.o',objects/'third_party/clipper2/src/clipper.triangulation.cpp.o'];receipt={'scope':'CPU-only real-stock source preparation and preserved material invariants; no game/GPU/delivery acceptance','runs':[]}
def run(name,source,extra=[],args=[]):
 cmd=['clang++','-std=c++20','-O1']+extra+['-I',str(native),'-I',str(native/'engine/src/libs/math/include'),'-I',str(native/'engine/src/libs/geometry/src'),str(source)]+list(map(str,links))+['-Wl,-dead_strip','-o',str(out/name)];r=subprocess.run(cmd,text=True,capture_output=True,timeout=60);(out/(name+'-compile.txt')).write_text(r.stdout+r.stderr)
 if r.returncode:raise SystemExit(r.stdout+r.stderr)
 info={'compile':cmd,'run':[str(out/name)]+args,'timeout_seconds':60}
 try:r=subprocess.run(info['run'],text=True,capture_output=True,timeout=60);info['exit']=r.returncode;log=r.stdout+r.stderr
 except subprocess.TimeoutExpired as e:info['exit']='timeout';log=(e.stdout or b'').decode()+(e.stderr or b'').decode()
 (out/(name+'.txt')).write_text(log);receipt['runs'].append(info);print(name,info['exit'],log,flush=True);(out/'final_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
run('material_final',out/'material_regression.cpp')
s=(native/'ship_hull_convex_material.hpp').read_text()
for needle in ['inline std::shared_ptr<Source>buildSource(std::shared_ptr<const MaterialCellSource>material,std::shared_ptr<const storm_ship_structure::Layout>layout={}){','inline bool partitionSurface(const Surface&source,const MaterialCells&retained,const std::vector<Body>&bodies,const std::map<int,float>&wood,std::vector<Surface>&out){']:
 assert s.count(needle)==1;label=needle[:needle.index('(')].split()[-1];s=s.replace(needle,needle+'ProbePhase probe_phase{"'+label+'"};')
(out/'instrumented/ship_hull_convex_material.hpp').write_text(s)
asset=repo/'.local-archives/metal-renderer-evidence/ship-collision-20261016/secondary-scene/native-hook/assets/08ec45ccffbc35fbc0c1a03c9c810ddfb1d8c5090b5845291160b11f22142949/flyingdutchman1.gm';run('stock_final',out/'stock_phase_probe.cpp',['-include',str(out/'probe_phase.hpp'),'-I',str(out/'instrumented')],[str(asset)])
receipt['asset']={'path':str(asset),'sha256':hashlib.sha256(asset.read_bytes()).hexdigest()};receipt['source_sha256']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [native/'ship_hull_material.hpp',native/'ship_hull_convex_material.hpp',native/'engine/src/libs/touch/src/ship_contact.hpp',out/'ownership_probe.cpp',out/'ownership_probe.txt',out/'projection_piece_probe.cpp',out/'projection_moments.txt']};receipt['ownership']='ownership_probe.txt: CHM3 encode/decode/rebase with freshly recreated material source, real split, allowance0 and resumed capacity; authoritative positive-volume delta4.62e-10';receipt['canonical_contact']='projection_moments.txt + ordinary_current.txt + contact_regression.txt; ordinary frozen witness differs only one velocity rounding4.44e-16';(out/'final_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
