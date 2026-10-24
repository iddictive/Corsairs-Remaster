"""Private SAVE replay through the real native load and transition consumers.

Run with codex-run-scratch; only aggregate results may be retained in Git.
The source SAVE is read-only. Renderer, scripts and native actors are real.
"""
import json
import hashlib
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
WORK = Path(os.environ['CODEX_SCRATCH_DIR'])
NATIVE = ROOT / 'experiments/native-metal'
BUILD = NATIVE / '.cache/build'
sys.path.insert(0, str(ROOT / 'tools'))
import gameplay_sources

assert len(sys.argv) == 2
save = Path(sys.argv[1]).resolve(strict=True)
processes = subprocess.check_output(['ps', '-axo', 'comm='], text=True)
assert not any('/metal-engine' in line or line.endswith('/engine-1')
               for line in processes.splitlines()), 'Close the game before native replay'
runtime = WORK / 'runtime'
runtime.mkdir()
shutil.copytree(NATIVE / '.cache/runtime/PROGRAM', runtime / 'PROGRAM')
canonical=gameplay_sources.read()[0]
source_identity={name:hashlib.sha256(row[0]).hexdigest() for name,row in canonical.items()}
fixture_identity=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
for name, (data, _, _) in canonical.items():
    if name.startswith('PROGRAM/'):
        target = runtime / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
traffic = runtime / 'PROGRAM/sea_ai/sea_traffic.c'
if os.environ.get('CORSAIRS_REPLAY_DIAGNOSE_FIRST'):
    body=traffic.read_bytes().replace(b'\r\n',b'\n')
    start=body.index(b'void SeaTraffic_Admit(aref descriptor)')
    end=body.index(b'\nvoid ',start+1)
    observed=body[start:end]
    marker=b'void SeaTraffic_Admit(aref descriptor)\n{'
    assert observed.count(marker)==1
    observed=observed.replace(marker,marker+b'\n    string probeID=GetAttributeName(descriptor);')
    replacements={
        b'if (hulls == 0 || iNumShips + hulls > MAX_SHIPS_ON_SEA) return;':
        b'NativeReplayAdmissions.(probeID).hulls=hulls; NativeReplayAdmissions.(probeID).ships=iNumShips; if (hulls == 0 || iNumShips + hulls > MAX_SHIPS_ON_SEA) {NativeReplayAdmissions.(probeID).reason="capacity";return;}',
        b'if (hulls > available || hulls > REAL_SHIPS_QUANTITY - GetRealShipsCount()) return;':
        b'if (hulls > available || hulls > REAL_SHIPS_QUANTITY - GetRealShipsCount()) {NativeReplayAdmissions.(probeID).reason="resources";return;}',
        b'if (!WdmFleetSeaAssemblyReady(fleet)) return;':
        b'if (!WdmFleetSeaAssemblyReady(fleet)) {NativeReplayAdmissions.(probeID).reason="assembly";return;}',
        b'{ SeaTraffic_Rollback(&bundle); return; }':
        b'{NativeReplayAdmissions.(probeID).reason="import-or-mount";SeaTraffic_Rollback(&bundle);return;}',
    }
    for before,after in replacements.items():
        assert observed.count(before)==(2 if before.startswith(b'{ SeaTraffic') else 1)
        observed=observed.replace(before,after)
    traffic.write_bytes(b'object NativeReplayAdmissions;\n'+body[:start]+observed+body[end:])
