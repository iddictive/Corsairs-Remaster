"""Capture actual GEOM terrain and replay its vertex/texture data on Metal.

Usage: bundled-python replay.py EMPTY_SCRATCH_DIR (from repository root).
Requires Pillow, the configured native build, runtime GM and TX files. No game UI.
"""
import pathlib,shlex,subprocess,sys
root=pathlib.Path.cwd(); native=root/'experiments/native-metal'; build=native/'.cache/build'
evidence=root/'docs/evidence/island-flicker-20261105'; work=pathlib.Path(sys.argv[1]).resolve()
assert work.is_dir() and not any(work.iterdir()), 'scratch directory must be empty'
ninja=native/'inputs/toolchain/bin/ninja'
commands=subprocess.check_output([str(ninja),'-C',str(build),'-t','commands','sea-fleet-benchmark'],text=True).splitlines()
a=shlex.split(next(x for x in commands if 'geom_static.cpp.o -c ' in x));a=a[:a.index('-MD')]+['-I'+str(p) for p in (native/'engine/src/libs').glob('*/include')]
source=work/'geom.cpp';source.write_bytes((evidence/'geom-capture.cpp.txt').read_bytes())
subprocess.run(a+['-c',str(source),'-o',str(work/'geom.o')],cwd=build,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
l=shlex.split(commands[-1].split(' && ')[1]);old='native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o';assert l.count(old)==1;l[l.index(old)]=str(work/'geom.o');l[l.index('-o')+1]=str(work/'geom');subprocess.run(l,cwd=build,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
subprocess.run([str(work/'geom'),str(native/'.cache/runtime'),str(work/'geom.bin')],cwd=build,check=True)
subprocess.run([sys.executable,str(evidence/'texture.py'),str(work)],cwd=root,check=True)
commands=subprocess.check_output([str(ninja),'-t','commands','gpu-skinning-offscreen-probe'],cwd=build,text=True).splitlines()
c=shlex.split(next(x for x in commands if ' -c ' in x and x.endswith('/gpu_skinning_offscreen_probe.cpp')));source=work/'material.cpp';source.write_bytes((evidence/'material-replay.cpp.txt').read_bytes());c[c.index('-o')+1]=str(work/'material.o');c[c.index('-c')+1]=str(source)
if '-MF' in c:c[c.index('-MF')+1]=str(work/'material.d')
subprocess.run(c,cwd=build,check=True)
l=shlex.split(commands[-1]);assert l[:2]==[':','&&'] and l[-2:]==['&&',':'];l=l[2:-2];old=next(x for x in l if x.endswith('/gpu_skinning_offscreen_probe.cpp.o'));l[l.index(old)]=str(work/'material.o');l[l.index('-o')+1]=str(work/'material');subprocess.run(l,cwd=build,check=True)
subprocess.run([str(work/'material'),str(work/'geom.bin')],check=True,timeout=60)
