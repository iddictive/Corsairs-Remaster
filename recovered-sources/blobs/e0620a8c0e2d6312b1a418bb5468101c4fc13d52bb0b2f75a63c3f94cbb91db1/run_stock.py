from pathlib import Path
import subprocess,json,hashlib
repo=Path.cwd();out=Path(__file__).resolve().parent;native=repo/'experiments/native-metal';asset=repo/'.local-archives/metal-renderer-evidence/ship-collision-20261016/secondary-scene/native-hook/assets/08ec45ccffbc35fbc0c1a03c9c810ddfb1d8c5090b5845291160b11f22142949/flyingdutchman1.gm';objects=native/'.cache/build/native-engine/storm/libs/geometry/CMakeFiles/geometry.dir';links=[objects/'src/world_cut_planar.cpp.o',objects/'third_party/clipper2/src/clipper.engine.cpp.o',objects/'third_party/clipper2/src/clipper.triangulation.cpp.o'];receipt={'asset':str(asset),'asset_sha256':hashlib.sha256(asset.read_bytes()).hexdigest(),'scope':'Pristine stock root CPU-only bootstrap/capture/material prepare/contact projection. No player SAVE/queue/child/GPU acceptance.','cases':[]}
for baseline in [True,False]:
 name='stock_'+('baseline' if baseline else 'fixed');cmd=['clang++','-std=c++20','-O1','-I',str(out/'baseline') if baseline else str(native),'-I',str(native),'-I',str(native/'engine/src/libs/math/include'),'-I',str(native/'engine/src/libs/geometry/src'),str(out/'stock_probe.cpp')]+list(map(str,links))+['-Wl,-dead_strip','-o',str(out/name)]
 r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(out/(name+'-compile.txt')).write_text(r.stdout)
 if r.returncode:raise SystemExit(r.stdout)
 case={'compile':cmd,'run':[str(out/name),str(asset)],'timeout_seconds':60}
 try:r=subprocess.run(case['run'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60);case['exit']=r.returncode;log=r.stdout
 except subprocess.TimeoutExpired as e:case['exit']='timeout';log=(e.stdout or b'').decode() if isinstance(e.stdout,bytes) else e.stdout or ''
 (out/(name+'.txt')).write_text(log);case['stdout']=name+'.txt';receipt['cases'].append(case);print(name,case['exit'],log,flush=True)
 (out/'stock_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
receipt['source_sha256']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [out/'stock_probe.cpp',native/'ship_hull_material.hpp',native/'ship_hull_convex_material.hpp',native/'engine/src/libs/geometry/src/geom_static.cpp']}
(out/'stock_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
