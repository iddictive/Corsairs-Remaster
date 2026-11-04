from pathlib import Path
import os,re,subprocess,shutil,json,sys
EVIDENCE=Path(__file__).resolve().parent
repo=EVIDENCE.parents[2];work=Path(os.environ['CODEX_SCRATCH_DIR'])

p=repo/'docs/evidence/unarmed-boxing/replay.py'
s=p.read_text().split('inputs = [',1)[0]
s=s.replace("source = source[:source.index('    // Production collision, not a copied separation formula.')] + (HERE/'replay.cpp.inc').read_text()", "source = source[:source.index('    // Production collision, not a copied separation formula.')] + CROWD_SCENE.read_text()")
old='p.y=y0;\n        const CVECTOR a='
new='''p.y=y0;
        bool broad=true;
        for(float x : {-2.0f,2.0f}) for(float z : {-2.0f,2.0f}) {float gy=0; if(ptc.FindNode(p+CVECTOR(x,0,z),gy)<0 || std::fabs(gy-y0)>.02f)broad=false;}
        if(!broad) continue;
        const CVECTOR a='''
s=s.replace("(S/'fixture.cpp').write_text(source)","source=replace_once(source,"+repr(old)+","+repr(new)+")\n(S/'fixture.cpp').write_text(source)")
s=s.replace("(S/'RESOURCE').symlink_to(RES/'RESOURCE', target_is_directory=True)","None")
ns={'__file__':str(p),'__name__':'__main__','CROWD_SCENE':EVIDENCE/'crowd-native.cpp.inc'}
exec(compile(s,str(p),'exec'),ns)
resource=work/'RESOURCE';resource.mkdir()
for c in (ns['RES']/'RESOURCE').iterdir():
 if c.name.lower() not in ('animation','shared'): (resource/c.name).symlink_to(c,target_is_directory=c.is_dir())
(resource/'animation').mkdir()
for c in (ns['RES']/'RESOURCE/animation').iterdir():
 alt=repo/'experiments/native-metal/.cache/roundhouse/RESOURCE/animation'/c.name
 (resource/'animation'/c.name).symlink_to(alt if alt.exists() else c)
shutil.copytree(repo/'experiments/native-metal/engine/src/libs/shared_headers/include/shared',resource/'shared')
shutil.copytree(ns['RES']/'PROGRAM',work,dirs_exist_ok=True)
sys.path.insert(0,str(repo/'tools'));import gameplay_sources
sources,_=gameplay_sources.read()
for name,(data,_,_) in sources.items():
 if name.startswith('PROGRAM/'):
  path=work/Path(name).relative_to('PROGRAM');path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
for name,old,new in [('seadogs.c',b'void Main(',b'void GameMainNotExecuted('),('Loc_ai/LAi_utils.c',b'void LaunchBlood(',b'void ReplayUncalledLaunchBlood(')]:
 path=work/name;data=path.read_bytes();assert data.count(old)==1;path.write_bytes(data.replace(old,new,1))
consumer=(ns['BASE']/'native-contact-consumer.c.txt').read_text().replace('object Victim;','object Victim; object VictimLeft; object VictimRight;')
consumer=consumer.replace('if (attackType == "roundhouse" && attack.id == "replay-attacker" && enemy.id == "replay-victim") Hits++;','if (attack.id == "replay-attacker") { Hits++; enemy.ReplayHits=sti(enemy.ReplayHits)+1; }')
consumer=consumer.replace('    Hits = 0;', '''    Victim.ReplayHits=0;
    CopyAttributes(&VictimLeft,&Victim); VictimLeft.id="replay-left"; VictimLeft.index=2;
    CopyAttributes(&VictimRight,&Victim); VictimRight.id="replay-right"; VictimRight.index=3;
    Attacker.perks.list.RoundhouseJump=true;
    Hits = 0;''')
(work/'fixture.c').write_text(consumer)
for cmd in (ns['cc'],ns['link']):
 r=subprocess.run(cmd,cwd=ns['BUILD'],capture_output=True,text=True);(work/'compile.txt').open('a').write(r.stdout+r.stderr);assert not r.returncode,r.stderr[-2500:]
r=subprocess.run([str(work/'replay'),str(ns['RES']),'crowd','1'],cwd=work,env=ns['env'],capture_output=True,text=True,timeout=45)
(work/'result.json').write_text(json.dumps(dict(exit=r.returncode,stdout=r.stdout,stderr=r.stderr),indent=2));print(r.stdout,r.stderr);raise SystemExit(r.returncode)
