import os, pathlib, shlex, shutil, subprocess, sys
root=pathlib.Path(__file__).resolve().parents[3]; build=root/'experiments/native-metal/.cache/build'; tmp=pathlib.Path(os.environ['CODEX_SCRATCH_DIR']); ninja=root/'experiments/native-metal/inputs/toolchain/bin/ninja'; src=root/'experiments/native-metal/engine/src/libs/sea/src'; fixture=root/'docs/evidence/nuclear-foam-20261105/footprint-antitile-20261105/fixture'
def cmds(t): return subprocess.check_output([str(ninja),'-C',str(build),'-t','commands',t],text=True).splitlines()
def run(c, **kw): return subprocess.run(c,cwd=build,check=True,**kw)
def rewrite(a,out):
 for f,v in [('-o',str(out)),('-MF',str(out)+'.d'),('-MT',str(out))]:
  if f in a:a[a.index(f)+1]=v
 return a
sea=(src/'sea.cpp').read_text(); marker='    // Disabled for ordinary play: the coarse exclusion exposes polygon facets.'
candidate=os.getenv('CANDIDATE')=='1'; morph=(src/'sea_geomorph.h').read_text()
if candidate:
 old='return Sqr(x - vCamPos.x) + /*Sqr((y - vCamPos.y)) + */ Sqr(z - vCamPos.z);'; assert sea.count(old)==1
 sea=sea.replace(old,'const float h = std::max(0.f, std::abs(y - vCamPos.y) - 100.f); return Sqr(x - vCamPos.x) + h*h + Sqr(z - vCamPos.z);')
 old='pB.iLOD, fGridStep, fLodScale);'; assert sea.count(old)==1
 sea=sea.replace(old,'pB.iLOD, fGridStep, fLodScale, std::max(0.f, std::abs(vSeaCenterPos.y - vCamPos.y) - 100.f));')
 morph=morph.replace('int level, float gridStep, float lodScale)','int level, float gridStep, float lodScale, float heightOffset)');morph=morph.replace('dx * dx + dz * dz','dx * dx + dz * dz + heightOffset * heightOffset')
