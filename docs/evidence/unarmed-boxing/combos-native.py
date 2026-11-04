import hashlib,json,os,re,shlex,shutil,subprocess,sys
from pathlib import Path
EVIDENCE=Path(__file__).resolve().parent
S=Path(os.environ['CODEX_SCRATCH_DIR'])
ROOT=EVIDENCE.parents[2]
BUILD=ROOT/'experiments/native-metal/.cache/build'
RES=Path(os.environ['CORSAIRS_APP']).expanduser().resolve()/'Contents/Resources'
BASE=ROOT/'docs/evidence/roundhouse-kick'
def replace(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new,1)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def prepare():
    (S/'run.py').write_bytes(Path(__file__).read_bytes())
    (S/'fixture-tail.cpp.inc').write_bytes((EVIDENCE/'combos-native.cpp.inc').read_bytes())
    headers=S/'fixture_headers';headers.mkdir(exist_ok=True)
    inputs={}
    for name,rel in {'character.h':'location/src/character.h','location.h':'location/src/location.h','model_array.h':'location/src/model_array.h','modelr.h':'model/src/modelr.h','ai_character.h':'location/src/ai_character.h','np_character.h':'location/src/np_character.h','player.h':'location/src/player.h'}.items():
        p=ROOT/'experiments/native-metal/engine/src/libs'/rel
        text=re.sub(r'^class (Character|Location|ModelArray|NODER|MODELR)(\b)',r'struct \1\2',p.read_text(),flags=re.M)
        (headers/name).write_text(text);inputs[str(p)]=sha(p)
    source=(BASE/'native-contact-replay.cpp.txt').read_text()
    seam='''struct ReplayControls final : CONTROLS { std::map<std::string,CONTROL_STATE> states;
    bool GetControlState(const char *name,CONTROL_STATE &out) override {out={};auto it=states.find(name);if(it!=states.end())out=it->second;return true;}
    };
    class ReplayBlade final : public Entity {public:bool Init() override{return true;} void ProcessStage(Stage,uint32_t) override{} uint64_t ProcessMessage(MESSAGE&) override{return 1;}};
    CREATE_CLASS(ReplayBlade)
    CREATE_CLASS(ReplayModel)'''
    source=replace(source,'CREATE_CLASS(ReplayModel)',seam)
    source=replace(source,'core_internal.Controls=new CONTROLS;','core_internal.Controls=new ReplayControls;')
    source=replace(source,'auto *m=model(); a->mdl=m->GetId();','auto *m=model(); a->mdl=m->GetId();a->blade=core.CreateEntity("ReplayBlade",nullptr);a->unarmedRig="man";a->characterID=strdup(name);')
    source=replace(source,'m->ani->Player(0).Pause(); m->ani->Player(0).SetPosition(0); m->ani->EvaluatePose();','m->ani->SetEventListener(&a->eventListener);a->eventId=m->ani->SetEvent(ae_end,0,&a->eventListener);m->ani->Player(0).Pause();m->ani->Player(0).SetPosition(0);m->ani->EvaluatePose();a->AttributeChanged(a->AttributesPointer->GetAttributeClass("actions"));')
    source=source[:source.index('    // Production collision, not a copied separation formula.')]+(S/'fixture-tail.cpp.inc').read_text()
    (S/'fixture.cpp').write_text(source)
    if not (S/'RESOURCE').exists():(S/'RESOURCE').symlink_to(RES/'RESOURCE',target_is_directory=True)
    (S/'userdata/Logs').mkdir(parents=True,exist_ok=True)
    shutil.copytree(RES/'PROGRAM',S,dirs_exist_ok=True)
    sys.path.insert(0,str(ROOT/'tools'));import gameplay_sources
    values,src_inputs=gameplay_sources.read()
    for name,(data,legacy,encoding) in values.items():
        if name.startswith('PROGRAM/'):
            p=S/Path(name).relative_to('PROGRAM');p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
    for p in src_inputs:inputs[str(p)]=sha(p)
    p=S/'seadogs.c';data=p.read_bytes();assert len(re.findall(rb'void\s+Main\s*\(',data))==1;p.write_bytes(re.sub(rb'void\s+Main\s*\(',b'void GameMainNotExecuted(',data,count=1))
    p=S/'Loc_ai/LAi_utils.c';data=p.read_bytes();assert len(re.findall(rb'void\s+LaunchBlood\s*\(',data))==1;p.write_bytes(re.sub(rb'void\s+LaunchBlood\s*\(',b'void ReplayUncalledLaunchBlood(',data,count=1))
    consumer=(BASE/'native-contact-consumer.c.txt').read_text().replace('attackType == "roundhouse" && ','')
    consumer=replace(consumer,'Hits = 0;\n    SetEventHandler','SetDefaultFight(&Attacker);SetDefaultFight(&Victim);\n    Hits = 0;\n    SetEventHandler')
    (S/'fixture.c').write_text(consumer)
    (S/'prepared-inputs.json').write_text(json.dumps(inputs,indent=2))
    print('PREPARED',S)
