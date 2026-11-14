#!/usr/bin/env python3
"""Bounded current-source hidden sea replay; run via codex-run-scratch."""
import csv, hashlib, json, math, os, pathlib, shlex, shutil, statistics, subprocess, sys

R = pathlib.Path.cwd()
B = R / 'experiments/native-metal/.cache/build'
T = pathlib.Path(os.environ['CODEX_SCRATCH_DIR'])
E = pathlib.Path(os.environ['SCENE_RESULTS'])
N = R / 'experiments/native-metal/inputs/toolchain/bin/ninja'
F = R / 'docs/evidence/nuclear-foam-20261105/footprint-antitile-20261105/fixture'
sys.path.insert(0, str(R / 'tools'))
import metal_engine_source

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def run(args, **kwargs):
    p = subprocess.run(args, cwd=B, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)
    if p.returncode:
        raise RuntimeError(f'{args[0]} exit {p.returncode}: {p.stderr.decode(errors="replace")[-7000:]}')
    return p
def command(target):
    if target == 'CMakeFiles/sea-fleet-benchmark.dir/sea_fleet_benchmark.cpp.o':
        lines = subprocess.check_output([str(N), '-C', str(B), '-t', 'commands', 'bin/sea-fleet-benchmark'], text=True).splitlines()
        return shlex.split(next(line for line in lines if ' -c ' in line and line.endswith('/sea_fleet_benchmark.cpp')))
    return shlex.split(subprocess.check_output([str(N), '-C', str(B), '-t', 'commands', target], text=True).splitlines()[-1])
def compile_object(target, source, dest, extra=()):
    args = command(target)
    for flag, value in [('-o', str(dest)), ('-MF', str(dest)+'.d'), ('-MT', str(dest))]:
        args[args.index(flag)+1] = value
    if '-v' in args: args.remove('-v')
    args[1:1] = list(extra)
    args[-1] = str(source)
    run(args)
def replace_once(text, old, new):
    if text.count(old) != 1: raise RuntimeError('source marker mismatch: '+old[:100])
    return text.replace(old, new)

E.mkdir(parents=True, exist_ok=False)
fingerprint = metal_engine_source.fingerprint()
libraries = sorted((B/'lib').glob('*.a')) + [B/'libstorm_metal.a', B/'libcloud_noise_assets.a']
before = {str(p.relative_to(R)):sha(p) for p in libraries}
sources = ['experiments/native-metal/backend.mm', 'experiments/native-metal/underwater_medium.hpp',
           'experiments/native-metal/engine/src/libs/sea/src/sea.cpp',
           'experiments/native-metal/engine/src/libs/sea/src/sea.h',
           'experiments/native-metal/engine/src/libs/sea/src/sea_ship_wake.hpp',
           'experiments/native-metal/engine/src/libs/core/src/core_impl.cpp',
           'experiments/native-metal/engine/src/libs/weather/src/sky.cpp',
           'experiments/native-metal/sea_fleet_benchmark.cpp']
source_hashes = {p:sha(R/p) for p in sources}
print('source fingerprint', fingerprint, flush=True)