with traffic.open('ab') as handle:
    handle.write(b'''
int NativeReplayInspect() {
    int stale=0;
    aref encounters; makearef(encounters,worldMap.encounters);
    for(int d=0;d<GetAttributesNum(encounters);d++) {
        aref descriptor=GetAttributeN(encounters,d);
        if(!SeaTraffic_HasOriginalGroup(descriptor))continue;
        int g=Group_FindGroup(descriptor.trafficActorGroup);
        int index=Group_GetGroupCommanderIndexR(&AIGroups[g]);
        if(!IsEntity(&Characters[index]))stale++;
    }
    return stale;
}
bool NativeReplayMap() {return Sea_TryMapLoad(false);}
int NativeReplayCamera() {
    int selected=-1;float best=3000.0;
    for(int i=0;i<iNumShips;i++) {
        int index=Ships[i];
        if(index<940 || !IsEntity(&Characters[index]))continue;
        ref captain=&Characters[index];
        if(!CheckAttribute(captain,"trafficFleetID"))continue;
        float dx=stf(captain.Ship.Pos.x)-stf(pchar.Ship.Pos.x);
        float dz=stf(captain.Ship.Pos.z)-stf(pchar.Ship.Pos.z);
        float distance=sqrt(dx*dx+dz*dz);
        if(distance<best) {best=distance;selected=index;}
    }
    if(selected>=0)SeaCameras_SetShipForSeaCamera(&Characters[selected]);
    return selected;
}
object NativeReplaySelected;
ref NativeReplayFleet() {
    // Select an actual map encounter, not a dormant worldwide descriptor.
    int count=wdmGetNumberShipEncounters();
    for(int d=0;d<count;d++) {
        if(!wdmSetCurrentShipData(d))continue;
        string path="encounters."+worldMap.encounter.id;
        if(!CheckAttribute(&worldMap,path))continue;
        aref descriptor;makearef(descriptor,worldMap.(path));
        if(!WdmTrafficIsOrdinary(descriptor) || SeaTraffic_HasOriginalGroup(descriptor))continue;
        if(!CheckAttribute(descriptor,"encdata.trafficRoster.count"))continue;
        CopyAttributes(&NativeReplaySelected,descriptor);
        aref fleet; makearef(fleet,NativeReplaySelected.encdata);
        if(!CheckAttribute(fleet,"trafficFleetID"))fleet.trafficFleetID=descriptor.trafficFleetID;
        if(WdmFleetSeaAssemblyReady(fleet)) {
            NativeReplaySelected.liveHulls=WdmFleetSeaHullCount(fleet);
            NativeReplaySelected.x=worldMap.encounter.x;
            NativeReplaySelected.z=worldMap.encounter.z;
            return &NativeReplaySelected;
        }
    }
    return &worldMap;
}
''')
installed = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE')
if os.environ.get('CORSAIRS_REPLAY_PREPARED_CHART'):
    # Inspect candidate model bytes before modifying the player's installation.
    (runtime/'RESOURCE/MODELS').mkdir(parents=True)
    for entry in installed.iterdir():
        if entry.name not in ('MODELS','Textures'):(runtime/'RESOURCE'/entry.name).symlink_to(entry)
    for entry in (installed/'MODELS').iterdir():
        candidate = (NATIVE/'.cache/seamless-sea/RESOURCE/MODELS/SeamlessSeaChart'
                     if entry.name=='SeamlessSeaChart' else entry)
        (runtime/'RESOURCE/MODELS'/entry.name).symlink_to(candidate)
    overlay=runtime/'RESOURCE/Textures/WorldMap/geometry'
    overlay.mkdir(parents=True)
    for directory,relative,excluded in ((installed/'Textures','Textures','WorldMap'),
            (installed/'Textures/WorldMap','Textures/WorldMap','geometry')):
        for entry in directory.iterdir():
            if entry.name.casefold()!=excluded.casefold():
                (runtime/'RESOURCE'/relative/entry.name).symlink_to(entry)
    for entry in (installed/'Textures/WorldMap/geometry').iterdir():
        if entry.name.casefold()!='seamlesssea_land.tga.tx':(overlay/entry.name).symlink_to(entry)
    (overlay/'SeamlessSea_land.tga.tx').symlink_to(NATIVE/'.cache/seamless-sea/RESOURCE/Textures/WorldMap/geometry/SeamlessSea_land.tga.tx')
else:
    (runtime/'RESOURCE').symlink_to(installed)
