from pathlib import Path
import subprocess,json,hashlib
repo=Path.cwd();out=Path(__file__).resolve().parent;native=repo/'experiments/native-metal'
objects=native/'.cache/build/native-engine/storm/libs/geometry/CMakeFiles/geometry.dir'
links=[objects/'src/world_cut_planar.cpp.o',objects/'third_party/clipper2/src/clipper.engine.cpp.o',objects/'third_party/clipper2/src/clipper.triangulation.cpp.o']
receipt={'baseline':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'meaning':'CPU source checks only. No game replay/build/staging.','runs':[]}
for kind in ['clip','convex']:
 for baseline in [True,False]:
  name=kind+('_baseline' if baseline else '_fixed');cmd=['clang++','-std=c++20','-O1','-I',str(native)]
  if baseline:cmd+=['-D'+('MATERIAL' if kind=='clip' else 'CONVEX')+'_HEADER="'+str(out/('material_baseline.hpp' if kind=='clip' else 'convex_baseline.hpp'))+'"']
  cmd +=[str(out/(kind+'_probe.cpp'))]+[str(p) for p in links]+['-Wl,-dead_strip','-o',str(out/name)]
  r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(out/(name+'-compile.txt')).write_text(r.stdout)
  if r.returncode:raise SystemExit(r.stdout)
  r=subprocess.run([str(out/name)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(out/(name+'.txt')).write_text(r.stdout);print(name,r.returncode,r.stdout,flush=True)
  receipt['runs'].append({'command':cmd,'run':[str(out/name)],'exit':r.returncode,'stdout':name+'.txt'})
  if r.returncode:raise SystemExit(r.returncode)
receipt['sha256']={str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest() for p in list(out.glob('*.cpp'))+list(out.glob('*.hpp'))+links+[native/'ship_hull_material.hpp',native/'ship_hull_convex_material.hpp']}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
