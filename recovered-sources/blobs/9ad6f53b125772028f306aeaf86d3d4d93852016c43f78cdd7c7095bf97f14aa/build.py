import shlex,subprocess,json,hashlib,sys
from pathlib import Path
root=Path.cwd();build=root/'experiments/native-metal/.cache/build';dst=Path(__file__).resolve().parent
assert len(sys.argv)>2, 'pass a task-owned temporary output directory'
outputs=Path(sys.argv[2]).resolve();assert outputs.is_dir()
ninja=root/'experiments/native-metal/inputs/toolchain/bin/ninja'
lines=subprocess.check_output([str(ninja),'-C',str(build),'-t','commands','sea-fleet-benchmark'],text=True).splitlines()
c=[x for x in lines if ' -c ' in x and '/sea_fleet_benchmark.cpp' in x];l=[x for x in lines if ' -o bin/sea-fleet-benchmark ' in x];assert len(c)==len(l)==1
compile=shlex.split(c[0]);source=root/'experiments/native-metal/sea_fleet_benchmark.cpp';obj='native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
for flag in ['-MD','-MT','-MF']:
 if flag in compile:
  at=compile.index(flag)
  if flag=='-MD':compile.pop(at)
  else:del compile[at:at+2]
assert compile.count(str(source))==1 and compile.count(obj)==1
compile[compile.index(str(source))]=str(dst/'probe.cpp');compile[compile.index(obj)]=str(outputs/'world.o')
if '-v' in compile:compile.remove('-v')
compile+=['-UNDEBUG']
link=shlex.split(l[0]);assert link[:3]==[':','&&',compile[0]] and link[-2:]==['&&',':']
link=link[2:-2];assert link.count(obj)==1;link[link.index(obj)]=str(outputs/'world.o')
name=sys.argv[1] if len(sys.argv)>1 else 'baseline'
link[link.index('bin/sea-fleet-benchmark')]=str(outputs/name)
if '-v' in link:link.remove('-v')
(dst/(name+'-commands.json')).write_text(json.dumps({'compile':compile,'link':link,'libraries':{str(p.relative_to(build)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (build/'lib').glob('*.a')}},indent=2)+'\n')
for title,cmd in [('compile',compile),('link',link)]:
 with (dst/(title+'-'+name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=build,stdout=log,stderr=subprocess.STDOUT)
 print(title,p.returncode)
 if p.returncode:print((dst/(title+'-'+name+'.log')).read_text()[-4000:]);raise SystemExit(p.returncode)
q=dst/'RESOURCE'
if not q.exists():q.symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE',target_is_directory=True)
print('private native fixture ready',name)