shutil.copy2(save, runtime / 'replay.save')
shutil.copytree(NATIVE / 'engine/src/libs/shared_headers/include/shared', WORK / 'resource/shared')
# The graphical engine displays compile failures in a modal OS alert even
# with a hidden window. Compile this exact private PROGRAM headlessly first.
main=runtime/'PROGRAM/seadogs.c'
original=main.read_bytes()
renamed,count=re.subn(rb'void\s+Main\s*\(\s*\)',b'void NativeReplayGameMain()',original)
assert count==1
probe=WORK/'script-vm-probe'
shutil.copy2(BUILD/'bin/script-vm-probe',probe)
try:
    main.write_bytes(renamed)
    (runtime/'PROGRAM/cases.c').write_text('#include "seadogs.c"\nint Run(){return 0;}\n')
    check=subprocess.run([str(probe),str(runtime/'PROGRAM'),'Run','0'],
                         cwd=WORK,capture_output=True,text=True,timeout=60)
    if check.returncode:
        log=runtime/'PROGRAM/.vm-userdata/Logs/error.log'
        print(log.read_text(errors='replace')[-2000:] if log.exists() else check.stderr[-2000:])
        raise SystemExit(check.returncode)
finally:
    main.write_bytes(original)
(runtime / 'engine.ini').write_text(
    'full_screen=0\nscreen_x=960\nscreen_y=600\nprogram_directory=PROGRAM\\\n'
    'run=seadogs.c\nmax_fps=0\nvsync=0\nNearClipPlane=0.115\nFarClipPlane=32000\n'
    'startFontIniFile=resource\\ini\\fonts.ini\nfont=interface_normal\n'
    '[compatibility]\ntarget_version=teho\n[script]\ndebuginfo=1\ncodefiles=0\ntracefiles=0\n')