# Guard only this private fixture: no global CrashReporter or environment changes.
driver = (R/'experiments/native-metal/sea_fleet_benchmark.cpp').read_text()
driver = replace_once(driver, '#include "ship_base.h"', '#include "ship_base.h"\n#include "sea_base.h"\n#include "model.h"\n#include "shared/messages.h"\n#include <csignal>\n#include <unistd.h>\n#include <sys/resource.h>')
driver = replace_once(driver, 'int main(int argc, char **argv)\n{', '''static void fixture_signal(int signal) { const char message[]="fixture signal caught\\n"; (void)!write(2,message,sizeof(message)-1); _exit(128+signal); }
int main(int argc, char **argv)
{
    struct rlimit limit{0,0}; setrlimit(RLIMIT_CORE,&limit);
    for (int s : {SIGABRT,SIGSEGV,SIGBUS,SIGILL,SIGFPE}) std::signal(s,fixture_signal);
    if (argc==2 && std::strcmp(argv[1],"--guard-check")==0) { std::raise(SIGABRT); return 99; }
''')
old = '            renderer->SetCamera(CVECTOR(0.f, 40.f, -80.f), CVECTOR(0.f, 5.f, 170.f), CVECTOR(0.f, 1.f, 0.f));'
new = '''            auto *sea=static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
            if (!sea) return 78;
            const float signedDepth=std::strtof(std::getenv("SCENE_DEPTH"),nullptr);
            static float siteX=0.f,siteZ=-80.f;
            const char *site=std::getenv("SCENE_SITE");
            if(frame==1 && site && (std::strcmp(site,"crest")==0 || std::strcmp(site,"trough")==0)) {
                const bool crest=std::strcmp(site,"crest")==0;float best=crest?-1.e9f:1.e9f;
                for(int x=-300;x<=300;x+=30)for(int z=-300;z<=300;z+=30){float y=sea->WaveXZ(float(x),float(z));if((crest&&y>best)||(!crest&&y<best)){best=y;siteX=float(x);siteZ=float(z);}}
                std::fprintf(stderr,"site=%s x=%.1f z=%.1f initial_water=%.3f\\n",site,siteX,siteZ,best);
            }
            const float surface=sea->WaveXZ(siteX,siteZ);
            const CVECTOR camera(siteX,surface-signedDepth,siteZ);
            const char *look=std::getenv("SCENE_LOOK_Y");const float lookY=look?std::strtof(look,nullptr):0.f;
            renderer->SetCamera(camera,camera+CVECTOR(0.f,lookY,250.f),CVECTOR(0.f,1.f,0.f));
            if (frame==2) std::fprintf(stderr,"camera_depth=%.3f water_y=%.3f camera_y=%.3f\\n",signedDepth,surface,camera.y);'''
driver = replace_once(driver, old, new)
if os.environ.get('SCENE_BOTTOM') == '1':
    marker = '        const auto begin = std::chrono::steady_clock::now();'
    insert = '''        static CVECTOR bottomCamera(0.f), bottomAim(0.f);
        if(frame==1){auto id=core.CreateEntity("MODELR");if(!core.Send_Message(id,"ls",MSG_MODEL_LOAD_GEO,"islands/Cuba1/Cuba1_seabed"))return 75;auto *model=static_cast<MODEL *>(core.GetEntityPointer(id));if(!model||!model->GetNode(0)||!model->GetNode(0)->geo)return 76;GEOS::INFO info{};model->GetNode(0)->geo->GetInfo(info);std::fprintf(stderr,"BOTTOM center=(%.2f,%.2f,%.2f) size=(%.2f,%.2f,%.2f)\\n",info.boxcenter.x,info.boxcenter.y,info.boxcenter.z,info.boxsize.x,info.boxsize.y,info.boxsize.z);bottomCamera=CVECTOR(info.boxcenter.x,std::min(-4.f,info.boxcenter.y+info.boxsize.y*.4f+20.f),info.boxcenter.z-30.f);bottomAim=std::getenv("SCENE_BOTTOM_HORIZON")?bottomCamera+CVECTOR(0.f,-30.f,250.f):CVECTOR(info.boxcenter.x,info.boxcenter.y,info.boxcenter.z);core.AddToLayer(3,id,30);}
        if(frame){auto *renderer=static_cast<VDX9RENDER *>(core.GetService("dx9render"));renderer->SetCamera(bottomCamera,bottomAim,CVECTOR(0.f,1.f,0.f));if(frame==2){auto *sea=static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));std::fprintf(stderr,"bottom_camera_depth=%.3f\\n",sea->WaveXZ(bottomCamera.x,bottomCamera.z)-bottomCamera.y);}}
'''
    driver = replace_once(driver, marker, insert+marker)
driver_file=T/'driver.cpp';driver_file.write_text(driver)

core = (R/'experiments/native-metal/engine/src/libs/core/src/core_impl.cpp').read_text()
core = replace_once(core, 'SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Engine Initialization Error", e.what(), nullptr);', 'std::fprintf(stderr,"fixture init error: %s\\n",e.what());')
core = replace_once(core, 'SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Wrong script version", nullptr);', 'std::fprintf(stderr,"fixture wrong script version\\n");')
core_file=T/'core_impl.cpp';core_file.write_text(core)
backend_source=R/'experiments/native-metal/backend.mm'
sea_source=R/'experiments/native-metal/engine/src/libs/sea/src/sea.cpp'
sea_extra=()
if os.environ.get('SCENE_BASELINE') == '1':
    baseline=R/'docs/evidence/underwater-medium-20261105/current-scene-20261111'
    manifest=json.loads((baseline/'baseline-near-2/manifest.json').read_text())['source_files']
    old_sea=baseline/'baseline-sea.cpp.txt';old_medium=baseline/'baseline-underwater-medium.hpp.txt'
    if sha(old_sea)!=manifest['experiments/native-metal/engine/src/libs/sea/src/sea.cpp'] or sha(old_medium)!=manifest['experiments/native-metal/underwater_medium.hpp']:raise RuntimeError('baseline source hash mismatch')
    backend_source=T/'backend.mm';shutil.copy2(R/'experiments/native-metal/backend.mm',backend_source)
    shutil.copy2(old_medium,T/'underwater_medium.hpp')
    sea_source=T/'sea.cpp';shutil.copy2(old_sea,sea_source)
    sea_extra=('-I'+str(R/'experiments/native-metal/engine/src/libs/sea/src'),)
