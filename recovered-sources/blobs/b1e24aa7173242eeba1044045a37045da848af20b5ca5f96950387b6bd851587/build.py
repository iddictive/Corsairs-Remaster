"""Build the private ordered backend against the frozen current native archives."""
from pathlib import Path
import argparse, difflib, hashlib, importlib.util, json, re, shlex, shutil, subprocess, sys

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];NATIVE=ROOT/'experiments/native-metal';BUILD=NATIVE/'.cache/build'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=HERE/'backend.mm')
parser.add_argument('--name',default='sea-fleet-ordered-batch')
parser.add_argument('--archive-source',default='4ee15fd')
args=parser.parse_args();source_path=args.source.resolve();OUT=source_path.parent
assert source_path.is_file() and source_path.is_relative_to(HERE.parent)
assert re.fullmatch('[a-z0-9-]+',args.name)
sys.path.insert(0,str(ROOT/'tools'))
from metal_storage import ensure_space
ensure_space(128*1024*1024,OUT)
spec=importlib.util.spec_from_file_location('existing_variant',HERE.parent/'hull-cache-crash-reproducer/prepare.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
ninja=(BUILD/'build.ninja').read_text();_,compile=helper.block(ninja,'CMakeFiles/storm_metal.dir/backend.mm.o')
argv=['/usr/bin/c++',*shlex.split(compile['DEFINES']),*shlex.split(compile['INCLUDES']),*shlex.split(compile['FLAGS']),'-I'+str(NATIVE),'-MMD','-MF',str(OUT/'backend.d'),'-o',str(OUT/'backend.mm.o'),'-c',str(source_path)]
_,link=helper.block(ninja,'bin/sea-fleet-benchmark')
driver=OUT/'driver.cpp.o'
# Every new candidate binds the driver to the same frozen header generation as
# the canonical archives. No old SHIP/CORE ABI is inferred from a filename.
changes=subprocess.check_output(['git','-C',str(ROOT),'diff','--name-only',args.archive_source,'--','experiments/native-metal'],text=True)
assert not changes.strip(),('native inputs differ from declared archive source',changes)
_,driver_compile=helper.block(ninja,'native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o')
driver_argv=['/usr/bin/c++',*shlex.split(driver_compile['DEFINES']),*shlex.split(driver_compile['INCLUDES']),*shlex.split(driver_compile['FLAGS']),'-MMD','-MF',str(OUT/'driver.d'),'-o',str(driver),'-c',str(NATIVE/'sea_fleet_benchmark.cpp')]
original_archive=BUILD/'libstorm_metal.a';archive=OUT/'libstorm_metal.a'
link_argv=['/usr/bin/c++',*shlex.split(link['FLAGS']),*shlex.split(link['LINK_FLAGS']),str(driver),'-o',str(OUT/args.name),*shlex.split(link.get('LINK_PATH','')),*shlex.split(link['LINK_LIBRARIES'])]
link_argv=[str(archive) if word in (str(original_archive),'libstorm_metal.a') else word for word in link_argv]
inputs={str(original_archive):sha(original_archive)}
for word in link_argv:
 for token in word.split(','):
  p=Path(token);p=p if p.is_absolute() else BUILD/p
  if p.is_file() and p.suffix in ('.a','.o','.dylib') and p not in (archive,driver):inputs[str(p)]=sha(p)
source={str(p):sha(p) for p in (source_path,NATIVE/'backend.mm',NATIVE/'resources.hpp',NATIVE/'land_shadow.hpp')}
if source_path==HERE/'backend.mm':
 for p in (HERE/'ordered_batch.hpp',HERE/'ordered_indices.hpp'):source[str(p)]=sha(p)
plan={'scope':'Declared coherent canonical archives and freshly compiled same-generation driver; private backend only. No gameplay source build or app delivery.','native_archive_source':args.archive_source,'cwd':str(BUILD),'compile':argv,'driver_compile':driver_argv,'link':link_argv,'frozen_link_inputs_sha256':inputs,'source_sha256':source}
(OUT/'plan.json').write_text(json.dumps(plan,indent=2)+'\n')
(OUT/'backend.patch').write_text(''.join(difflib.unified_diff((NATIVE/'backend.mm').read_text().splitlines(True),source_path.read_text().splitlines(True),fromfile='canonical/backend.mm',tofile='diagnostic/'+source_path.name)))
with (OUT/'build.log').open('w') as log:
 subprocess.run(driver_argv,cwd=BUILD,stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run(argv,cwd=BUILD,stdout=log,stderr=subprocess.STDOUT,check=True)
 members=subprocess.check_output(['/usr/bin/ar','-t',str(original_archive)],text=True).splitlines();assert members.count('backend.mm.o')==1
 shutil.copy2(original_archive,archive)
 for command in (['/usr/bin/ar','-d',str(archive),'backend.mm.o'],['/usr/bin/ar','-q',str(archive),str(OUT/'backend.mm.o')],['/usr/bin/ranlib',str(archive)],link_argv):subprocess.run(command,cwd=BUILD,stdout=log,stderr=subprocess.STDOUT,check=True)
for name,expected in inputs.items():assert sha(Path(name))==expected,name
dependencies=(OUT/'backend.d').read_text().replace('\\\n',' ').split(':',1)[1]
dependency_hashes={}
for token in shlex.split(dependencies):
 p=Path(token);p=p if p.is_absolute() else BUILD/p
 if p.is_file():dependency_hashes[str(p)]=sha(p)
driver_dependencies=(OUT/'driver.d').read_text().replace('\\\n',' ').split(':',1)[1]
driver_hashes={}
for token in shlex.split(driver_dependencies):
 p=Path(token);p=p if p.is_absolute() else BUILD/p
 if p.is_file():driver_hashes[str(p)]=sha(p)
plan.update(binary_sha256=sha(OUT/args.name),driver_object_sha256=sha(driver),driver_dependency_sha256=driver_hashes,backend_dependency_sha256=dependency_hashes,compile_and_link_exit=0)
(OUT/'build-receipt.json').write_text(json.dumps(plan,indent=2)+'\n')
print(json.dumps({'binary_sha256':plan['binary_sha256'],'dependencies':len(dependency_hashes),'frozen_link_inputs':len(inputs)}),flush=True)
