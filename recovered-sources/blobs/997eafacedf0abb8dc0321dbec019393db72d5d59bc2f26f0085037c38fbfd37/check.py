"""Task-local source/component evidence; never build, stage, launch or edit player state."""
from pathlib import Path
import hashlib, json, re, shlex, shutil, subprocess, sys

ROOT = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
NATIVE = ROOT / 'experiments/native-metal'
BUILD = NATIVE / '.cache/build'
HERE = Path(__file__).resolve().parent
WORK = Path(sys.argv[1]).resolve()
assert WORK.name.startswith('corsairs-sea-only-') and WORK.is_dir()
sys.path.insert(0, str(ROOT / 'tools'))
import gameplay_sources

def run(name, command, cwd):
    result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (HERE / (name + '.log')).write_text(result.stdout)
    (HERE / (name + '-command.json')).write_text(json.dumps([str(x) for x in command], indent=2))
    print(name, result.returncode, result.stdout[-1500:] if result.returncode else result.stdout[-100:], flush=True)
    return result.returncode

def function(text, name):
    start = re.search(r'(?:void|bool|ref|float|int)\s+' + re.escape(name) + r'\s*\(', text).start()
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        if text[end] == '{': depth += 1
        if text[end] == '}': depth -= 1
        end += 1
    return text[start:end]

results = {}
if sys.argv[2] == 'syntax':
    for module, unit in [('sea_cameras', 'ship_camera'), ('sea_cameras', 'free_camera'),
                         ('sea_cameras', 'deck_camera'), ('sea_cameras', 'sea_cameras'),
                         ('worldmap', 'world_map'), ('renderer', 's_device')]:
        obj = f'native-engine/storm/libs/{module}/CMakeFiles/{module}.dir/src/{unit}.cpp.o'
        lines = subprocess.check_output(['ninja', '-C', str(BUILD), '-t', 'commands', obj], text=True).splitlines()
        command = shlex.split(lines[-1])
        for flag in ('-MD', '-c', '-v'):
            if flag in command: command.remove(flag)
        for flag in ('-MT', '-MF', '-o'):
            index = command.index(flag)
            del command[index:index+2]
        command += ['-fsyntax-only']
        results[unit] = run('syntax-' + unit, command, BUILD)
elif sys.argv[2] == 'vm':
    full = WORK / 'full'
    shutil.copytree(Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/PROGRAM'), full)
    for name, (data, _, _) in gameplay_sources.read()[0].items():
        if name.startswith('PROGRAM/'):
            target = full / name.removeprefix('PROGRAM/')
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    main = full / 'seadogs.c'
    data, count = re.subn(rb'void\s+Main\s*\(\s*\)', b'void FullProgramUnusedMain()', main.read_bytes())
    assert count == 1
    main.write_bytes(data)
    (full / 'cases.c').write_text('#include "seadogs.c"\nint FullChain(){return 0;}\n')
    results['full_vm'] = run('full-vm', [str(BUILD / 'bin/script-vm-probe'), str(full), 'FullChain', '0'], WORK)
elif sys.argv[2] == 'component':
    old = (ROOT / '.local-archives/direct-sailing-20261016/component/cases.c').read_text()
    prefix = old[old.index('#define COMPANION_MAX'):old.index('// Region travel')]
    prefix = prefix.replace('void SeaLogin(ref login){bSkipSeaLogin=false;', 'void SeaLogin(ref login){bSkipSeaLogin=Redirect;')
    current = (ROOT / 'src/gameplay/PROGRAM/sea_ai/direct_sailing.c').read_text()
    current = current.replace('IsEntity(', 'FixtureEntity(').replace('SendMessage(', 'FixtureMessage(')
    sea = (ROOT / 'src/gameplay/PROGRAM/sea_ai/sea.c').read_text()
    done = function(sea, 'Sea_MapTransitionDone')
    case = old[old.index('void Setup()'):old.index('int AfterLoad()')]
    case = case.replace('Setup();', 'Setup(); pchar.nation=0;')
    a = case.index('worldMap.exists=1;')
    b = case.index('DeleteAttribute(&Sea,"Phonk");', a)
    case = case[:a] + '''worldMap.exists=1;
wdmLockReload=true;
SeaMapTransition.exists=1; SeaMapTransition.seaOnly=1;
Event("WorldMap_TransitionReady");
Event("WorldMap_TransitionReady");
if(Returns!=1 || Flags!=1 || !wdmLockReload)return 30;
if(!CheckAttribute(&worldMap,"directSail.returning"))return 31;
''' + case[b:]
    a = case.index('worldMap.directSail=1;', case.index('if(Impulses'))
    case = case[:a] + '''worldMap.directSail=1; worldMap.directSail.retainCameras=1;
Redirect=true; Sea_DirectSailLogin(&worldMap);
if(DeletedCameras!=1 || CheckAttribute(&worldMap,"directSail"))return 32;
Redirect=false; worldMap.directSail=1; bSeaReloadStarted=true; bCanSwitchCameras=false;
SetEventHandler("Sea_MapTransitionCommit","Sea_MapTransitionCommit",0);
SetEventHandler("Sea_MapTransitionDone","Sea_MapTransitionDone",0);
Sea_DirectSailAbort();
if(bSeaReloadStarted || !bCanSwitchCameras || CheckAttribute(&worldMap,"directSail"))return 33;
return 0;
}
'''
    component = WORK / 'component'
    component.mkdir(exist_ok=True)
    body = ('bool bCanSwitchCameras,Redirect; int Flags,DeletedCameras;\n' + prefix +
            'void BI_ShowInterface(bool visible){}\nvoid DeleteSeaCamerasEnvironment(){DeletedCameras++;}\n' +
            'void wdmSetNationFlag(int nation){Flags++;}\nvoid Sea_MapTransitionCommit(){}\n' + done + '\n' + current + '\n' + case)
    (component / 'cases.c').write_text(body)
    (HERE / 'component-cases.c').write_text(body)
    results['component_vm'] = run('component-vm', [str(BUILD / 'bin/script-vm-probe'), str(component), 'Run', '0'], WORK)
else:
    raise ValueError(sys.argv[2])

(HERE / (sys.argv[2] + '-results.json')).write_text(json.dumps(results, indent=2))
raise SystemExit(int(any(results.values())))