if os.environ.get('SCENE_MEDIUM_BASE') == '1':
    baseline=R/'docs/evidence/underwater-medium-20261105/current-scene-20261111'
    old_medium=baseline/'geometry-light-before.hpp.txt'
    expected=json.loads((baseline/'final-seabed/manifest.json').read_text())['source_files']['experiments/native-metal/underwater_medium.hpp']
    if sha(old_medium)!=expected:raise RuntimeError('prior medium source hash mismatch')
    backend_source=T/'backend.mm';shutil.copy2(R/'experiments/native-metal/backend.mm',backend_source)
    shutil.copy2(old_medium,T/'underwater_medium.hpp')
if os.environ.get('SCENE_DISABLE_MEDIUM') == '1':
    text=backend_source.read_text()
    text=replace_once(text,'const bool underwaterActive=std::isfinite(waterDepth)&&waterDepth>0.f&&sceneDepthReady&&depth&&sceneDepth().pixelFormat==MTLPixelFormatDepth32Float&&underwaterMedium.ready(sceneColor().pixelFormat);','const bool underwaterActive=false;')
    backend_source=T/'backend.mm';backend_source.write_text(text)
if os.environ.get('SCENE_DEBUG_DEPTH') == '1':
    if os.environ.get('SCENE_BASELINE') == '1':raise RuntimeError('depth visualization needs current medium')
    backend_source=T/'backend.mm';shutil.copy2(R/'experiments/native-metal/backend.mm',backend_source)
    medium=(R/'experiments/native-metal/underwater_medium.hpp').read_text()
    medium=replace_once(medium,'    float z = depth.read(pixel);','    float z = depth.read(pixel);\n    return z >= .99999f ? float4(1.f,0.f,0.f,1.f) : float4(0.f,1.f,0.f,1.f);')
    (T/'underwater_medium.hpp').write_text(medium)
if os.environ.get('SCENE_DEBUG_DISTANCE') == '1':
    backend_source=T/'backend.mm';shutil.copy2(R/'experiments/native-metal/backend.mm',backend_source)
    medium=(R/'experiments/native-metal/underwater_medium.hpp').read_text()
    medium=replace_once(medium,'    float distance = z >= .99999f ? 180.f : min(length(view),180.f);','    float distance = z >= .99999f ? 180.f : min(length(view),180.f);\n    uint d=uint(min(length(view)*100.f,16777215.f)); return float4(float((d>>16)&255u)/255.f,float((d>>8)&255u)/255.f,float(d&255u)/255.f,1.f);')
    (T/'underwater_medium.hpp').write_text(medium)
if os.environ.get('SCENE_CLASS_TRACE') == '1':
    if os.environ.get('SCENE_BASELINE') == '1':raise RuntimeError('classification tracing requires current sea')
    source=sea_source.read_text()
    source=replace_once(source,'    bUnderSea = cameraDepth > 0.f;','    bUnderSea = cameraDepth > 0.f;\n    std::fprintf(stderr,"SEA_CLASS depth=%.5f y=%.5f water=%.5f under=%d whirlpool=%d\\n",cameraDepth,vCamPos.y,cameraDepth+vCamPos.y,int(bUnderSea),int(whirlpool.Frame().active));')
    sea_source=T/'sea.cpp';sea_source.write_text(source)
    sea_extra=('-I'+str(R/'experiments/native-metal/engine/src/libs/sea/src'),)
