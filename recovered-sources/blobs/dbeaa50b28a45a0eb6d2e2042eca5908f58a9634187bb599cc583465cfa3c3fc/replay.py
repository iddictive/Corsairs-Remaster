"""One-off current-ABI native replay; hidden isolated CORE, no player control."""
from pathlib import Path
import hashlib
import json
import os
import shlex
import shutil
import subprocess
import sys

repo = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
native = repo / 'experiments/native-metal'
build = native / '.cache/build'
evidence = Path(__file__).resolve().parent
scratch = Path(os.environ['CODEX_SCRATCH_DIR']).resolve()
scenario = sys.argv[1]
kind = 'world' if scenario in ('world', 'forest') else 'ship'
assert scenario in ('manual', 'auto', 'ram', 'baseline', 'world', 'forest')
local = scratch / kind
local.mkdir()
source = evidence.parent / ('phonk-world-async-20261016' if kind == 'world' else 'phonk-auto-scene-20261016')
for path in source.iterdir():
    if path.is_file() and path.suffix in ('.cpp', '.hpp', '.c', '.ini'):
        shutil.copy2(path, local / path.name)
(local / 'RESOURCE').symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE', target_is_directory=True)
if scenario == 'forest':
    # Native Mount can rebuild height-map caches; isolate that writable owner.
    source_resource = (local / 'RESOURCE').resolve()
    (local / 'RESOURCE').unlink()
    (local / 'RESOURCE').mkdir()
    for path in source_resource.iterdir():
        if path.name.lower() != 'foam':
            (local / 'RESOURCE' / path.name).symlink_to(path, target_is_directory=path.is_dir())
    shutil.copytree(source_resource / 'foam', local / 'RESOURCE/foam')

def replace(path, old, new):
    value = path.read_text()
    assert value.count(old) == 1, old
    path.write_text(value.replace(old, new))

probe = local / 'probe.cpp'
quiet = 'setenv("STORM_QUIET_CLIENT", "1", 1);' if kind == 'ship' else 'setenv("STORM_QUIET_CLIENT","1",1);'
replace(probe, quiet, quiet.replace('"1"', '"0"'))
if 'spdlog::set_level(spdlog::level::off);' in probe.read_text():
    replace(probe, 'spdlog::set_level(spdlog::level::off);', 'spdlog::set_level(spdlog::level::info);')
if kind == 'ship':
    replace(local / 'engine.ini', 'screen_x=1920', 'screen_x=960')
    replace(local / 'engine.ini', 'screen_y=1200', 'screen_y=600')
    helper = local / 'scene_exercise.hpp'
    replace(helper, '#include "ship_base.h"', '#include "ship_base.h"\n#include "common_camera.h"')
    replace(helper, '    firstRevision=target->GetHullContourRevision();', '''    // The actual native camera owns mode; stale script proxy must not steer
    // first-person fire. Do not add camera to layers or inject OS input.
    auto *cameraAttributes = cameras->GetAClass()->VerifyAttributeClass("NativeFixtureCamera");
    cameraAttributes->SetAttributeUseDword("WalkMode", 1);
    cameraAttributes->SetAttributeUseDword("ThirdPerson", 0);
    cameraAttributes->SetAttributeUseDword("TelescopeActive", 0);
    const auto cameraId = core.CreateEntity(automatic() ? "ship_camera" : "deck_camera", cameraAttributes);
    auto *nativeCamera = dynamic_cast<COMMON_CAMERA *>(core.GetEntityPointer(cameraId));
    require(nativeCamera, "native camera entity unavailable");
    nativeCamera->SetOn(true); nativeCamera->SetActive(true);
    AIHelper::pASeaCameras->VerifyAttributeClass("Crosshair")->SetAttributeUseDword("OutsideCamera", 1);
    firstRevision=target->GetHullContourRevision();''')
    # Offscreen third-person acquisition is intentional. Animated draw count
    # cannot be its oracle; cut revision/contact and full CORE still are.
    replace(probe, 'if (frames > 1 && !StormMetalRawSkinnedDraws(device)) return 74;',
            'if (frames > 1 && !destruction_scene::automatic() && !StormMetalRawSkinnedDraws(device)) return 74;')