source = WORK / 'replay.cpp'
source.write_text(r'''
#include "core_impl.h"
#include "dx9render.h"
#include "os_window.hpp"
#include "steam_api.hpp"
#include "ai_ship.h"
#include "wdm_objects.h"
#include "wdm_player_ship.h"
#include "wdm_camera.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <limits>
#include <spdlog/spdlog.h>
int counter(const char *name) {
    auto *v=static_cast<VDATA *>(core.GetScriptVariable(name));
    return v ? v->GetInt() : -1;
}
entid_t mapEntity() {
    const auto resident=core.GetEntityId("WorldMapResident");
    return resident ? resident : core.GetEntityId("WorldMap");
}
bool report(const char *phase) {
    int invalid=0, phantom=0, maximum=-1;
    AIShip *player=nullptr;
    for(auto *ship:AIShip::AIShips)if(ship->isMainCharacter())player=ship;
    float nearest=std::numeric_limits<float>::infinity();int nearby=0;
    for(auto *ship: AIShip::AIShips) {
        auto *c=ship->GetACharacter();
        int index=c ? int(c->GetAttributeAsDword("index")) : -1;
        maximum=std::max(maximum,index);
        if(index>=940) ++phantom;
        if(player && c && c->GetAttribute("trafficFleetID")) {
            float distance=ship->GetDistance(*player);
            nearest=std::min(nearest,distance);if(distance<3000.f)++nearby;
        }
        if(!c || !c->FindAClass(c,"Ship.Cannons")) {
            ++invalid;
            std::cout<<"invalid_actor="<<index<<"\n";
        }
    }
    std::cout<<"phase="<<phase<<" native_ships="<<AIShip::AIShips.size()
             <<" script_ships="<<counter("iNumShips")<<" phantom_ships="<<phantom
             <<" phantom_cursor="<<counter("iNumFantoms")<<" maximum_actor="<<maximum
             <<" invalid_cannons="<<invalid<<" sea="<<counter("bSeaActive")
             <<" reloading="<<counter("bSeaReloadStarted")
             <<" map="<<mapEntity()<<" nearby_fleet_hulls="<<nearby<<" nearest_fleet="<<nearest<<std::endl;
    if(std::getenv("CORSAIRS_REPLAY_DIAGNOSE_FIRST") && phantom>0 && !mapEntity()) {
        auto *observations=static_cast<VDATA *>(core.GetScriptVariable("NativeReplayAdmissions"));
        auto *root=observations?observations->GetAClass():nullptr;
        if(root)for(unsigned i=0;i<root->GetAttributesNum();++i) {
            auto *a=root->GetAttributeClass(i);
            std::cout<<"admission="<<a->GetThisName()<<" reason="<<(a->GetAttribute("reason")?a->GetAttribute("reason"):"none")
                     <<" hulls="<<a->GetAttributeAsDword("hulls")<<" ships="<<a->GetAttributeAsDword("ships")
                     <<" group="<<(a->GetAttribute("group")?a->GetAttribute("group"):"none")
                     <<" live="<<a->GetAttributeAsDword("live")<<" members="<<a->GetAttributeAsDword("members")<<'\n';
        }
        for(const char *name:{"worldMap","AISea","NativeReplaySelected","wdmLoginToSea"}) {
            auto *v=static_cast<VDATA *>(core.GetScriptVariable(name));auto *a=v?v->GetAClass():nullptr;
            if(!a)continue;
            for(const char *path:{"playerShipX","playerShipZ","trafficFleetID","liveHulls","x","z","PlayerGroup.x","PlayerGroup.z","worldFrame.atlas_to_sea.tx","worldFrame.atlas_to_sea.tz"}) {
                auto *p=a->FindAClass(a,path);if(p)std::cout<<name<<'.'<<path<<'='<<p->GetThisAttr()<<'\n';
            }
        }
        for(auto *s:AIShip::AIShips) {
            auto *c=s->GetACharacter();if(!c)continue;
            std::cout<<"actor="<<c->GetAttributeAsDword("index")<<" main="<<s->isMainCharacter()
                     <<" x="<<s->GetPos().x<<" z="<<s->GetPos().z
                     <<" fleet="<<(c->GetAttribute("trafficFleetID")?c->GetAttribute("trafficFleetID"):"none")<<'\n';
        }
    }
    return invalid==0;
}
bool capture(const char *name) {
    auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    auto *device=static_cast<storm_render::Device *>(renderer->GetNativeDevice());
    storm_render::Surface *target=nullptr,*readback=nullptr;
    if(storm_render::failed(device->GetRenderTarget(&target)))return false;
    storm_render::SurfaceDesc desc{};target->GetDesc(&desc);
    if(storm_render::failed(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,storm_render::AssetFormat::A8R8G8B8,&readback)) ||
       storm_render::failed(device->GetRenderTargetData(target,readback)))return false;
    storm_render::LockedRect pixels{};
    if(storm_render::failed(readback->LockRect(&pixels,nullptr,storm_render::lock::readOnly)))return false;
    std::ofstream out(name,std::ios::binary);out<<"P6\n"<<desc.Width<<' '<<desc.Height<<"\n255\n";
    for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x) {
        auto *p=static_cast<unsigned char *>(pixels.pBits)+y*pixels.Pitch+x*4;
        const char rgb[]={char(p[2]),char(p[1]),char(p[0])};out.write(rgb,3);
    }
    readback->UnlockRect();readback->Release();target->Release();return bool(out);
}
int main(int argc,char **argv) {
    if(argc!=2)return 64;
    const auto root=std::filesystem::canonical(argv[1]);
    std::filesystem::current_path(root);
    std::filesystem::create_directories(root/"userdata/Logs");
    setenv("STORM_USERDATA",(root/"userdata").c_str(),1);
    setenv("STORM_QUIET_CLIENT","1",1);
    spdlog::set_level(spdlog::level::off);
    if(SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO)!=0)return 65;
    steamapi::SteamApi::getInstance(true);
    auto window=storm::OSWindow::Create(960,600,0,false,false);
    if(!window || SDL_GetWindowFlags(static_cast<SDL_Window *>(window->OSHandle()))&SDL_WINDOW_SHOWN)return 66;
    core_internal.Init();core_internal.SetWindow(window);core_internal.InitBase();
    core_internal.SetDeltaTime(16);
    if(!core_internal.Run())return 67;
    core_internal.EraseEntities();
    {std::fstream file(root/"replay.save",std::ios::binary|std::ios::in);
     if(!core_internal.Compiler->LoadState(file))return 68;}
    if(!report("loaded"))return 70;
    if(std::getenv("CORSAIRS_REPLAY_MISSING_CANNONS")) {
        auto *ship=AIShip::AIShips.front();auto *c=ship->GetACharacter();
        auto *guns=c->FindAClass(c,"Ship.Cannons");if(!guns)return 74;
        c->DeleteAttributeClassX(guns);
        ship->GetCannonController()->Execute(.016f);
        std::cout<<"missing_cannons_survived=1"<<std::endl;
        core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();return 0;
    }
    core_internal.Compiler->SetEventHandler("native_replay_inspect","NativeReplayInspect",0);
    if(auto *r=core.Event("native_replay_inspect"))std::cout<<"false_residency="<<r->GetInt()<<std::endl;
    core_internal.Compiler->SetEventHandler("native_replay_fleet","NativeReplayFleet",0);
    core_internal.Compiler->SetEventHandler("native_replay_camera","NativeReplayCamera",0);
    double cpu=0;int measured=0;
    const bool chartViews=std::getenv("CORSAIRS_REPLAY_CHART_VIEWS");
    for(int frame=0;frame<1220;++frame) {
        SDL_PumpEvents();
        SDL_Delay(16);
        if(chartViews && (frame==120 || frame==176 || frame==206 || frame==236 || frame==266)) {
            if(frame==176)wdmObjects->playerShip->Teleport(-360.f,820.f,0.f);
            if(frame==206)wdmObjects->playerShip->Teleport(200.f,590.f,0.f);
            if(frame==236)wdmObjects->playerShip->Teleport(510.f,460.f,0.f);
            if(frame==266)wdmObjects->playerShip->Teleport(-400.f,-900.f,0.f);
            wdmObjects->camera->Init(0.f,300.f);wdmObjects->camera->lock=true;
        }
        if(!chartViews && frame%400==225) {
            auto *r=core.Event("native_replay_fleet");
            auto *fleet=r ? r->GetAClass() : nullptr;
            if(!fleet || !fleet->GetAttribute("trafficFleetID") || !wdmObjects || !wdmObjects->playerShip)return 73;
            wdmObjects->playerShip->Teleport(fleet->GetAttributeAsFloat("x")+1.f,fleet->GetAttributeAsFloat("z")+1.f,0.f);
            std::cout<<"approached_saved_fleet=1"<<std::endl;
        }
        if(frame%200==30 && (!chartViews || frame==30)) {
            auto &vm=*core_internal.Compiler;
            const char *function=frame%400==30?"NativeReplayMap":"wdmReloadToSea";
            vm.SetEventHandler("native_replay_step",function,0);
            if(auto *r=core.Event("native_replay_step"))
                std::cout<<"transition_requested="<<frame<<" accepted="<<r->GetInt()<<std::endl;
            vm.DelEventHandler("native_replay_step",function);
        }
        if(frame==1190) {
            auto *r=core.Event("native_replay_camera");
            if(!r || r->GetInt()<940)return 78;
            std::cout<<"camera_new_fleet_actor="<<r->GetInt()<<std::endl;
        }
        const auto start=std::chrono::steady_clock::now();
        if(!core_internal.Run())return 69;
        cpu+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++measured;
        if(frame%400==229 && !mapEntity())return 71;
        if(frame%400==19 && frame>19 && (!counter("bSeaActive") || mapEntity()))return 72;
        if(frame%200==29 || frame==1219)
            if(!report(mapEntity()?"map":"sea"))return 70;
        if(!chartViews && frame==210 && !capture("chart.ppm"))return 76;
        if(chartViews) {
            const char *view=frame==175?"chart.ppm":frame==205?"cuba.ppm":
                             frame==235?"hispaniola.ppm":frame==265?"puerto-rico.ppm":
                             frame==295?"mainland-south.ppm":nullptr;
            if(view && !capture(view))return 76;
            if(frame==296)break;
        }
        if(frame==1210 && !capture("sea.ppm"))return 77;
        if(frame==429 && std::getenv("CORSAIRS_REPLAY_DIAGNOSE_FIRST"))break;
    }
    std::cout<<"mean_cpu_ms="<<cpu/measured<<std::endl;
    // Follow the game's scene teardown while particle/renderer services and
    // native projectile children are still alive, before wiping the VM world.
    core_internal.Compiler->SetEventHandler("native_replay_unload","DeleteSeaEnvironment",0);
    core.Event("native_replay_unload");
    for(int frame=0;frame<3;++frame)core_internal.Run();
    core_internal.EraseEntities();core_internal.CleanUp();window.reset();SDL_Quit();
    return 0;
}
''')
commands = subprocess.check_output([str(NATIVE / 'inputs/toolchain/bin/ninja'),
    '-C', str(BUILD), '-t', 'commands', 'sea-fleet-benchmark'], text=True).splitlines()