def run():
    prepared=json.loads((S/'prepared-inputs.json').read_text())
    for p,h in prepared.items():assert sha(Path(p))==h,'prepared source changed: '+p
    ninja=(BUILD/'build.ninja').read_text()
    def fields(pattern):
        m=re.search(pattern+r'.*?(?=\nbuild |\Z)',ninja,re.S);assert m,pattern
        return dict(re.findall(r'^  ([A-Z_]+) = (.*)$',m[0],re.M))
    f=fields(r'build native-engine/CMakeFiles/script-vm-probe\.dir/[^\n]+\.cpp\.o:');l=fields(r'build bin/script-vm-probe:')
    cc=['/usr/bin/c++']+sum(([x for x in shlex.split(f[k]) if x!='-v'] for k in ('DEFINES','FLAGS','INCLUDES')),[])
    cc+=['-I'+str(S)]+['-I'+str(ROOT/'experiments/native-metal/engine/src/libs'/x) for x in ('location/src','model/src','animation/src')]
    cc+=['-c',str(S/'fixture.cpp'),'-o',str(S/'fixture.o')]
    link=['/usr/bin/c++']+shlex.split(l['FLAGS'])+[x for x in shlex.split(l['LINK_FLAGS']) if x!='-v']+[str(S/'fixture.o'),'-o',str(S/'replay')]+shlex.split(l['LINK_LIBRARIES'])
    resources=[RES/'RESOURCE/animation'/name for name in ('man.an','man.ani','Unarmed_man.an','Unarmed_man.ani','Roundhouse_man.an','Roundhouse_man.ani')]
    resources += [RES/'RESOURCE/INI/roundhouse-rigs.ini',RES/'RESOURCE/MODELS/Locations/Outside/TownExitKSM/DuelField.gm',RES/'RESOURCE/MODELS/Locations/Outside/TownExitKSM/duelfield_patch.ptc']
    metadata={'inputs':prepared,'commands':[cc,link],'archives':{str(p):sha(p) for p in (BUILD/'lib').glob('*.a')},'resources':{str(p):sha(p) for p in resources},'fixtures':{str(S/n):sha(S/n) for n in ('run.py','fixture-tail.cpp.inc','fixture.cpp','fixture.c')},'effective_program':{str(p.relative_to(S)):sha(p) for p in S.rglob('*') if p.is_file() and p.suffix in ('.c','.h')},'compiler':subprocess.check_output(['/usr/bin/c++','--version'],text=True),'sdk':subprocess.check_output(['xcrun','--show-sdk-version'],text=True).strip(),'runs':[]};logs=[]
    def execute(cmd,cwd):
        r=subprocess.run(cmd,cwd=cwd,env=dict(os.environ,STORM_USERDATA=str(S/'userdata'),STORM_QUIET_CLIENT='1'),text=True,capture_output=True,timeout=100)
        logs.append('$ '+shlex.join(cmd)+'\n'+r.stdout+r.stderr);return r
    try:
        for cmd in (cc,link):
            r=execute(cmd,BUILD)
            if r.returncode:raise RuntimeError('compile/link failed '+str(r.returncode)+'\n'+r.stderr[-6000:])
        for case in sys.argv[2:] or ['jab-cross','double-jab','uppercut','high-victim','blocked','miss','swap','dialogue','death','guard-left','guard-right','guard-back','guard-armed']:
            r=execute([str(S/'replay'),str(RES),case,'1'],S)
            metadata['runs'].append({'case':case,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
            print(case,r.returncode,r.stdout[-1400:],r.stderr[-700:],flush=True)
            if r.returncode:raise RuntimeError('native replay failed: '+case)
    finally:
        (S/'result.json').write_text(json.dumps(metadata,indent=2));(S/'raw.txt').write_text('\n'.join(logs))
if sys.argv[1]=='prepare':prepare()
elif sys.argv[1]=='run':run()
