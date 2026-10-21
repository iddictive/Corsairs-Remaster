from pathlib import Path
import subprocess, shlex, json, hashlib
r=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal');p=Path(__file__).parent
src=(r/'backend.mm').read_text()
extra='''\nextern "C" unsigned StormTestCasterDraws(void*p){return static_cast<Device*>(static_cast<storm_render::Device*>(p))->landShadow.prepass.casterDraws;}
extern "C" unsigned StormTestPackets(void*p){return static_cast<Device*>(static_cast<storm_render::Device*>(p))->landShadow.packets.size();}
extern "C" unsigned StormTestRegistry(void*p){return static_cast<Device*>(static_cast<storm_render::Device*>(p))->landShadow.pointRegistry.lifecycle().entries.size();}
'''
(p/'backend-fixture.mm').write_text(src+extra)
s=(r/'sea_point_shadow_gpu_probe.cpp').read_text()
def replace(a,b,count=1):
 global s
 assert s.count(a)==count,(a,s.count(a));s=s.replace(a,b)
replace('extern "C" bool StormMetalSceneLight', '''extern "C" bool StormMetalSetShipAuxiliaryDetail(void*,bool);
extern "C" void StormMetalLandLocation(void*,bool);
extern "C" unsigned StormTestCasterDraws(void*);
extern "C" unsigned StormTestPackets(void*);
extern "C" unsigned StormTestRegistry(void*);
extern "C" bool StormMetalSceneLight''')
replace('struct FramePixels { uint32_t red{},blue{}; };','struct FramePixels { uint32_t red{},blue{}; unsigned packets{}, registry{}; };')
replace('    auto frame=[&]', '    bool casterDetail=true,receiverDetail=true;\n    auto frame=[&]')
replace('unsigned scope=StormMetalBeginSceneModel(d);if(blocker&&red)draw(-.1f,.14f,.1f);if(blocker&&blue)draw(.05f,.14f,.1f);draw(0,.9f,.6f);StormMetalEndLandModel(d,scope);', '''unsigned scope=StormMetalBeginSceneModel(d);auto previous=StormMetalSetShipAuxiliaryDetail(d,casterDetail);if(blocker&&red)draw(-.1f,.14f,.1f);if(blocker&&blue)draw(.05f,.14f,.1f);StormMetalSetShipAuxiliaryDetail(d,receiverDetail);draw(0,.9f,.6f);StormMetalSetShipAuxiliaryDetail(d,previous);StormMetalEndLandModel(d,scope);''')
replace('FramePixels value{};std::memcpy(&value.red', 'FramePixels value{};value.packets=StormTestPackets(d);value.registry=StormTestRegistry(d);std::memcpy(&value.red',2)
replace('        if(lateBlocker){\n            unsigned scope=StormMetalBeginSceneModel(d);', '        if(lateBlocker){\n            const bool previous=StormMetalSetShipAuxiliaryDetail(d,casterDetail);\n            unsigned scope=StormMetalBeginSceneModel(d);')
replace('            StormMetalEndLandModel(d,scope);\n        }','            StormMetalEndLandModel(d,scope);\n            StormMetalSetShipAuxiliaryDetail(d,previous);\n        }')
extra_tests='''
    const auto validBefore=frame(true,true,false);
    casterDetail=false;const auto culled=frame(true,true,false);
    need(culled.red==redLit.red,"LOD false removes same-visible caster shadow immediately after a valid frame");
    need(culled.packets==validBefore.packets&&culled.registry+1==validBefore.registry,"receiver packets retained while prior-valid caster retires");
    casterDetail=true;receiverDetail=false;const auto receiving=frame(true,true,false);
    need(receiving.red==validBefore.red,"LOD false receiver still receives independent caster shadow");
    receiverDetail=true;const auto restored=frame(true,true,false);
    need(restored.red==validBefore.red&&restored.registry==validBefore.registry,"zoom/near scope restores prior-valid caster");
    casterDetail=false;const auto sunCulled=lateSunFrame(true,true,true);casterDetail=true;
    unsigned restoredSun=0;for(unsigned i=0;i<4096;i++)if(channel(sunMastShadow.pixels[i],16)>channel(sunShadow.pixels[i],16)+8)restoredSun+=std::abs(channel(sunCulled.pixels[i],16)-channel(sunMastShadow.pixels[i],16))<=3;
    need(restoredSun>8,"LOD false suppresses skinned sun caster while keeping its ordinary model draw");
    need(StormMetalSetShipAuxiliaryDetail(nullptr,false),"null device defaults to detail");
    int foreign=0;need(StormMetalSetShipAuxiliaryDetail(&foreign,false),"foreign device defaults to detail");
    need(StormMetalSetShipAuxiliaryDetail(d,false),"detail starts enabled");
    need(!StormMetalSetShipAuxiliaryDetail(d,true),"nested scope returns suppressed state");
    need(StormMetalSetShipAuxiliaryDetail(d,false),"restore outer scope returns nested state");
    need(!StormMetalSetShipAuxiliaryDetail(d,false),"outer scope restored");
    need(storm_render::succeeded(d->Present()),"present while suppressed");
    need(StormMetalSetShipAuxiliaryDetail(d,false),"Present clears leaked LOD state");
    StormMetalLandLocation(d,true);need(StormMetalSetShipAuxiliaryDetail(d,false),"location transition clears leaked LOD state");
    StormMetalLandLocation(d,false);need(StormMetalSetShipAuxiliaryDetail(d,true),"sea transition clears leaked LOD state");
    target->Release();target=nullptr;
    StormMetalSetShipAuxiliaryDetail(d,false);need(storm_render::succeeded(d->Reset(&pp)),"valid device Reset");
    need(StormMetalSetShipAuxiliaryDetail(d,true),"Reset clears leaked LOD state");d->GetRenderTarget(&target);
    std::fprintf(stderr,"[aux-lod] valid=%08x culled=%08x receiver=%08x restored=%08x packets=%u/%u registry=%u/%u sun-restored=%u\\n",validBefore.red,culled.red,receiving.red,restored.red,validBefore.packets,culled.packets,validBefore.registry,culled.registry,restoredSun);
'''
replace('    alphaTexture->Release();',extra_tests+'    alphaTexture->Release();')
(p/'aux-probe.cpp').write_text(s)
inputs=['backend.mm','land_shadow.hpp','point_shadow_registry.hpp','sea_point_shadow_gpu_probe.cpp','.cache/build/libstorm_metal.a','.cache/build/CMakeFiles/storm_metal.dir/dev_telemetry.cpp.o','.cache/build/CMakeFiles/storm_metal.dir/gpu_skinning_bridge.cpp.o']
(p/'final-inputs.json').write_text(json.dumps({x:hashlib.sha256((r/x).read_bytes()).hexdigest() for x in inputs},indent=2))
a=shlex.split((p/'compile-original.txt').read_text());a[a.index('-o')+1]=str(p/'backend.mm.o');a[-1]=str(p/'backend-fixture.mm')
for flag in ['-MD','-MT','-MF']:
 if flag in a:
  i=a.index(flag);a[i:i+(1 if flag=='-MD' else 2)]=[]