compile_command = next(shlex.split(c) for c in commands
                       if ' -c ' in c and c.endswith('/sea_fleet_benchmark.cpp'))
link = shlex.split(commands[-1])
assert link[:2] == [':', '&&'] and link[-2:] == ['&&', ':']
link = link[2:-2]
obj = WORK / 'replay.o'
compile_command[compile_command.index('-o')+1] = str(obj)
compile_command[compile_command.index('-c')+1] = str(source)
compile_command += ['-I'+str(NATIVE / 'engine/src/libs/sea_ai/src')]
compile_command += ['-I'+str(NATIVE / 'engine/src/libs/worldmap/src')]
if '-MF' in compile_command:
    compile_command[compile_command.index('-MF')+1] = str(WORK / 'replay.d')
old = next(x for x in link if x.endswith('/sea_fleet_benchmark.cpp.o'))
link[link.index(old)] = str(obj)
link[link.index('-o')+1] = str(WORK / 'replay')
for command in (compile_command, link):
    built = subprocess.run([x for x in command if x != '-v'], cwd=BUILD,
                           capture_output=True, text=True, timeout=90)
    if built.returncode:
        errors = [line for line in (built.stdout + built.stderr).splitlines()
                  if 'error:' in line or 'undefined' in line or 'Undefined' in line]
        print('\n'.join(errors)[:3000] or built.stderr[-3000:])
        raise SystemExit(built.returncode)
