"""Link only the diagnostic backend against the existing frozen fleet inputs."""
from pathlib import Path
import argparse, difflib, hashlib, importlib.util, json, shlex, shutil, subprocess, sys

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];NATIVE=ROOT/'experiments/native-metal';BUILD=NATIVE/'.cache/build'
sys.path.insert(0,str(ROOT/'tools'))
from metal_storage import ensure_space
spec=importlib.util.spec_from_file_location('existing_variant',HERE.parent/'hull-cache-crash-reproducer/prepare.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build',action='store_true');args=parser.parse_args()
ensure_space(128*1024*1024,HERE)
ninja=(BUILD/'build.ninja').read_text();_,compile=helper.block(ninja,'CMakeFiles/storm_metal.dir/backend.mm.o')
object_path=HERE/'backend.mm.o'
argv=['/usr/bin/c++',*shlex.split(compile['DEFINES']),*shlex.split(compile['INCLUDES']),*shlex.split(compile['FLAGS']),'-MMD','-MF',str(HERE/'backend.d'),'-o',str(object_path),'-c',str(HERE/'backend.mm')]
_,link=helper.block(ninja,'bin/sea-fleet-benchmark')
driver=BUILD/'native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
link_argv=['/usr/bin/c++',*shlex.split(link['FLAGS']),*shlex.split(link['LINK_FLAGS']),str(driver),'-o',str(HERE/'sea-fleet-reflection-lod'),*shlex.split(link.get('LINK_PATH','')),*shlex.split(link['LINK_LIBRARIES'])]
original_archive=BUILD/'libstorm_metal.a';archive=HERE/'libstorm_metal.a'
link_argv=[str(archive) if word in (str(original_archive),'libstorm_metal.a') else word for word in link_argv]
inputs={str(driver):sha(driver),str(original_archive):sha(original_archive)}
for word in link_argv:
 for token in word.split(','):
  p=Path(token);p=p if p.is_absolute() else BUILD/p
  if p.is_file() and p.suffix in ('.a','.o','.dylib') and p!=archive:inputs[str(p)]=sha(p)
source={str(p):sha(p) for p in (HERE/'backend.mm',HERE/'reflection_lod.hpp',HERE/'indices.bin',NATIVE/'backend.mm')}
plan={'scope':'Frozen fleet libraries and original driver, diagnostic backend only; current dirty gameplay sources are not compiled.', 'cwd':str(BUILD),'compile':argv,'link':link_argv,'frozen_link_inputs_sha256':inputs,'source_sha256':source}
(HERE/'plan.json').write_text(json.dumps(plan,indent=2)+'\n')
(HERE/'backend.patch').write_text(''.join(difflib.unified_diff((NATIVE/'backend.mm').read_text().splitlines(True),(HERE/'backend.mm').read_text().splitlines(True),fromfile='canonical/backend.mm',tofile='diagnostic/backend.mm')))
if args.build:
 with (HERE/'build.log').open('w') as log:
  subprocess.run(argv,cwd=BUILD,stdout=log,stderr=subprocess.STDOUT,check=True)
  members=subprocess.check_output(['/usr/bin/ar','-t',str(original_archive)],text=True).splitlines();assert members.count('backend.mm.o')==1
  shutil.copy2(original_archive,archive)
  for command in (['/usr/bin/ar','-d',str(archive),'backend.mm.o'],['/usr/bin/ar','-q',str(archive),str(object_path)],['/usr/bin/ranlib',str(archive)],link_argv):subprocess.run(command,cwd=BUILD,stdout=log,stderr=subprocess.STDOUT,check=True)
 for name,expected in inputs.items():assert sha(Path(name))==expected,name
 deps=(HERE/'backend.d').read_text().replace('\\\n',' ').split(':',1)[1]
 dependency_hashes={}
 for token in shlex.split(deps):
  p=Path(token);p=p if p.is_absolute() else BUILD/p
  if p.is_file():dependency_hashes[str(p)]=sha(p)
 plan.update(binary_sha256=sha(HERE/'sea-fleet-reflection-lod'),backend_dependency_sha256=dependency_hashes,compile_and_link_exit=0)
 (HERE/'build-receipt.json').write_text(json.dumps(plan,indent=2)+'\n')
 print(json.dumps({'binary_sha256':plan['binary_sha256'],'dependencies':len(dependency_hashes),'frozen_link_inputs':len(inputs)}),flush=True)
else:print(HERE/'plan.json')
