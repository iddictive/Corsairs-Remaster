from pathlib import Path
import subprocess,shlex,re,json,os
S=Path(os.environ['CODEX_SCRATCH_DIR'])
ROOT=Path(__file__).resolve().parents[3];BUILD=ROOT/'experiments/native-metal/.cache/build'
source=(S/'fixture.cpp').read_text()
consumer=S/'fixture.c'
text=consumer.read_text()
assert 'ReplayFatalWound' not in text
consumer.write_text(text+'\n#event_handler("ReplayFatalWound","ReplayFatalWound")\nvoid ReplayFatalWound(){Victim.chr_ai.hp=0.0; LAi_RecordWeaponWound(&Attacker,&Victim,500.0,1);}\n')
old='class ReplayModel final : public MODELR { public: bool Init() override { return true; } };'
new='''class ReplayModel final : public MODELR { public:
 bool part=false; CMatrix partWorld; int neck=-1;
 bool Init() override { return true; }
 bool ApplyCombatDamage(const CVECTOR&,const CVECTOR&,int kind,bool fatal,int joint)override {
  neck=joint;part=fatal&&kind==1;partWorld=mtx;partWorld.Pos().y+=1.6f;return true;
 }
 bool GetCombatDetachedPart(CMatrix &world,float &radius)const override {
  if(!part)return false;world=partWorld;radius=.12f;return true;
 }
 void SetCombatDetachedTransform(const CMatrix &world)override {partWorld=world;}
};'''
assert source.count(old)==1;source=source.replace(old,new)
at=source.index('    b->curPos=b->oldPos=initial+')
source=source[:at]+'''
    require(!b->ApplyCombatWeaponImpact(a->GetId(),3,true),"invalid weapon kind rejected");
    core.Event("ReplayFatalWound");
    auto *part=static_cast<ReplayModel *>(b->Model());
    require(part->neck>=0,"authored neck mapping reaches model API");
    auto *world=location->ExistingRagdollWorld();
    require(world&&world->Size()==1&&b->roundhouseState->detachedHandle,"one native detached body");
    const CVECTOR start=part->partWorld.Pos();
    require(b->ApplyCombatWeaponImpact(a->GetId(),1,true),"repeated impact admitted without duplicate body");
    require(world->Size()==1,"body deduplicated");
    for(int i=0;i<90;++i){location->StepRagdolls(1.f/60);b->UpdateCombatDetached();}
    const float displacement=std::sqrt(~(part->partWorld.Pos()-start));
    require(displacement>.01f,"actual Jolt pose published through native model API");
    b->CancelRoundhouse();
    require(world->Size()==0&&!b->roundhouseState->detachedHandle,"normal native cleanup removes detached physics");
    std::printf("detached_lifecycle=PASS displacement=%g neck=%d no_window=%d\\n",displacement,part->neck,core.GetWindow()==nullptr);
    std::fflush(stdout);std::_Exit(0);
}catch(const std::exception &e){std::fprintf(stderr,"FAIL %s\\n",e.what());return 2;}
'''
(S/'lifecycle.cpp').write_text(source)
ninja=(BUILD/'build.ninja').read_text()
def fields(pattern):
 m=re.search(pattern+r'.*?(?=\nbuild |\Z)',ninja,re.S);assert m
 return dict(re.findall(r'^  ([A-Z_]+) = (.*)$',m[0],re.M))
f=fields(r'build native-engine/CMakeFiles/script-vm-probe\.dir/[^\n]+\.cpp\.o:');l=fields(r'build bin/script-vm-probe:')
cc=['/usr/bin/c++']+sum(([v for v in shlex.split(f[k]) if v!='-v'] for k in ('DEFINES','FLAGS','INCLUDES')),[])
cc+=['-I'+str(S)]+['-I'+str(ROOT/'experiments/native-metal/engine/src/libs'/x) for x in ('location/src','model/src','animation/src')]
cc+=['-c',str(S/'lifecycle.cpp'),'-o',str(S/'lifecycle.o')]
link=['/usr/bin/c++']+shlex.split(l['FLAGS'])+[v for v in shlex.split(l['LINK_FLAGS']) if v!='-v']+[str(S/'lifecycle.o'),'-o',str(S/'lifecycle')]+shlex.split(l['LINK_LIBRARIES'])
for cmd in (cc,link):
 r=subprocess.run(cmd,cwd=BUILD,text=True,capture_output=True);assert not r.returncode,r.stderr[-2500:]
r=subprocess.run([str(S/'lifecycle'),str(Path(os.environ['CORSAIRS_APP']).expanduser().resolve()/'Contents/Resources'),'lifecycle','1'],cwd=S,text=True,capture_output=True,timeout=45)
(S/'lifecycle-result.json').write_text(json.dumps(dict(exit=r.returncode,stdout=r.stdout,stderr=r.stderr),indent=2));print(r.stdout,r.stderr);raise SystemExit(r.returncode)
