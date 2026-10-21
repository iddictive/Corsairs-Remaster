#!/usr/bin/env python3
"""Prepare an engine-linked destruction scene packet; never build or run it."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import types

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
NATIVE=ROOT/'experiments/native-metal'
BUILD=NATIVE/'.cache/build'
APP=Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources')

def once(text,before,after):
    assert text.count(before)==1,(before,text.count(before))
    return text.replace(before,after)

def block(ninja,needle):
    found=[m for m in re.finditer(r'^build ([^\n]+)\n((?:  [^\n]*\n)*)',ninja,re.M)
           if needle in m[1].split(':',1)[0]]
    assert len(found)==1,(needle,len(found))
    return dict(re.findall(r'^  ([A-Z_]+) = (.*)$',found[0][2],re.M))

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def git_bytes(ref,path):
    relative=path.relative_to(ROOT)
    return subprocess.check_output(['git','show',f'{ref}:{relative}'],cwd=ROOT)

def snapshot_headers(ref,out,flags,driver):
    """Resolve the tracked header closure without invoking a compiler.

    Preserve repo-relative paths so quoted relative includes stay frozen.
    Source include roots are replaced, never backed by mutable live headers.
    Cached vendor/compatibility headers are copied into the same closure;
    compiler/SDK system headers remain with the installed toolchain.
    """
    tree=subprocess.check_output(['git','ls-tree','-r','-z',ref,'--','experiments/native-metal'],cwd=ROOT)
    objects={}
    for record in tree.split(b'\0'):
        if not record:continue
        metadata,name=record.split(b'\t',1)
        if name.decode().endswith(('.h','.hpp','.inl','.inc','.rh')):
            objects[name.decode()]=metadata.split()[2].decode()
    oids=list(dict.fromkeys(objects.values()))
    result=subprocess.run(['git','cat-file','--batch'],cwd=ROOT,
        input=('\n'.join(oids)+'\n').encode(),stdout=subprocess.PIPE,check=True).stdout
    blobs={};cursor=0
    for oid in oids:
        end=result.index(b'\n',cursor)
        actual,kind,size=result[cursor:end].split()
        assert actual.decode()==oid and kind==b'blob'
        cursor=end+1;blobs[oid]=result[cursor:cursor+int(size)]
        cursor+=int(size)+1
    snapshot=out/'include-snapshot'
    include_dirs=[Path(os.path.normpath(flag[2:])) for flag in flags if flag.startswith('-I')]
    include_dirs += [Path(os.path.normpath(flags[i+1])) for i,flag in enumerate(flags) if flag=='-isystem']
    def project_path(path):
        try:return str(Path(os.path.normpath(path)).relative_to(ROOT))
        except ValueError:return None
    def frozen_root(path):
        relative=project_path(path)
        return relative and (relative=='experiments/native-metal' or relative.startswith('experiments/native-metal/'))
    found={};external={};unresolved=set();queue=[]
    def resolve(name,parent,quoted):
        locations=([parent/name] if quoted else [])+[directory/name for directory in include_dirs]
        for candidate in locations:
            relative=project_path(candidate)
            if relative in objects:
                if relative not in found:
                    found[relative]=blobs[objects[relative]];queue.append((ROOT/relative,found[relative]))
                return
            if candidate.is_file():
                if frozen_root(candidate.parent):
                    # Only cached vendor/compatibility/generated headers may
                    # lack a Git blob. Never admit a new live engine header.
                    assert relative and relative.startswith(('experiments/native-metal/.cache/build/',
                        'experiments/native-metal/inputs/')),candidate
                    if relative not in found:
                        found[relative]=candidate.read_bytes();queue.append((ROOT/relative,found[relative]))
                else:external[str(candidate.resolve())]=digest(candidate)
                return
        unresolved.add(name)
    queue=[(out/'driver.cpp',driver.encode()),(HERE/'scene_exercise.hpp',(HERE/'scene_exercise.hpp').read_bytes())]
    # Freeze SHIP's concrete layout as an explicit ABI witness even though the
    # driver calls its public SHIP_BASE interface and never allocates SHIP.
    for relative in ('experiments/native-metal/engine/src/libs/ship/src/ship.h',):
        assert relative in objects
        found[relative]=blobs[objects[relative]];queue.append((ROOT/relative,found[relative]))
    while queue:
        path,content=queue.pop()
        for opening,name in re.findall(rb'^\s*#\s*include\s*(["<])([^">\n]+)[">]',content,re.M):
            resolve(name.decode(),path.parent,opening==b'"')
    for relative,content in found.items():
        destination=snapshot/relative;destination.parent.mkdir(parents=True,exist_ok=True)
        destination.write_bytes(content)
    rewritten=[]
    for i,flag in enumerate(flags):
        if flag.startswith('-I') and frozen_root(Path(os.path.normpath(flag[2:]))):
            rewritten.append('-I'+str(snapshot/project_path(Path(os.path.normpath(flag[2:])))))
        elif i>0 and flags[i-1]=='-isystem' and frozen_root(Path(os.path.normpath(flag))):
            rewritten.append(str(snapshot/project_path(Path(os.path.normpath(flag)))))
        else:rewritten.append(flag)
    return rewritten,{
        'ref':ref,'root':str(snapshot),
        'files':{str(snapshot/name):hashlib.sha256(content).hexdigest() for name,content in found.items()},
        'git_blobs':{name:objects[name] for name in found if name in objects},
        'installed_dependency_sha256':external,
        'unresolved_system_or_inactive_includes':sorted(unresolved),
        'resolution':'Static superset of quoted/angle includes; source roots replaced by snapshot-only -I. No current engine header fallback. Compiler ownership remains FPS root.'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag',default='baseline-source')
    parser.add_argument('--base-ref',default='36cfbfee46a602030dfdee64ae28c5a6eb6071ef')
    args=parser.parse_args()
    assert re.fullmatch(r'[a-z0-9-]+',args.tag)
    out=HERE/args.tag
    out.mkdir(exist_ok=True)
    base_ref=subprocess.check_output(['git','rev-parse',args.base_ref+'^{commit}'],cwd=ROOT,text=True).strip()
    archive_record=HERE/'baseline-archive-inputs.json'
    if not archive_record.is_file():
        previous=json.loads((HERE/'baseline-source/plan.json').read_text())
        assert previous['base_head']==base_ref
        archive_record.write_text(json.dumps({'base_head':base_ref,'linked_input_sha256':previous['linked_input_sha256']},indent=2)+'\n')
    original_archives=json.loads(archive_record.read_text())
    assert original_archives['base_head']==base_ref
    for name,expected in original_archives['linked_input_sha256'].items():
        assert digest(Path(name))==expected,('cached baseline archive changed; do not relabel it',name)
    # Use canonical text extraction; no copied PROGRAM/RESOURCE runtime.
    import sys
    sys.path.insert(0,str(ROOT/'tools'))
    runner_source=git_bytes(base_ref,ROOT/'tools/run_sea_benchmark.py').decode()
    runner_module=types.ModuleType('scene_runner')
    runner_module.__file__=str(ROOT/'tools/run_sea_benchmark.py')
    exec(compile(runner_source,runner_module.__file__,'exec'),runner_module.__dict__)
    source_paths={
        'ship-catalog':APP/'PROGRAM/ships/ships_init.c',
        'ship-lights':APP/'PROGRAM/SEA_AI/ShipLights.c',
        'day':APP/'PROGRAM/Weather/Init/Day.c',
        'units':APP/'PROGRAM/utils.c',
        'native-declarations':APP/'PROGRAM/seadogs.c',
        'cannon-declarations':APP/'PROGRAM/cannons/cannons.h',
        'texture-selection':APP/'PROGRAM/SEA_AI/AIShip.c',
        'fixture':NATIVE/'sea_fleet_fixture.c'}
    texts={name:runner_module.read(path) for name,path in source_paths.items() if name!='fixture'}
    texts['fixture']=git_bytes(base_ref,NATIVE/'sea_fleet_fixture.c').decode()
    catalog=texts['ship-catalog']
    marker='makeref(refShip,ShipsTypes[SHIP_FLYINGDUTCHMAN]);'
    assert catalog.count(marker)==1
    start=catalog.index(marker)+len(marker)
    end=catalog.index('makeref(refShip,ShipsTypes[',start)
    ship=re.sub(r'\brefship\b','ShipType',catalog[start:end],flags=re.I)
    cannon=re.findall(r'^#define CANNON_TYPE_CANNON_LBS36\s+[0-9]+',texts['cannon-declarations'],re.M)
    assert len(cannon)==1
    # Default catalogue borts are shared by every stock ship; preserve their
    # exact values instead of inventing a diagnostic firing cone.
    first='ShipsTypes[idx].Cannons.Borts.cannonl = "";'
    last='cannonl.FireAngMax\t= 0.60;'
    assert catalog.count(first)==catalog.count(last)==1
    borts=catalog[catalog.index(first):catalog.index(last)+len(last)]
    assert borts.count('ShipsTypes[idx]')==8
    borts=borts.replace('ShipsTypes[idx]','ShipType')
    assert len(re.findall(r'Degree2Radian\(([0-9.]+)\)',borts))==8
    ship_type=cannon[0]+'\n'+runner_module.function(texts['units'],'int Tonnes2CWT(int iTonnes)')
    ship_type+='void InitShipType(){\naref cannonl,cannonr,cannonf,cannonb;\n'+borts+'\n'+ship
    ship_type+='\nShipType.Name="Flyingdutchman1";\nShipType.ship.Upgrades.hull=1;\n}\n'
    declarations=re.findall(r'^native [^\n]+;',texts['native-declarations'],re.M)
    assert len(declarations)==11
    fixture=once(texts['fixture'],'int FleetCount = 8;','int FleetCount = 2;')
    fixture=once(fixture,'#include "native-declarations.c"','\n'.join(declarations))
    fixture=once(fixture,'object Seafoam;','object Seafoam;\nobject SceneEvents;')
    fixture=once(fixture,'    InitShipType();','''    InitShipType();
    SetEventHandler("Ship_StartLoad","Ship_StartLoad",0);
    SetEventHandler("Ship_EndLoad","Ship_EndLoad",0);
    SetEventHandler("Shp_GetCurBallsNum","SceneBalls",0);
    SetEventHandler("CannLoad","SceneLoad",0);
    SetEventHandler("CannGRTime","SceneRecharge",0);
    SetEventHandler("SeaPhonk_HullCut","SceneHullCut",0);
    SetEventHandler("SeaPhonk_Flood","SceneFlood",0);''')
    # Native AIShip::CreateShip now owns Mount and cannon controller admission.
    first='        SetTexturePath(0, "ships\\\\" + ShipType.Name + "\\\\hull1\\\\");'
    last='        SendMessage(&Sailors, "lis", 51001, &Fleet[i], "Manowar1");'
    assert fixture.count(first)==fixture.count(last)==1
    start=fixture.index(first);end=fixture.index(last)+len(last)
    fixture=fixture[:start]+'        // Mount and crew construction follow the first CORE frame in the native driver.'+fixture[end:]
    fixture=once(fixture,'        Fleet[i].ship.HP = 10000;','''        Fleet[i].ship.HP = ShipType.HP;
        Fleet[i].ship.Cannons.SpeedV0 = 300;
        Fleet[i].ship.Cannons.Borts = "";''')
    fixture=once(fixture,'        Fleet[i].ship.Crew.Quantity = 830;','        Fleet[i].ship.Crew.Quantity = ShipType.MaxCrew;')
    fixture=once(fixture,'        Fleet[i].ship.speed.z = 4.0;','        Fleet[i].ship.speed.z = 0.0;')
    fixture+='''
// Same path selection as installed Ship_StartLoad -> SetTextureForShip for
// this exact known real-ship name/hull; no lookup of player RealShips/SAVE.
void Ship_StartLoad(){SetTexturePath(0,"ships\\\\"+ShipType.Name+"\\\\hull1\\\\");}
void Ship_EndLoad(){SetTexturePath(0,"");}
int SceneBalls(){return 1000;}
int SceneLoad(){return 1;}
float SceneRecharge(){return 20.0;}
int SceneHullCut(){
    aref character=GetEventData();
    int owner=GetEventData();
    float removed=GetEventData();
    int structural=GetEventData();
    int partial=GetEventData();
    SceneEvents.calls=sti(SceneEvents.calls)+1;
    SceneEvents.owner=owner;
    SceneEvents.removed=removed;
    SceneEvents.structural=structural;
    SceneEvents.partial=partial;
    return 0; // No game death/quest handler; never claim gameplay acceptance.
}
void SceneFlood(){
    aref character=GetEventData();
    int owner=GetEventData();
    float fraction=GetEventData();
    SceneEvents.floodFraction=fraction;
}
'''
    fixture=once(fixture,'    InitShipType();','    SceneEvents.calls=0;\n    InitShipType();')
    # sti is not a native function in this minimal fixture.
    fixture=once(fixture,'sti(SceneEvents.calls)','makeint(SceneEvents.calls)')
    (out/'fixture.c').write_text(fixture)
    (out/'ship-type.c').write_text(ship_type)
    (out/'ship-lights.c').write_text(runner_module.function(texts['ship-lights'],'void InitShipLights()'))
    (out/'day.c').write_text(texts['day'])
    driver=git_bytes(base_ref,NATIVE/'sea_fleet_benchmark.cpp').decode()
    driver=once(driver,'class SeaBenchmarkLibrary','#include "scene_exercise.hpp"\n\nclass SeaBenchmarkLibrary')
    driver=once(driver,'    core_internal.SetDeltaTime(16);','    core_internal.SetDeltaTime(20);')
    start='        if (frame) {\n'
    end='        const auto begin = std::chrono::steady_clock::now();'
    assert driver.count(start)==driver.count(end)==1
    begin=driver.index(start);finish=driver.index(end,begin)
    driver=driver[:begin]+'        destruction_scene::beforeFrame(frame);\n'+driver[finish:]
    driver=once(driver,'            std::srand(1337);','            std::srand(1337);\n            destruction_scene::initialize();')
    driver=once(driver,'        const auto end = std::chrono::steady_clock::now();','''        destruction_scene::afterFrame(frame);
        destruction_scene::capture(frame,width,height);
        const auto end = std::chrono::steady_clock::now();''')
    driver=once(driver,'    core_internal.EraseEntities();','    destruction_scene::finish();\n    core_internal.EraseEntities();')
    (out/'driver.cpp').write_text(driver)
    ninja=(BUILD/'build.ninja').read_text()
    compile_values=block(ninja,'native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o')
    flags=[*shlex.split(compile_values['DEFINES']),*shlex.split(compile_values['INCLUDES']),*shlex.split(compile_values['FLAGS'])]
    flags+=['-I'+str(HERE),'-I'+str(NATIVE/'engine/src/libs/sea_ai/src')]
    current_header_flags=list(flags)
    flags,header_closure=snapshot_headers(base_ref,out,flags,driver)
    compile_command=['/usr/bin/c++',*flags,'-o',str(out/'driver.o'),'-c',str(out/'driver.cpp')]
    link=block(ninja,'bin/sea-fleet-benchmark')
    libraries=shlex.split(link['LINK_LIBRARIES'])
    link_command=['/usr/bin/c++',*shlex.split(link['FLAGS']),*shlex.split(link['LINK_FLAGS']),str(out/'driver.o'),'-o',str(out/'destruction-benchmark'),*shlex.split(link.get('LINK_PATH','')),*libraries]
    runner=runner_source
    runner=once(runner,'    import metal_engine_source\n','')
    runner=once(runner,'    source_identity = metal_engine_source.fingerprint()',
        '    source_identity = json.loads(Path('+repr(str(out/'plan.json'))+').read_text())["compiled_source_identity"]')
    runner=once(runner,'from metal_storage import diagnostic_run, ensure_build_size, ensure_space, maintain',
        'import sys\nsys.path.insert(0,'+repr(str(ROOT/'tools'))+')\nfrom metal_storage import diagnostic_run, ensure_build_size, ensure_space, maintain')
    runner=once(runner,'ROOT = Path(__file__).resolve().parents[1]','ROOT = Path('+repr(str(ROOT))+')')
    runner=once(runner,'binary = NATIVE / ".cache/build/bin/sea-fleet-benchmark"','binary = Path('+repr(str(out/'destruction-benchmark'))+')')
    begin='    with (NATIVE / ".cache/diagnostics/sea-fleet-build.log").open("w") as build_log:'
    end='    if metal_engine_source.fingerprint() != source_identity:'
    assert runner.count(begin)==runner.count(end)==1
    start=runner.index(begin);finish=runner.index(end,start)
    runner=runner[:start]+'''    plan=json.loads(Path('''+repr(str(out/'plan.json'))+''').read_text())
    for name,expected in {**plan['input_sha256'],**plan['linked_input_sha256'],**plan['header_closure']['files'],**plan['header_closure']['installed_dependency_sha256']}.items():
        if hashlib.sha256(Path(name).read_bytes()).hexdigest()!=expected:
            raise RuntimeError('scene packet input changed: '+name)
'''+runner[finish:]
    runner=once(runner,'    if metal_engine_source.fingerprint() != source_identity:\n        raise RuntimeError("native inputs changed during benchmark build")\n','')
    runner=once(runner,'"native_source_sha256": source_identity,','"compiled_source_identity": source_identity, "baseline_ref": '+repr(base_ref)+',')
    runner=once(runner,'model = resources / "RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm"','model = resources / "RESOURCE/MODELS/Ships/Flyingdutchman1/flyingdutchman1.gm"')
    begin='    marker = "makeref(refShip,ShipsTypes[SHIP_MANOWAR]);"'
    end='    declarations = re.findall('
    assert runner.count(begin)==runner.count(end)==1
    start=runner.index(begin);finish=runner.index(end,start)
    runner=runner[:start]+"    ship=Path("+repr(str(out/'ship-type.c'))+").read_text()\n"+runner[finish:]
    runner=once(runner,'required Manowar1 asset absent','required Flyingdutchman1 asset absent')
    runner=once(runner,'"fixture": NATIVE / "sea_fleet_fixture.c",','"fixture": Path('+repr(str(out/'fixture.c'))+'),')
    # Prepared fixture already has native declarations and exact two-ship setup.
    begin='    if texts["fixture"].count("int FleetCount = 8;") != 1'
    end='    (run / "fixture.c").write_text(fixture)'
    assert runner.count(begin)==runner.count(end)==1
    start=runner.index(begin);finish=runner.index(end,start)
    runner=runner[:start]+'    fixture = texts["fixture"]\n'+runner[finish:]
    runner=once(runner,'"instrumented": args.profile is not None or args.capture}',
        '"instrumented": True, "destruction_mode": env.get("DESTRUCTION_SCENE_MODE", "baseline"), "diagnostic_plan": '+repr(str(out/'plan.json'))+'}')
    runner=once(runner,'    env["TMPDIR"] = str(tooltemp) + os.sep','    env["TMPDIR"] = str(tooltemp) + os.sep\n    env["STORM_METAL_UPSCALING"]="0"')
    (out/'run_variant.py').write_text(runner)
    model_root=APP/'RESOURCE/MODELS/Ships/Flyingdutchman1'
    textures=APP/'RESOURCE/Textures/Ships/Flyingdutchman1/hull1'
    assert model_root.is_dir() and textures.is_dir()
    inputs=[HERE/'prepare.py',HERE/'scene_exercise.hpp',archive_record,
        *[path for name,path in source_paths.items() if name!='fixture'],*model_root.rglob('*'),*textures.rglob('*'),
        *[out/name for name in ('driver.cpp','fixture.c','ship-type.c','day.c','ship-lights.c','run_variant.py')]]
    linked={}
    for word in libraries:
        path=Path(word) if word.startswith('/') else BUILD/word
        if path.is_file():linked[str(path)]=digest(path)
        elif word.startswith('-Wl,-force_load,'):
            path=Path(word.split(',',2)[2]);path=path if path.is_absolute() else BUILD/path
            assert path.is_file(),path
            linked[str(path)]=digest(path)
    assert linked==original_archives['linked_input_sha256'],'baseline archive population changed'
    identity=hashlib.sha256(json.dumps({'ref':base_ref,'archives':linked,'header_blobs':header_closure['git_blobs']},sort_keys=True).encode()).hexdigest()
    plan={
        'execution_owner':'FPS integration thread 01a13b94-e3f0-7391-9461-12bb0b4b9c96 only; no child compilation/GPU',
        'base_head':base_ref,'header_closure':header_closure,'compiled_source_identity':identity,
        'cwd':str(BUILD),'compile_commands':[compile_command],'link_command':link_command,
        'input_sha256':{str(p):digest(p) for p in inputs if p.is_file()},
        'linked_input_sha256':linked,
        'source_contract':'Frozen baseline: cached archives are the original linked-input population; source headers come from base_head with snapshot-only source include roots. No mutable current gameplay source hash or current-source freshness claim.',
        'modes':['baseline','ram','laser'],'sequence_ms':[0,200,1000,3000,6000],
        'frame_count':401,'settle_frames':100,'delta_ms':20,
        'ram':'One SHIP_BASE::ApplyHullImpact(...,1000,0), not TOUCH/collision HP/solver replay',
        'laser':'Driver logical Ship_Fire held 0<t<=0.9s -> actual ExecutePhonkLaser -> AIShipCannonController muzzle -> actual SHIP cut and DrawPhonkLaser fragment motion',
        'limits':['SeaPhonk_HullCut records event and returns ACK=0; game HP/death/quest handler absent',
                  'No physical fragment collision system is added or emulated by this driver; candidate uses its actual registered entity through normal CORE Execute',
                  'Camera framing/material/crew/rig/mast pixel completeness awaits first baseline inspection',
                  'No performance acceptance: five readbacks intentionally synchronize GPU'],
        'run_argv':['python3',str(out/'run_variant.py'),'--ships','2','--frames','401','--warmup','100','--width','1920','--height','1200'],
        'run_environment':{'DESTRUCTION_SCENE_MODE':'baseline (first), then ram, then laser; serialize','STORM_METAL_UPSCALING':'0'},
        'expected':['baseline: cut revision unchanged; all five complete ship/sea/rig/crew readbacks',
                    'ram: revision/cut_count increase; non-charred actual pieces move through native fragment draw',
                    'laser: charge decreases, beam/burn visible at0.2s, revision grows after0.65s dwell; actual native fragment motion at1/3/6s',
                    'native dead may remain false despite structural event because ACK=0; explicit fixture limit, not game acceptance']}
    (out/'plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    candidate=HERE/'candidate-after-canonical-build'
    candidate.mkdir(exist_ok=True)
    candidate_compile=['/usr/bin/c++',*current_header_flags,'-o',str(candidate/'driver.o'),'-c',str(out/'driver.cpp')]
    candidate_link=[str(candidate/'driver.o') if word==str(out/'driver.o') else
        str(candidate/'destruction-benchmark') if word==str(out/'destruction-benchmark') else word for word in link_command]
    metadata={
        'state':'DEFERRED: metadata only; do not compile or bind candidate headers/archives before canonical build',
        'execution_owner':'FPS integration thread 01a13b94-e3f0-7391-9461-12bb0b4b9c96',
        'prerequisite':'Player app idle; candidate canonical engine/archive build completed by shared owner with matching source receipt',
        'driver_source':str(out/'driver.cpp'),'driver_sha256':digest(out/'driver.cpp'),
        'scene_header':str(HERE/'scene_exercise.hpp'),'scene_header_sha256':digest(HERE/'scene_exercise.hpp'),
        'cwd':str(BUILD),'compile_commands':[candidate_compile],'link_command':candidate_link,
        'header_policy':'Use then-current canonical header roots from compile argv. Do not use baseline include-snapshot or freeze candidate headers now.',
        'binding_at_activation':'After canonical build, bind current source/build receipt, actual transitive header closure and every linked archive SHA-256 to candidate run plan. Existing baseline run_variant.py is frozen and must not be relabelled.',
        'sequence_ms':[0,200,1000,3000,6000],'frames':401,'delta_ms':20,
        'lifecycle':'Normal core_internal.Run executes registered SHIP_CUT_FRAGMENTS late Execute. Driver never calls an artificial Advance or direct GEOS CutByLaser/DrawLaserFragment.',
        'retention':'Keep baseline and candidate binaries/plans separate; baseline archive hash change invalidates its runner instead of consuming new ABI under old headers.'}
    (candidate/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(out/'plan.json')

if __name__=='__main__':main()
