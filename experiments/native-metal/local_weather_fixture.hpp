#pragma once
// Optional deterministic GPU weather scene, owned by dynamic_sky-probe.
// Geometry is in metres; exports are review evidence, not game acceptance.
#include "volumetric_sky.hpp"
#include "local_weather.hpp"
#include <fstream>
#include <iostream>
using namespace storm_metal;
static const char* localWeatherFixtureMSL=R"MSL(
kernel void density_checks(device float*output [[buffer(1)]],constant LocalWeatherUniform&u [[buffer(0)]],texture2d<float>terrain [[texture(1)]],texture3d<float>shape [[texture(2)]],texture3d<float>detail [[texture(3)]],texture2d<float>regional [[texture(4)]],uint id [[thread_position_in_grid]]){
    if(id==0)output[0]=localCloudDensity(float3(0,400,2500),u,terrain,shape,detail,regional);
    if(id==1)output[1]=localCloudDensity(float3(0,2000,2500),u,terrain,shape,detail,regional);
    if(id==2){float peak=0;for(int z=-4;z<=4;z++)for(int x=-4;x<=4;x++){float3 p=float3(x*150.f,0,2500.f+z*150.f);p.y=localTerrainHeight(p.xz,terrain,u.terrainBounds)+120;peak=max(peak,localCloudDensity(p,u,terrain,shape,detail,regional));}output[2]=peak;}
    if(id==3){float maximum=0;for(uint layer=0;layer<3;layer++){CloudLayer l=cloudLayer(layer,u.weather);float3 p=float3(2000,g_radius+l.base-10,500);maximum=max(maximum,density(p,float3(.8),0,shape,detail,regional,u.weather,l));}output[3]=maximum;}
}
kernel void medium_reprojection(device float4*output [[buffer(0)]],constant SkyFogDraw*draws [[buffer(1)]],texture3d<float>transport [[texture(0)]],uint id [[thread_position_in_grid]]){
    float3 pixel=float3(192.5f,108.5f,.99995f);
    output[id]=skyLocalMedium(pixel,draws[id],transport);
}
kernel void fixture_image(texture2d<float,access::write>output [[texture(0)]],texture2d<float>clouds [[texture(1)]],texture2d<float>atmosphere [[texture(2)]],texture2d<float>transmittance [[texture(3)]],texture2d<float>terrain [[texture(4)]],texture3d<float>transport [[texture(5)]],constant LocalWeatherUniform&u [[buffer(0)]],constant SkyFogDraw&draw [[buffer(1)]],uint2 id [[thread_position_in_grid]]){
    if(id.x>=output.get_width()||id.y>=output.get_height())return;
    float2 pixel=float2(id)+.5f,uv=pixel/float2(output.get_width(),output.get_height());
    float4 far=u.worldFromClip*float4(uv.x*2-1,1-uv.y*2,1,1);float3 ray=normalize(far.xyz/far.w-u.cameraRange.xyz);
    float distance=6000;bool ground=false;
    for(int j=1;j<300;j++){float d=j*20.f;float3 p=u.cameraRange.xyz+ray*d;if(p.y<=localTerrainHeight(p.xz,terrain,u.terrainBounds)){distance=d;ground=true;break;}}
    float3 position=u.cameraRange.xyz+ray*distance;
    float4 clip=draw.volumeClipFromWorld*float4(position,1);
    float4 rad=skyRadiance(ray,u.weather,clouds,clouds,atmosphere,transmittance);
    float3 color=rad.rgb+float3(1,.25,.055)*rad.a;
    if(ground)color=float3(.11,.15,.085)*(.10f+.9f*max(0.f,ray.y<0.f?u.weather.sunDirection_hour.y:.5f));
    float4 result=skyLocalComposite(float4(color,1),float3(pixel,clip.z/clip.w),draw,transport);
    output.write(result,id);
}
)MSL";
static int checkLocalWeather(const char* outputDirectory){@autoreleasepool{
 auto device=MTLCreateSystemDefaultDevice();auto queue=[device newCommandQueue];NSError*error=nil;
 std::string source=std::string(dynamicSkyShaderSource)+localWeatherMSL+localWeatherFixtureMSL;
 auto lib=[device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];
 if(!lib){std::cerr<<error.localizedDescription.UTF8String<<"\n";return 2;}
 VolumetricSky sky;if(!sky.prepare(device,lib))return 3;
 auto pipeline=[&](NSString*name){return [device newComputePipelineStateWithFunction:[lib newFunctionWithName:name] error:&error];};
 auto texture=[&](unsigned w,unsigned h,MTLPixelFormat format){auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:w height:h mipmapped:NO];d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;d.storageMode=MTLStorageModeShared;return [device newTextureWithDescriptor:d];};
 auto cloud=texture(256,256,MTLPixelFormatRGBA16Float),image=texture(384,216,MTLPixelFormatRGBA8Unorm);
 std::vector<island_geometry::Face>faces;
 auto point=[](int x,int z){float px=x*100.f,pz=z*100.f+2500;return island_geometry::P{px,900.f*std::max(0.f,1.f-std::sqrt(px*px+(pz-2500)*(pz-2500))/1000.f),pz};};
 for(int z=-15;z<15;z++)for(int x=-15;x<15;x++){island_geometry::Face a{},b{};a.v[0].p=point(x,z);a.v[1].p=point(x+1,z);a.v[2].p=point(x,z+1);b.v[0].p=point(x+1,z);b.v[1].p=point(x+1,z+1);b.v[2].p=point(x,z+1);faces.push_back(a);faces.push_back(b);}
 weatherTerrain.capture(&faces,faces);if(weatherTerrain.heights.size()!=WeatherTerrain::side*WeatherTerrain::side||*std::max_element(weatherTerrain.heights.begin(),weatherTerrain.heights.end())<880)return 4;
 simd_float3 camera={0,100,-800},target={0,650,2500};auto forward=simd_normalize(target-camera),right=simd_normalize(simd_cross(simd_make_float3(0,1,0),forward)),up=simd_cross(forward,right);
 simd_float4x4 view{};view.columns[0]={right.x,up.x,forward.x,0};view.columns[1]={right.y,up.y,forward.y,0};view.columns[2]={right.z,up.z,forward.z,0};view.columns[3]={-simd_dot(right,camera),-simd_dot(up,camera),-simd_dot(forward,camera),1};
 simd_float4x4 projection{};float f=1.5,near=.1f,far=12000;projection.columns[0]={f/(384.f/216.f),0,0,0};projection.columns[1]={0,f,0,0};projection.columns[2]={0,0,far/(far-near),1};projection.columns[3]={0,0,-near*far/(far-near),0};
 LocalWeatherVolume local;
 for(unsigned scenario=0;scenario<4;scenario++){
  DynamicSkyInput input;input.hour=scenario==2?22:8;input.cloudCoverage=scenario==1?.32f:scenario==3?.62f:.90f;input.cloudDensity=.8;input.windSpeed=10;input.cloudPhaseX=350;input.cloudPhaseY=400;
  auto w=makeDynamicSkyUniform(input);
  auto command=[queue commandBuffer];auto blit=[command blitCommandEncoder];[blit generateMipmapsForTexture:sky.shape];[blit generateMipmapsForTexture:sky.detail];[blit endEncoding];
  auto dispatch=[&](id<MTLComputePipelineState>p,id<MTLTexture>out){auto e=[command computeCommandEncoder];[e setComputePipelineState:p];[e setTexture:out atIndex:0];return e;};
  auto e=dispatch(sky.transmitKernel,sky.transmittance);[e dispatchThreads:MTLSizeMake(256,64,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[e endEncoding];
  e=dispatch(sky.atmosKernel,sky.atmosphere);[e setTexture:sky.transmittance atIndex:1];[e setBytes:&w length:sizeof(w) atIndex:0];[e dispatchThreads:MTLSizeMake(256,128,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[e endEncoding];
  e=dispatch(sky.cloudKernel,cloud);[e setTexture:sky.shape atIndex:1];[e setTexture:sky.detail atIndex:2];[e setTexture:sky.weatherNoise atIndex:3];[e setBytes:&w length:sizeof(w) atIndex:0];simd_uint4 region={256,0,0,256};[e setBytes:&region length:sizeof(region) atIndex:1];[e dispatchThreads:MTLSizeMake(256,256,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[e endEncoding];
  e=[command computeCommandEncoder];[e setComputePipelineState:sky.solarKernel];[e setBytes:&w length:sizeof(w) atIndex:0];[e setBuffer:sky.solarSamples[scenario%3] offset:0 atIndex:1];[e setTexture:cloud atIndex:0];[e setTexture:cloud atIndex:1];[e setTexture:sky.atmosphere atIndex:2];[e setTexture:sky.transmittance atIndex:3];[e dispatchThreads:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(1,1,1)];[e endEncoding];
  [command commit];[command waitUntilCompleted];if(command.error){std::cerr<<command.error.localizedDescription.UTF8String;return 5;}
  command=[queue commandBuffer];if(!local.encode(device,lib,command,scenario,w,view,projection,{0,0,0},sky.shape,sky.detail,sky.weatherNoise,sky.atmosphere,sky.solarSamples[scenario%3],true))return 6;
  [command commit];[command waitUntilCompleted];double ms=(command.GPUEndTime-command.GPUStartTime)*1000;std::cout<<"scenario "<<scenario<<" local GPU ms "<<ms<<"\n";
  command=[queue commandBuffer];auto checks=[device newBufferWithLength:16 options:MTLResourceStorageModeShared];
  LocalWeatherUniform u{simd_inverse(simd_mul(projection,view)),simd_make_float4(camera,6000),{weatherTerrain.bounds[0],weatherTerrain.bounds[1],weatherTerrain.bounds[2],weatherTerrain.bounds[3]},w};
  e=[command computeCommandEncoder];[e setComputePipelineState:pipeline(@"density_checks")];[e setBytes:&u length:sizeof(u) atIndex:0];[e setBuffer:checks offset:0 atIndex:1];[e setTexture:local.terrain atIndex:1];[e setTexture:sky.shape atIndex:2];[e setTexture:sky.detail atIndex:3];[e setTexture:sky.weatherNoise atIndex:4];[e dispatchThreads:MTLSizeMake(4,1,1) threadsPerThreadgroup:MTLSizeMake(4,1,1)];[e endEncoding];
  auto draw=makeSkyFogDraw(u.worldFromClip,simd_make_float4(0,0,384,216),true);local.bind(nil,draw,{0,0,0});
  // Translating the game's coordinate origin must not move world-space weather.
  simd_float3 origin={13500,0,-7200};auto shift=matrix_identity_float4x4;shift.columns[3]=simd_make_float4(origin,1);
  auto rebased=makeSkyFogDraw(simd_mul(shift,u.worldFromClip),simd_make_float4(0,0,384,216),true);local.bind(nil,rebased,origin);
  SkyFogDraw draws[2]={draw,rebased};auto projected=[device newBufferWithLength:sizeof(simd_float4)*2 options:MTLResourceStorageModeShared];
  e=[command computeCommandEncoder];[e setComputePipelineState:pipeline(@"medium_reprojection")];[e setBuffer:projected offset:0 atIndex:0];[e setBytes:draws length:sizeof(draws) atIndex:1];[e setTexture:local.transport atIndex:0];[e dispatchThreads:MTLSizeMake(2,1,1) threadsPerThreadgroup:MTLSizeMake(2,1,1)];[e endEncoding];
  e=dispatch(pipeline(@"fixture_image"),image);[e setTexture:cloud atIndex:1];[e setTexture:sky.atmosphere atIndex:2];[e setTexture:sky.transmittance atIndex:3];[e setTexture:local.terrain atIndex:4];[e setTexture:local.transport atIndex:5];[e setBytes:&u length:sizeof(u) atIndex:0];[e setBytes:&draw length:sizeof(draw) atIndex:1];[e dispatchThreads:MTLSizeMake(384,216,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[e endEncoding];
  [command commit];[command waitUntilCompleted];if(command.error)return 7;
  auto reprojection=static_cast<simd_float4*>(projected.contents);
  if(simd_length(reprojection[0]-reprojection[1])>.001f)return 11;
  auto result=static_cast<float*>(checks.contents);std::cout<<"densities "<<result[0]<<" "<<result[1]<<" "<<result[2]<<" "<<result[3]<<"\n";if(result[0]!=0||result[1]>1.e-7f||result[3]!=0||(scenario!=1&&result[2]<=0))return 8;
  std::vector<uint8_t>bytes(384*216*4);[image getBytes:bytes.data() bytesPerRow:384*4 fromRegion:MTLRegionMake2D(0,0,384,216) mipmapLevel:0];std::ofstream file(std::string(outputDirectory)+"/weather-"+std::to_string(scenario)+".ppm",std::ios::binary);file<<"P6\n384 216\n255\n";for(size_t j=0;j<bytes.size();j+=4)file.write(reinterpret_cast<char*>(bytes.data()+j),3);
 }
 local.current=true;auto command=[queue commandBuffer];DynamicSkyUniform w{};local.encode(device,lib,command,4,w,view,projection,{0,0,0},sky.shape,sky.detail,sky.weatherNoise,sky.atmosphere,sky.solarSamples[0],false);if(local.current)return 9;
 auto oldRevision=weatherTerrain.revision;weatherTerrain.clear(&faces);if(!weatherTerrain.heights.empty()||weatherTerrain.revision!=oldRevision+1)return 10;
 std::cout<<"PASS: three shells, ground exclusion, wet orographic volume, indoor exclusion, terrain teardown\n";return 0;
}}