else:
    replace(probe, '    snapshot(cwd/(island+"-before.ppm"),center);', '''    // Same message order as PROGRAM/utils.c CreateGrass. This uses the real
    // island Grass consumer, not a direct standalone forest bridge call.
    const auto grass=core.CreateEntity("Grass");
    if(!grass)throw std::runtime_error("native island grass entity");
    core.Send_Message(grass,"ls",41666,"junglesprites.tga");
    core.Send_Message(grass,"lffffff",42666,1.f,16.f,32.f,1000.f,6000.f,0.f);
    const std::string grassPath="resource/models/islands/"+island+"/"+island+".grs";
    if(!core.Send_Message(grass,"ls",40666,grassPath.c_str()))throw std::runtime_error("native island forest load");
    core.AddToLayer(3,grass,1000);
    renderer->SetCamera(center+CVECTOR(0.f,450.f,-500.f),center,CVECTOR(0.f,1.f,0.f));
    for(unsigned i=0;i<100;++i)if(!core_internal.Run())throw std::runtime_error("forest progressive preparation");
    snapshot(cwd/(island+"-before.ppm"),center);''')
    if scenario == 'forest':
        replace(probe, '    const auto terrain=core.CreateEntity("MODELR",attributes("Terrain"));', '    entid_t terrain=0;')
        start = '    for(const auto &item:std::vector<std::pair<entid_t,std::string>>{'
        end = '    attributes("Fort")->SetAttribute("PhonkWorldKey","fort_probe");'
        value = probe.read_text()
        assert value.count(start) == value.count(end) == 1
        first, last = value.index(start), value.index(end)
        value = value[:first] + '''    // Real sea Mount is required: a foreign land collision model has no
    // primary sea model identity and intentionally cannot generate sea trees.
    attributes("Scene")->SetAttribute("LightingPath","day1");
    core.Send_Message(scene,"lsss",MSG_ISLAND_LOAD_GEO,"islands",("Islands/"+island).c_str(),island.c_str());
    auto *mounted=dynamic_cast<ISLAND_BASE *>(core.GetEntityPointer(scene));
    if(!mounted || !(terrain=mounted->GetModelEID()))throw std::runtime_error("native island Mount");
    const auto seabed=mounted->GetSeabedEID();
    for(const auto &item:std::vector<std::pair<entid_t,std::string>>{{reflection,root+"_refl"},{fort,root+"_fort1"}}) {
        core.Send_Message(item.first,"ls",MSG_MODEL_LOAD_GEO,item.second.c_str());model(item.first)->Update();
    }
''' + value[last:]
        probe.write_text(value)
        replace(probe, '    snapshot(cwd/(island+"-before.ppm"),center);', '''    const CVECTOR forestFocus(-994.4f,57.9f,401.2f);
    snapshot(cwd/(island+"-forest.ppm"),forestFocus);
    snapshot(cwd/(island+"-before.ppm"),center);''')
        replace(probe, '    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();', '''    // Exact real-FNV-placement root from the retained scene input, plus an
    // actual distant supported root. Cut only after prior world-state checks.
    const CVECTOR forestRoot(-887.8226318359375f,111.01901245117188f,241.5111541748047f);
    const CVECTOR distantRoot(-440.1786804199219f,36.71901321411133f,-2020.531494140625f);
    if(!world->IsWorldPositionRetained(terrain,forestRoot)||!world->IsWorldPositionRetained(terrain,distantRoot))
        throw std::runtime_error("forest support prior-state oracle");
    snapshot(cwd/"Antigua-forest-root-before.ppm",forestRoot);
    const auto priorForestCuts=cuts->GetAttributeAsDword("count");
    if(!world->BlastWorldWeapon(forestRoot,100.f,-1))throw std::runtime_error("forest support blast admission");
    awaitCuts(priorForestCuts+1,"forest root blast");
    if(world->IsWorldPositionRetained(terrain,forestRoot)||!world->IsWorldPositionRetained(terrain,distantRoot))
        throw std::runtime_error("forest committed support/neighbor oracle");
    snapshot(cwd/"Antigua-forest-root-after.ppm",forestRoot);
    std::puts("forest_actual_mount_grass_committed_support_neighbor=PASS");
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();''')

