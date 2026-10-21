"""One-off frozen fleet cost probe; does not present generated frames or deliver app."""
from pathlib import Path
import difflib, hashlib, json, subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
source=subprocess.check_output(['git','show','87b4acb:experiments/native-metal/backend.mm'],cwd=ROOT,text=True)
backend=source
def once(old,new):
    global backend
    assert backend.count(old)==1,(old[:120],backend.count(old))
    backend=backend.replace(old,new)

once('#include "metalfx.hpp"','#include "metalfx.hpp"\n#include "frame_interpolation.hpp"\n#include <thread>')
once('struct Device : storm_render::Device {','''struct Device : storm_render::Device {
 sm::FrameInterpolation fiProbe;
 bool fiProbeEnabled=[](){const char*v=std::getenv("STORM_METAL_FI_COST_PROBE");return v&&std::strcmp(v,"1")==0;}();
 uint64_t fiProbeFrame=~uint64_t(0),fiProbeEncodes=0;
 id<MTLTexture> fiProbeOutput=nil;
 void encodeFIProbe(){
  if(!fiProbeEnabled||fiProbeFrame==dynamicSkyFrame||!back||target!=back||interfaceBackSceneActive)return;
  fiProbeFrame=dynamicSkyFrame;finish();if(!command)command=[queue commandBuffer];
  if(fiProbe.status()==sm::FrameInterpolation::Status::Disabled){
   fiProbe.prepare(metal,back->desc.Width,back->desc.Height);
   // Diagnostic-only cold setup happens before warm samples; the production
   // module itself remains asynchronous and never waits on a rendering thread.
   auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(35);
   while(fiProbe.status()==sm::FrameInterpolation::Status::Loading&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(5));
   if(fiProbe.status()!=sm::FrameInterpolation::Status::Ready){fprintf(stderr,"FI setup failed: %s\\n",sm::FrameInterpolation::statusName(fiProbe.status()));std::exit(76);}
  }
  const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  fiProbeOutput=fiProbe.encode(command,back->gpu,flightRing.current,seconds);
  if(fiProbeOutput)++fiProbeEncodes;
 }
 bool saveFIProbe(){
  const auto counts=fiProbe.counters();
  fprintf(stderr,"[FI cost] enabled=%u generated=%llu seeded=%llu fallback=%llu admitted=%llu status=%s\\n",unsigned(fiProbeEnabled),(unsigned long long)counts.generated,(unsigned long long)counts.seeded,(unsigned long long)counts.fallback,(unsigned long long)fiProbeEncodes,sm::FrameInterpolation::statusName(fiProbe.status()));
  if(!fiProbeEnabled)return true;if(!fiProbeOutput||counts.generated<256||counts.fallback)return false;
  const auto w=fiProbeOutput.width,h=fiProbeOutput.height,pitch=(w*4+255)&~NSUInteger(255);
  auto buffer=[metal newBufferWithLength:pitch*h options:MTLResourceStorageModeShared];auto cb=[queue commandBuffer];auto blit=[cb blitCommandEncoder];
  [blit copyFromTexture:fiProbeOutput sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(w,h,1) toBuffer:buffer destinationOffset:0 destinationBytesPerRow:pitch destinationBytesPerImage:pitch*h];[blit endEncoding];[cb commit];[cb waitUntilCompleted];if(cb.status!=MTLCommandBufferStatusCompleted)return false;
  const char*trace=std::getenv("STORM_METAL_FRAME_TRACE");if(!trace)return false;std::string path=trace;auto slash=path.find_last_of('/');if(slash==std::string::npos)return false;path.resize(slash+1);path+="generated.ppm";
  FILE*file=std::fopen(path.c_str(),"wb");if(!file)return false;std::fprintf(file,"P6\\n%lu %lu\\n255\\n",(unsigned long)w,(unsigned long)h);
  std::vector<unsigned char> rgb(w*h*3);const auto*bytes=(const unsigned char*)buffer.contents;
  for(NSUInteger y=0;y<h;++y)for(NSUInteger x=0;x<w;++x){const auto*p=bytes+y*pitch+x*4;const auto i=(y*w+x)*3;rgb[i]=p[2];rgb[i+1]=p[1];rgb[i+2]=p[0];}
  const bool ok=std::fwrite(rgb.data(),1,rgb.size(),file)==rgb.size();std::fclose(file);return ok;
 }
''')
once('else device->markSceneHudBoundary();','else{device->markSceneHudBoundary();device->encodeFIProbe();}')
once('d->synchronize();return d->profile.trace.flush();','d->synchronize();return d->saveFIProbe()&&d->profile.trace.flush();')
(HERE/'backend.mm').write_text(backend)
(HERE/'backend.patch').write_text(''.join(difflib.unified_diff(source.splitlines(True),backend.splitlines(True),fromfile='87b4acb/backend.mm',tofile='cost-only/backend.mm')))
print(json.dumps({'scope':'Diagnostic generation cost only, native presentation untouched','base':'87b4acb','backend_sha256':hashlib.sha256(backend.encode()).hexdigest(),'header_sha256':hashlib.sha256((HERE/'frame_interpolation.hpp').read_bytes()).hexdigest()}))