builds = [
 ('CMakeFiles/storm_metal.dir/backend.mm.o', backend_source, T/'backend.mm.o', ()),
 ('native-engine/storm/libs/sea/CMakeFiles/sea.dir/src/sea.cpp.o', sea_source, T/'sea.cpp.o', sea_extra),
 ('native-engine/storm/libs/sea/CMakeFiles/sea.dir/src/env_map.cpp.o', R/'experiments/native-metal/engine/src/libs/sea/src/env_map.cpp', T/'env_map.cpp.o', ()),
 ('native-engine/storm/libs/core/CMakeFiles/core.dir/src/core_impl.cpp.o', core_file, T/'core_impl.cpp.o', ('-I'+str(R/'experiments/native-metal/engine/src/libs/core/src'),)),
 ('native-engine/storm/libs/weather/CMakeFiles/weather.dir/src/sky.cpp.o', R/'experiments/native-metal/engine/src/libs/weather/src/sky.cpp', T/'sky.cpp.o', ()),
 ('CMakeFiles/sea-fleet-benchmark.dir/sea_fleet_benchmark.cpp.o', driver_file, T/'driver.o', ()),
]
for target, source, dest, extra in builds:
    print('compile', dest.name, flush=True); compile_object(target, source, dest, extra)
archives = {}
for name, src, objects in [('libstorm_metal.a',B/'libstorm_metal.a',['backend.mm.o']),('libsea.a',B/'lib/libsea.a',['sea.cpp.o','env_map.cpp.o']),('libcore.a',B/'lib/libcore.a',['core_impl.cpp.o']),('libweather.a',B/'lib/libweather.a',['sky.cpp.o'])]:
    dest=T/name;shutil.copy2(src,dest);run(['ar','-r',str(dest)]+[str(T/o) for o in objects]);archives[name]=dest