a[a.index('-O3')]='-O1';(p/'compile-final.json').write_text(json.dumps(a,indent=2))
with (p/'compile-final.log').open('w') as f:subprocess.run(['/usr/bin/time','-l']+a,cwd=r/'.cache/build',stdout=f,stderr=f,timeout=300,check=True)
# Exact canonical frozen bridge/telemetry objects; the candidate backend object is linked explicitly.
a=['clang++','-std=c++20','-O1','-arch','arm64','-mmacosx-version-min=15.0','-I'+str(r),'-I'+str(r/'inputs/native/d3d9/dxvk-native/include/native/windows'),'-I'+str(r/'inputs/native/d3d9/sdl2-fixed/include/SDL2'),str(p/'aux-probe.cpp'),str(p/'backend.mm.o'),str(r/'.cache/build/CMakeFiles/storm_metal.dir/dev_telemetry.cpp.o'),str(r/'.cache/build/CMakeFiles/storm_metal.dir/gpu_skinning_bridge.cpp.o'),str(r/'.cache/build/libcloud_noise_assets.a'),str(r/'inputs/native/d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib'),'-Wl,-rpath,'+str(r/'inputs/native/d3d9/sdl2-fixed/lib'),'-framework','Metal','-framework','MetalFX','-framework','Cocoa','-framework','QuartzCore','-o',str(p/'aux-probe')]
(p/'link.json').write_text(json.dumps(a,indent=2))
with (p/'link.log').open('w') as f:subprocess.run(a,stdout=f,stderr=f,timeout=60,check=True)
print(p)