result = subprocess.run([str(WORK / 'replay'), str(runtime)], cwd=WORK,
                        capture_output=True, text=True, timeout=180)
from PIL import Image
output=NATIVE/'.cache/diagnostics/native-player-transition-20261024'
output.mkdir(parents=True,exist_ok=True)
for name in ('chart','sea','cuba','hispaniola','puerto-rico','mainland-south'):
    if (runtime/(name+'.ppm')).is_file():
        Image.open(runtime/(name+'.ppm')).save(output/(name+'.png'))
report={'exit':result.returncode,'stdout':result.stdout,
        'source_sha256':source_identity,'fixture_sha256':fixture_identity,
                  'stderr':result.stderr[-1500:],
                  'transitions': {p.name:p.read_text(errors='replace')[-6000:]
                     for p in (runtime/'userdata/Logs').glob('*transition*')}}
from collections import Counter
report['script_errors']=dict(Counter(line for line in result.stderr.splitlines()
                                    if line.startswith('RUNTIME ERROR')))
suffix=('missing-cannons' if os.environ.get('CORSAIRS_REPLAY_MISSING_CANNONS') else
        'chart-views' if os.environ.get('CORSAIRS_REPLAY_CHART_VIEWS') else
        'first-entry' if os.environ.get('CORSAIRS_REPLAY_DIAGNOSE_FIRST') else 'round-trip')
(output/(suffix+'.json')).write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
print(json.dumps({'exit':result.returncode,'report':str(output/(suffix+'.json')),
                  'stdout_tail':result.stdout[-1500:],'script_errors':report['script_errors']},ensure_ascii=False))
raise SystemExit(result.returncode)