commands=subprocess.check_output([str(N),'-C',str(B),'-t','commands','bin/sea-fleet-benchmark'],text=True).splitlines()
link=shlex.split(commands[-1]);assert link[:2]==[':','&&'] and link[-2:]==['&&',':'];link=link[2:-2]
link=[str(T/'driver.o') if a.endswith('/sea_fleet_benchmark.cpp.o') else str(archives[pathlib.Path(a).name]) if pathlib.Path(a).name in archives else a for a in link]
link[link.index('-o')+1]=str(T/'fleet');run(link)
guard=subprocess.run([str(T/'fleet'),'--guard-check'],cwd=T,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
if guard.returncode != 134 or b'fixture signal caught' not in guard.stderr: raise RuntimeError('crash-signal guard did not pass')
print('fixture signal guard PASS',flush=True)

baseenv=dict(os.environ);baseenv.update(json.loads((F/'manifest.json').read_text())['settings']);baseenv.update(STORM_METAL_PROFILE='1',STORM_METAL_MODERN_EFFECTS='1',STORM_METAL_UPSCALING='0',STORM_QUIET_CLIENT='1')
results=[]
depths=[float(x) for x in os.environ.get('SCENE_DEPTHS','0.1').split(',')]
modes=[int(x) for x in os.environ.get('SCENE_MODES','0').split(',')]
cases=[(int(pair.split(':')[0]),float(pair.split(':')[1])) for pair in os.environ['SCENE_CASES'].split(',')] if os.environ.get('SCENE_CASES') else [(mode,depth) for mode in modes for depth in depths]
frames=int(os.environ.get('SCENE_FRAMES','60'));warmup=frames//3
if frames<3 or frames>180:raise RuntimeError('frames outside bounded replay range')
sites=os.environ.get('SCENE_CASE_SITES','').split(',') if os.environ.get('SCENE_CASE_SITES') else []
vortices=os.environ.get('SCENE_CASE_VORTICES','').split(',') if os.environ.get('SCENE_CASE_VORTICES') else []
if sites and len(sites)!=len(cases) or vortices and len(vortices)!=len(cases):raise RuntimeError('case metadata length mismatch')
for case_index,(mode,depth) in enumerate(cases):
    site=sites[case_index] if sites else os.environ.get('SCENE_SITE','')
    vortex=(vortices[case_index]=='1') if vortices else os.environ.get('SCENE_WHIRLPOOL')=='1'
    label=f'mode{mode}-depth{depth:+.2f}'+(f'-{site}' if site else '')+('-vortex' if vortex else '');out=T/label;out.mkdir()
    for name in ['fixture.c','ship-type.c','ship-lights.c','day.c','native-declarations.c','engine.ini']:shutil.copy2(F/name,out/name)
    fixture=(out/'fixture.c').read_text();fixture=replace_once(fixture,'Sea.Sea2.SimpleSea = 0;',f'Sea.Sea2.SimpleSea = {mode};')
    if vortex:
        fixture=replace_once(fixture,'    Sea.isDone = "";','    Sea.Whirlpool.CenterX = 0.0; Sea.Whirlpool.CenterZ = -80.0; Sea.Whirlpool.Radius = 90.0; Sea.Whirlpool.Intensity = 1.0; Sea.Whirlpool.Duration = 90.0; Sea.Whirlpool.ResumeAge = 5.0; Sea.Whirlpool.Start = 1;\n    Sea.isDone = "";')
    (out/'fixture.c').write_text(fixture)
    (out/'RESOURCE').symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE',target_is_directory=True)
    env=baseenv.copy();env.update(SCENE_DEPTH=str(depth),STORM_METAL_FRAME_TRACE=str(out/'renderer-frames.csv'))
    if site:env['SCENE_SITE']=site
    print('run',label,flush=True)
    with (out/'run.log').open('w') as log: p=subprocess.run([str(T/'fleet'),str(out),str(frames),'1280','800'],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=90)
    if p.returncode: raise RuntimeError(f'{label} exit {p.returncode}: '+(out/'run.log').read_text()[-6000:])
    cpu=list(csv.DictReader((out/'cpu-frames.csv').open())); vals=sorted(float(r['cpu_wall_ms']) for r in cpu[warmup:]);
    if len(vals)!=frames-warmup:raise RuntimeError('unexpected frame count')
    trace=list(csv.DictReader((out/'renderer-frames.csv').open()))
    commands={}
    for row in trace:
        if row['record']!='command':continue
        frame=int(row['frame']);start=float(row['gpu_start_s']);end=float(row['gpu_end_s'])
        if row['status']!='4' or start<=0 or end<start:raise RuntimeError('invalid GPU command')
        commands.setdefault(frame,[]).append((start,end))
    gpu=[]
    for frame in range(warmup,frames):
        if frame not in commands:raise RuntimeError('missing GPU frame')
        merged=[]
        for start,end in sorted(commands[frame]):
            if merged and start<=merged[-1][1]:merged[-1]=(merged[-1][0],max(end,merged[-1][1]))
            else:merged.append((start,end))
        gpu.append(sum(end-start for start,end in merged)*1000)
    results.append({'case':label,'frames':frames,'warmup':warmup,'cpu_median_ms':statistics.median(vals),'cpu_p95_ms':vals[math.ceil(.95*len(vals))-1], 'gpu_median_ms':statistics.median(gpu),'gpu_p95_ms':sorted(gpu)[math.ceil(.95*len(gpu))-1],'max_rss':max(int(r['rss_bytes']) for r in cpu)})
    for name in ['scene.ppm','cpu-frames.csv','renderer-frames.csv','run.log']:shutil.copy2(out/name,E/(label+'-'+name))
    print(json.dumps(results[-1]),flush=True)
if {str(p.relative_to(R)):sha(p) for p in libraries}!=before or metal_engine_source.fingerprint()!=fingerprint: raise RuntimeError('source or shared library changed during run')
used_sources=dict(source_hashes)
if os.environ.get('SCENE_BASELINE')=='1':
    used_sources['experiments/native-metal/engine/src/libs/sea/src/sea.cpp']=sha(old_sea)
    used_sources['experiments/native-metal/underwater_medium.hpp']=sha(old_medium)
if os.environ.get('SCENE_MEDIUM_BASE')=='1':used_sources['experiments/native-metal/underwater_medium.hpp']=sha(old_medium)
(E/'manifest.json').write_text(json.dumps({'source_fingerprint':fingerprint,'canonical_source_files':source_hashes,'used_source_files':used_sources,'fixture_inputs':{p.name:sha(p) for p in F.iterdir() if p.is_file()},'libraries':before,'baseline_override':os.environ.get('SCENE_BASELINE')=='1','prior_medium':os.environ.get('SCENE_MEDIUM_BASE')=='1','medium_disabled':os.environ.get('SCENE_DISABLE_MEDIUM')=='1','class_trace':os.environ.get('SCENE_CLASS_TRACE')=='1','site':os.environ.get('SCENE_SITE'),'look_y':float(os.environ.get('SCENE_LOOK_Y','0')),'whirlpool':os.environ.get('SCENE_WHIRLPOOL')=='1','bottom':os.environ.get('SCENE_BOTTOM')=='1','bottom_horizon':os.environ.get('SCENE_BOTTOM_HORIZON')=='1','results':results},indent=2)+'\n')
