from pathlib import Path
import os, subprocess, shutil, re, sys, json, time
EVIDENCE=Path(__file__).resolve().parent
p=EVIDENCE.parent/'unarmed-boxing/replay.py';s=p.read_text().split('inputs = [',1)[0]
s=s.replace("source = source[:source.index('    // Production collision, not a copied separation formula.')] + (HERE/'replay.cpp.inc').read_text()", "source = source[:source.index('    // Production collision, not a copied separation formula.')] + FALL_FIXTURE.read_text()")
s=s.replace("(S/'RESOURCE').symlink_to(RES/'RESOURCE', target_is_directory=True)","None")
ns={'__file__':str(p),'__name__':'__main__','FALL_FIXTURE':EVIDENCE/'replay.cpp.inc'};exec(compile(s,str(p),'exec'),ns);S=ns['S'];ROOT=ns['ROOT'];RES=ns['RES']
resource=S/'RESOURCE';resource.mkdir()
for c in (RES/'RESOURCE').iterdir():
 if c.name.lower() not in ('animation','shared'): (resource/c.name).symlink_to(c,target_is_directory=c.is_dir())
(resource/'animation').mkdir()
for c in (RES/'RESOURCE/animation').iterdir():
 alt=ROOT/'experiments/native-metal/.cache/roundhouse/RESOURCE/animation'/c.name
 (resource/'animation'/c.name).symlink_to(alt if alt.exists() else c)
shutil.copytree(ROOT/'experiments/native-metal/engine/src/libs/shared_headers/include/shared',resource/'shared')
shutil.copytree(RES/'PROGRAM',S,dirs_exist_ok=True)
sys.path.insert(0,str(ROOT/'tools'));import gameplay_sources
sources,_=gameplay_sources.read()
for name,(data,_,_) in sources.items():
 if name.startswith('PROGRAM/'):
  target=S/Path(name).relative_to('PROGRAM');target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
for name,old,new in [('seadogs.c',b'void Main(',b'void GameMainNotExecuted('),('Loc_ai/LAi_utils.c',b'void LaunchBlood(',b'void ReplayUncalledLaunchBlood(')]:
 t=S/name;data=t.read_bytes();assert data.count(old)==1;t.write_bytes(data.replace(old,new,1))
consumer=(ns['BASE']/'native-contact-consumer.c.txt').read_text();(S/'fixture.c').write_text(consumer)
for cmd in (ns['cc'],ns['link']):
 r=subprocess.run(cmd,cwd=ns['BUILD'],capture_output=True,text=True);(S/'compile.txt').open('a').write(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:]
results=[]
for case in ('deep','floor','immortal','void','shallow'):
 start=time.perf_counter();r=subprocess.run([str(S/'replay'),str(RES),case,'1'],cwd=S,env=ns['env'],text=True,capture_output=True,timeout=30)
 results.append(dict(case=case,exit=r.returncode,seconds=time.perf_counter()-start,stdout=r.stdout,stderr=r.stderr));print(case,r.returncode,r.stdout[-2500:],r.stderr[-1000:]);(S/'result.json').write_text(json.dumps(results,indent=2));assert r.returncode==0