(tmp/'sea_geomorph.h').write_text(morph)
assert sea.count(marker)==1
sea=sea.replace(marker,'    std::fprintf(stderr,"ALT_PROBE y=%.1f blocks=%zu vertices=%d\\n",vCamPos.y,aBlocks.size(),iNumVPoints);\n'+marker)
sea=sea.replace('            aBlocks.erase(aBlocks.begin() + i, aBlocks.end());','            std::fprintf(stderr,"ALT_TRUNCATE removed=%zu\\n",aBlocks.size()-i);\n            aBlocks.erase(aBlocks.begin() + i, aBlocks.end());')
sea='#include <cstdio>\n'+sea; (tmp/'sea.cpp').write_text(sea)
sc=shlex.split(cmds('native-engine/storm/libs/sea/CMakeFiles/sea.dir/src/sea.cpp.o')[-1]);rewrite(sc,tmp/'sea.cpp.o');sc.insert(1,'-I'+str(src));sc[-1]=str(tmp/'sea.cpp');run(sc,stdout=subprocess.DEVNULL,stderr=None)
archive=tmp/'libsea.a';shutil.copy2(build/'lib/libsea.a',archive);run(['ar','-r',str(archive),str(tmp/'sea.cpp.o')],stdout=subprocess.DEVNULL,stderr=None)
driver=(root/'experiments/native-metal/sea_fleet_benchmark.cpp').read_text(); old='renderer->SetCamera(CVECTOR(0.f, 40.f, -80.f), CVECTOR(0.f, 5.f, 170.f), CVECTOR(0.f, 1.f, 0.f));'; assert driver.count(old)==1
driver=driver.replace(old,'renderer->SetCamera(CVECTOR(float(frame)*2.f, float(std::atof(std::getenv("PROBE_ALT"))), 0.f), CVECTOR(float(frame)*2.f, 0.f, float(std::atof(std::getenv("PROBE_LOOK")))), CVECTOR(0.f, 0.f, 1.f));')
driver=driver.replace('const int frames = std::stoi(argv[2]), width = std::stoi(argv[3]), height = std::stoi(argv[4]);','int frames = std::stoi(argv[2]), width = std::stoi(argv[3]), height = std::stoi(argv[4]);').replace('if (desc.Width != unsigned(width) || desc.Height != unsigned(height)) return 70;','width=int(desc.Width); height=int(desc.Height);')
(tmp/'driver.cpp').write_text(driver)
allc=cmds('bin/sea-fleet-benchmark'); line=next(x for x in allc if ' -c ' in x and x.endswith('/sea_fleet_benchmark.cpp')); dc=shlex.split(line);rewrite(dc,tmp/'driver.o');dc[-1]=str(tmp/'driver.cpp');run(dc,stdout=subprocess.DEVNULL,stderr=None)
foamfix=os.getenv('FOAMFIX') in ('1','prod','base'); prod=os.getenv('FOAMFIX') in ('prod','base'); baseline=os.getenv('FOAMFIX')=='base'
if foamfix:
 native=root/'experiments/native-metal'; (tmp/'backend').mkdir(exist_ok=True)
 for path in native.glob('*.hpp'):shutil.copy2(path,tmp/'backend'/path.name)
 if os.getenv('BASE_CURRENT')=='1': (tmp/'backend/sea_shaders.hpp').write_text(subprocess.check_output(['git','show','33bed15:experiments/native-metal/sea_shaders.hpp'],cwd=root,text=True))
 shutil.copy2(native/'backend.mm',tmp/'backend/backend.mm')
 if baseline:shutil.copy2(root/'docs/evidence/nuclear-foam-20261105/footprint-antitile-20261105/candidate-sea_shaders.hpp',tmp/'backend/sea_shaders.hpp')
 if prod and os.getenv('THRESHOLD')=='1':
  file=tmp/'backend/sea_shaders.hpp';s=file.read_text();assert s.count('smoothstep(1.f,3.f,footprint)')==1;file.write_text(s.replace('smoothstep(1.f,3.f,footprint)','smoothstep(.1f,.5f,footprint)'))
 if prod and os.getenv('PREVIOUS')=='1':shutil.copy2(root/'docs/evidence/sea-high-altitude-20261105/foam-pixel-20261105/before-sea_shaders.hpp.txt',tmp/'backend/sea_shaders.hpp')
 if prod and os.getenv('FOAMFAST')=='1':
  file=tmp/'backend/sea_shaders.hpp';shader=file.read_text();old='''    float4 sum=0;
    for(int y=0;y<4;++y) for(int x=0;x<4;++x)
        sum+=tex.sample(state,float2((float(x)+.5f)/4.f,(float(y)+.5f)/4.f));
    return sum/16.f;'''; assert shader.count(old)==1
  new='''    constexpr sampler mipState(coord::normalized,address::repeat,filter::linear,mip_filter::linear);
    float4 sum=0;
    for(int y=0;y<2;++y) for(int x=0;x<2;++x)
        sum+=tex.sample(mipState,float2((float(x)+.5f)/2.f,(float(y)+.5f)/2.f),level(float(tex.get_num_mip_levels()-1)));
    return sum/4.f;''';file.write_text(shader.replace(old,new))
 if not prod:
  shader=(root/'docs/evidence/nuclear-foam-20261105/footprint-antitile-20261105/candidate-sea_shaders.hpp').read_text();needle='fragment float4 seafoam_fs(SeaO o [[stage_in]]';assert shader.count(needle)==1
  helper='float4 seaFoamMipTest(texture2d<float> tex,sampler state,float2 uv) { constexpr sampler mipState(coord::normalized,address::repeat,filter::linear,mip_filter::linear); return tex.sample(mipState,uv,level(float(tex.get_num_mip_levels()-1))); }\n'
  shader=shader.replace(needle,helper+needle)
  shader=shader.replace('t[0].sample(s[0],o.t3.xy)','seaFoamMipTest(t[0],s[0],o.t3.xy)')
  shader=shader.replace('t[0].sample(s[0],substrate*(profile.x*profile.z))','seaFoamMipTest(t[0],s[0],substrate*(profile.x*profile.z))')
  (tmp/'backend/sea_shaders.hpp').write_text(shader)
 bc=shlex.split(cmds('CMakeFiles/storm_metal.dir/backend.mm.o')[-1]);rewrite(bc,tmp/'backend/backend.mm.o');bc[-1]=str(tmp/'backend/backend.mm');run(bc,stdout=subprocess.DEVNULL,stderr=None)
 backarch=tmp/'libstorm_metal.a';shutil.copy2(build/'libstorm_metal.a',backarch);run(['ar','-r',str(backarch),str(tmp/'backend/backend.mm.o')],stdout=subprocess.DEVNULL,stderr=None)
