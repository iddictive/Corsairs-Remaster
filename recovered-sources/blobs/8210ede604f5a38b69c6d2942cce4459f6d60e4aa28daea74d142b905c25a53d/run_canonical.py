from pathlib import Path
import subprocess,json,shutil
repo=Path.cwd();out=Path(__file__).resolve().parent;native=repo/'experiments/native-metal';receipt={'scope':'CPU canonical contact and stock phase diagnosis only','runs':[]}
def run(name,cmd,args=[],timeout=60):
 r=subprocess.run(cmd,text=True,capture_output=True,timeout=60);(out/(name+'-compile.txt')).write_text(r.stdout+r.stderr)
 if r.returncode:raise SystemExit(r.stdout+r.stderr)
 info={'compile':cmd,'run':[str(out/name)]+args,'timeout':timeout}
 try:r=subprocess.run(info['run'],text=True,capture_output=True,timeout=timeout);info['exit']=r.returncode;log=r.stdout+r.stderr
 except subprocess.TimeoutExpired as e:info['exit']='timeout';log=(e.stdout or b'').decode()+(e.stderr or b'').decode()
 (out/(name+'.txt')).write_text(log);info['output']=name+'.txt';receipt['runs'].append(info);(out/'canonical_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(name,info['exit'],log,flush=True)
for before in [True,False]:
 name='projection_'+('before' if before else 'after');cmd=['clang++','-std=c++20','-O1','-I',str(out/'baseline') if before else str(native),'-I',str(native),str(out/'projection_piece_probe.cpp'),'-o',str(out/name)]
 if before:cmd+=['-DBASELINE']
 run(name,cmd)
contact=out.parent/'contact';closure=out/'ordinary';closure.mkdir(exist_ok=True);shutil.copyfile(contact/'probe.cpp',closure/'probe.cpp');source=(contact/'after.hpp').read_text();source=source.replace('namespace frame_detail\n{','namespace frame_detail\n{\ninline std::size_t exactCalls = 0;').replace('    double lo, hi;','    ++exactCalls;\n    double lo, hi;');(closure/'query.hpp').write_text(source)
name='ordinary_current';run(name,['clang++','-std=c++20','-O2','-I',str(native),str(closure/'probe.cpp'),'-o',str(out/name)],['100']);same=(out/(name+'.txt')).read_text().split('persistent exact_calls=')[0]==(contact/'after-result.txt').read_text();receipt['ordinary_exact_frozen_stdout']=same;print('ordinary_exact_frozen_stdout',same,flush=True)
name='contact_regression';run(name,['clang++','-std=c++20','-O1','-I',str(native),str(repo/'.local-archives/metal-renderer-evidence/gemini-cleanup-20261017/ship/contact_probe.cpp'),'-o',str(out/name)])
objects=native/'.cache/build/native-engine/storm/libs/geometry/CMakeFiles/geometry.dir';links=[objects/'src/world_cut_planar.cpp.o',objects/'third_party/clipper2/src/clipper.engine.cpp.o',objects/'third_party/clipper2/src/clipper.triangulation.cpp.o'];asset=repo/'.local-archives/metal-renderer-evidence/ship-collision-20261016/secondary-scene/native-hook/assets/08ec45ccffbc35fbc0c1a03c9c810ddfb1d8c5090b5845291160b11f22142949/flyingdutchman1.gm';name='stock_phases';cmd=['clang++','-std=c++20','-O1','-include',str(out/'probe_phase.hpp'),'-I',str(out/'instrumented'),'-I',str(native),'-I',str(native/'engine/src/libs/math/include'),'-I',str(native/'engine/src/libs/geometry/src'),str(out/'stock_phase_probe.cpp')]+list(map(str,links))+['-Wl,-dead_strip','-o',str(out/name)];run(name,cmd,[str(asset)])
(out/'canonical_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