inputs = evidence / (scenario + '-inputs')
inputs.mkdir(exist_ok=True)
for path in local.iterdir():
    if path.is_file():
        shutil.copy2(path, inputs / path.name)
lines = subprocess.check_output([str(native / 'inputs/toolchain/bin/ninja'), '-C', str(build), '-t', 'commands', 'sea-fleet-benchmark'], text=True).splitlines()
compile_line = [line for line in lines if ' -c ' in line and '/sea_fleet_benchmark.cpp' in line]
link_line = [line for line in lines if ' -o bin/sea-fleet-benchmark ' in line]
assert len(compile_line) == len(link_line) == 1
compile = shlex.split(compile_line[0])
for flag in ('-MD', '-MT', '-MF'):
    at = compile.index(flag)
    del compile[at:at + (1 if flag == '-MD' else 2)]
obj = 'native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
compile[compile.index(str(native / 'sea_fleet_benchmark.cpp'))] = str(probe)
compile[compile.index(obj)] = str(local / 'probe.o')
compile.remove('-v')
compile += ['-UNDEBUG', '-I' + str(native / 'engine/src/libs/sea_ai/src'), '-I' + str(native / 'engine/src/libs/sea_cameras/src')]
link = shlex.split(link_line[0])[2:-2]
link[link.index(obj)] = str(local / 'probe.o')
binary = local / 'replay'
link[link.index('bin/sea-fleet-benchmark')] = str(binary)
link.remove('-v')

def idle():
    output = subprocess.check_output(['ps', '-axo', 'pid=,comm='], text=True)
    assert not any(row.strip().endswith('/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine') for row in output.splitlines()), 'player active'

def run(command, name, cwd, env=None, gpu=False):
    (evidence / (name + '-command.json')).write_text(json.dumps(command, indent=2) + '\n')
    with (evidence / (name + '.log')).open('w') as log:
        process = subprocess.Popen(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT, env=env)
        try:
            while process.poll() is None:
                if gpu:
                    idle()
                try:
                    process.wait(timeout=.5)
                except subprocess.TimeoutExpired:
                    pass
        except BaseException:
            process.terminate(); process.wait(timeout=10)
            raise
    print(name, process.returncode, flush=True)
    return process.returncode

idle()
receipt = {'compile': compile, 'link': link, 'native_receipt': json.loads((native / '.cache/gameplay-compiler.json').read_text()),
           'libraries': {str(p.relative_to(build)): hashlib.sha256(p.read_bytes()).hexdigest() for p in build.rglob('*.a')},
           'fixture_inputs': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs.iterdir()},
           'player_control': False, 'player_save': False, 'acceptance': 'component-only'}
(evidence / (scenario + '-receipt.json')).write_text(json.dumps(receipt, indent=2) + '\n')
assert run(compile, scenario + '-compile', build) == 0
assert run(link, scenario + '-link', build) == 0
env = dict(os.environ, STORM_METAL_FRAME_TRACE=str(local / 'frame-trace.csv'), STORM_METAL_PROFILE='1')
if kind == 'ship':
    env['DESTRUCTION_SCENE_MODE'] = 'ram' if scenario == 'ram' else 'baseline' if scenario == 'baseline' else 'laser'
    if scenario == 'auto': env['DESTRUCTION_AUTO'] = '1'
    command = [str(binary), str(local), '401', '960', '600']
else:
    command = [str(binary), str(local), 'Antigua', '1']
result = run(command, scenario + '-native', local, env, True)
outputs = evidence / (scenario + '-outputs')
outputs.mkdir(exist_ok=True)
for p in local.iterdir():
    if p.suffix in ('.csv', '.ppm'):
        shutil.copy2(p, outputs / p.name)
logs = local / 'userdata/Logs'
if logs.exists(): shutil.copytree(logs, outputs / 'logs', dirs_exist_ok=True)
print('native_result', result, 'player_acceptance=0', flush=True)
raise SystemExit(result)
