from pathlib import Path
import hashlib,json,subprocess,tempfile,shutil
root=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs');here=Path(__file__).resolve().parent
asset=root/'.local-archives/metal-renderer-evidence/ship-collision-20261016/secondary-scene/native-hook/assets/08ec45ccffbc35fbc0c1a03c9c810ddfb1d8c5090b5845291160b11f22142949/flyingdutchman1.gm'
scratch=Path(tempfile.mkdtemp(prefix='corsairs-pure-actual-seed-'))
common=['xcrun','clang++','-std=c++20','-O1','-g','-DSTORM_METAL_SHIP_HULL_DAMAGE','-fsanitize=address,undefined','-fno-omit-frame-pointer']+['-I'+str(root/p) for p in ['experiments/native-metal','experiments/native-metal/engine/src/libs/math/include','experiments/native-metal/engine/src/libs/geometry/src','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/include']]
inputs=[root/'experiments/native-metal/ship_laser_geometry.hpp',root/'experiments/native-metal/ship_hull_geometry.hpp',root/'experiments/native-metal/ship_wood_surface_profiles.hpp',root/'experiments/native-metal/engine/src/libs/geometry/src/rdf.h',root/'experiments/native-metal/engine/src/libs/geometry/src/geom_static.cpp',root/'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp',asset,here/'probe.cpp']
links=[root/p for p in ['experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.engine.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.triangulation.cpp']];inputs+=links
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p):digest(p) for p in inputs};saved=here/'inputs'
for p in inputs:
 relative=p.relative_to(root);out=saved/relative;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,out)
commands=[];log='';run=None
command=common+[str(here/'probe.cpp'),'-MMD','-MF',str(scratch/'probe.d'),'-c','-o',str(scratch/'probe.o')];commands.append(command);compiled=subprocess.run(command,text=True,capture_output=True);log+=compiled.stdout+compiled.stderr
if compiled.returncode==0:
 dependencies=(scratch/'probe.d').read_text().replace('\\\n',' ').split(':',1)[1].split()
 for name in dependencies:
  p=Path(name)
  if not p.is_relative_to(root):continue
  out=saved/p.relative_to(root);out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,out)
 command=common+[str(scratch/'probe.o')]+list(map(str,links))+['-o',str(scratch/'probe')];commands.append(command);compiled=subprocess.run(command,text=True,capture_output=True);log+=compiled.stdout+compiled.stderr
 if compiled.returncode==0:
  run=subprocess.run([str(scratch/'probe'),str(asset)],text=True,capture_output=True,timeout=60);log+=run.stdout+run.stderr
after={str(p):digest(p) for p in inputs};receipt={'commands':commands,'compile':compiled.returncode,'run':None if run is None else run.returncode,'inputs_before':before,'inputs_after':after,'inputs_unchanged':before==after,'scope':'Pure CPU actual-GM source loader + exact GEOM timber/Input configuration. Does not prove native queue publication, GPU uploads, model child transforms or installed gameplay.'};(here/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');(here/'probe.log').write_text(log);print(log,flush=True);subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(scratch)],check=True)
assert compiled.returncode==0 and run and before==after