link=shlex.split(allc[-1]); assert link[:2]==[':','&&'] and link[-2:]==['&&',':'];link=link[2:-2];link=[str(archive) if x in ('lib/libsea.a',str(build/'lib/libsea.a')) else str(tmp/'driver.o') if x.endswith('/sea_fleet_benchmark.cpp.o') else str(backarch) if foamfix and x in ('libstorm_metal.a',str(build/'libstorm_metal.a')) else x for x in link];link[link.index('-o')+1]=str(tmp/'fleet');run(link,stdout=subprocess.DEVNULL,stderr=None)
for h,look in ((int(os.getenv('PROBE_HEIGHT','3800')),int(os.getenv('PROBE_DIRECTION','1'))),):
 frames=int(os.getenv('PROBE_FRAMES','16')); width=int(os.getenv('PROBE_WIDTH','1280')); height=int(os.getenv('PROBE_VIEW_HEIGHT','800'));d=tmp/(os.getenv('PROBE_LABEL','')+('hires-' if width>1280 else '')+('compiledbase-' if baseline else 'fast-' if os.getenv('FOAMFAST')=='1' else 'prod-' if prod else 'mipsampler-' if foamfix else 'standard-')+str(frames)+'-'+str(h)+'-'+str(look));d.mkdir()
 for name in ('fixture.c','ship-type.c','ship-lights.c','day.c','native-declarations.c','engine.ini'):shutil.copy2(fixture/name,d/name)
 ini=d/'engine.ini'; raw=ini.read_text();raw=raw.replace('screen_x=1280',f'screen_x={width}').replace('screen_y=800',f'screen_y={height}');ini.write_text(raw)
 (d/'RESOURCE').symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE',target_is_directory=True)
 env=os.environ.copy();env['PROBE_ALT']=str(h);env['PROBE_LOOK']=str(look);env['STORM_METAL_FRAME_TRACE']=str(d/'renderer-frames.csv');env['STORM_METAL_PROFILE']='1'; p=subprocess.run([str(tmp/'fleet'),str(d),str(frames),str(width),str(height)],cwd=d,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=90)
 print('height',h,'look',look,'exit',p.returncode);print('\n'.join(x for x in p.stdout.splitlines() if 'ALT_PROBE' in x or 'ALT_TRUNCATE' in x or 'backbuffer' in x));

 shutil.copy2(d/'scene.ppm',pathlib.Path(os.environ['OUTPUT_FRAME']))
 shutil.copy2(d/'cpu-frames.csv',pathlib.Path(os.environ['OUTPUT_FRAME']+'.cpu.csv'))
 shutil.copy2(d/'renderer-frames.csv',pathlib.Path(os.environ['OUTPUT_FRAME']+'.gpu.csv'))
