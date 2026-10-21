import subprocess,pathlib,json,hashlib,concurrent.futures,time
root=pathlib.Path('/REQUIRED_EXTERNAL_INPUT/Corsairs'); evidence=root/'.local-archives/metal-renderer-evidence/ship-cut-topology-20261016'
base=['clang++','-std=c++20','-UNDEBUG','-DSTORM_METAL_SHIP_HULL_DAMAGE','-Iexperiments/native-metal','-Iexperiments/native-metal/engine/src/libs/geometry/src','-Iexperiments/native-metal/engine/src/libs/math/include','-Iexperiments/native-metal/engine/src/libs/geometry/third_party/clipper2/include']
link=['experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.engine.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.triangulation.cpp']
jobs=[('bow',evidence/'bow.cpp',False),('prepare',evidence/'prepare.cpp',True),('kernel',evidence/'kernel.cpp',False),('mast',root/'.local-archives/metal-renderer-evidence/bounded-laser-20261016/mast.cpp',False),('actual-prepare',evidence/'actual-prepare.cpp',False)]
def run(job):
 name,source,sanitize=job;binary=evidence/(name+'-final');options=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else ['-O2'];command=base+options+[str(source)]+link+['-o',str(binary)]
 compile=subprocess.run(command,cwd=root,text=True,capture_output=True);(evidence/(name+'-final-compile.log')).write_text(compile.stdout+compile.stderr)
 if compile.returncode:return dict(name=name,compile=compile.returncode)
 started=time.monotonic();result=subprocess.run([str(binary)],cwd=root,text=True,capture_output=True);(evidence/(name+'-final-run.log')).write_text(result.stdout+result.stderr);print(name,result.returncode,result.stdout,flush=True);return dict(name=name,compile=0,result=result.returncode,elapsed=time.monotonic()-started,command=command,stdout=result.stdout,stderr=result.stderr)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(run,jobs))
paths=['experiments/native-metal/ship_laser_geometry.hpp','experiments/native-metal/engine/src/libs/geometry/src/geom_static.cpp','experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp']
(evidence/'final-checks.json').write_text(json.dumps(dict(sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},checks=results),indent=2));raise SystemExit(any(x.get('compile') or x.get('result') for x in results))
