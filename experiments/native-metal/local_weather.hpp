#pragma once
#import <Metal/Metal.h>
#include "dynamic_sky.hpp"
#include "sky_fog.hpp"
#include "weather_terrain.hpp"

namespace storm_metal {
struct LocalWeatherUniform {
    simd_float4x4 worldFromClip;
    simd_float4 cameraRange,terrainBounds;
    DynamicSkyUniform weather;
    simd_float4 angles;
};
inline const char* localWeatherMSL=R"MSL(
struct LocalWeatherUniform {float4x4 worldFromClip;float4 cameraRange,terrainBounds;SkyWeather weather;float4 angles;};
float localTerrainHeight(float2 p,texture2d<float>terrain,float4 bounds){
    if(bounds.z<=0||bounds.w<=0)return 0;
    float2 uv=(p-bounds.xy)/bounds.zw;
    if(any(uv<0)||any(uv>1))return 0;
    constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);
    float2 size=float2(terrain.get_width(),terrain.get_height());
    return terrain.sample(s,(uv*(size-1.f)+.5f)/size,level(0)).r;
}
float localCloudDensity(float3 p,constant LocalWeatherUniform&u,texture2d<float>terrain,texture3d<float>shape,texture3d<float>detail,texture2d<float>regional,float footprint=0.f){
    float ground=localTerrainHeight(p.xz,terrain,u.terrainBounds);
    if(p.y<=ground+2.f)return 0;
    float wet=smoothstep(.36f,.86f,u.weather.wind_coverage_density.z);
    float2 wind=u.weather.wind_coverage_density.xy;
    float speed=length(wind);wind=speed>.1f?wind/speed:float2(1,0);
    float upwind=localTerrainHeight(p.xz-wind*240.f,terrain,u.terrainBounds);
    float uplift=saturate((ground-upwind)/240.f)*min(speed/8.f,1.5f);
    float2 phase=u.weather.options.yz;
    float3 macro=regional.sample(ns,(p.xz+phase*.4f)*.00018f+float2(.21f,.47f),level(0)).rgb;
    float morning=1.f-smoothstep(1.5f,4.f,abs(u.weather.sunDirection_hour.w-6.f));
    float moisture=clamp(.28f+wet*.36f+macro.b*.20f+morning*.08f,.15f,.92f);
    // A condensation level, not a surface decal. Moist wind rising on the
    // windward slope increases cloud formation; dry leeside air erodes it.
    float base=mix(950.f,150.f,wet)+180.f*(macro.r-.5f)-uplift*160.f;
    float thickness=mix(180.f,650.f,wet);
    float edge=max(80.f,footprint*.35f);
    float envelope=smoothstep(base-edge,base+edge,p.y)*(1.f-smoothstep(base+thickness-edge,base+thickness+edge,p.y));
    float mountain=smoothstep(120.f,700.f,ground)*(wet*.65f+uplift*.6f);
    float slopeCloud=mountain*exp(-pow((p.y-ground-110.f)/160.f,2.f));
    float fogEvent=smoothstep(.72f,.88f,macro.b+morning*.20f+wet*.22f);
    float fog=fogEvent*(.2f+.8f*wet)*exp(-max(0.f,p.y-ground-8.f)/mix(32.f,100.f,wet));
    float3 q=p+float3(phase.x*.65f,0,phase.y*.65f);
    float lod=max(0.f,log2(max(1.f,footprint*.0012f*128.f)));
    float4 n=shape.sample(ns,q*float3(.0006f,.0012f,.0006f),level(lod));
    float cell=remap(n.r,-(1.f-dot(n.gba,float3(.625f,.25f,.125f))),1.f,0.f,1.f);
    float erosion=detail.sample(ns,(q-float3(phase.x,0,phase.y)*2.f)*.007f,level(max(1.f,log2(max(1.f,footprint*.007f*32.f))))).r;
    float cloud=saturate((cell-(1.f-moisture)-erosion*.16f)/moisture)*moisture;
    float clearance=smoothstep(2.f,18.f,p.y-ground);
    return u.weather.wind_coverage_density.w*clearance*(cloud*max(envelope,slopeCloud)*.0018f+fog*(.6f+.4f*cell)*.0012f);
}
float localWeatherDistance(float z,float range){return 8.f*(exp(z*log(1.f+range/8.f))-1.f);}
kernel void local_weather_segments(texture3d<float,access::write>output [[texture(0)]],texture2d<float>terrain [[texture(1)]],texture3d<float>shape [[texture(2)]],texture3d<float>detail [[texture(3)]],texture2d<float>regional [[texture(4)]],texture2d<float>atmosphere [[texture(5)]],constant LocalWeatherUniform&u [[buffer(0)]],constant float4*lighting [[buffer(1)]],uint3 id [[thread_position_in_grid]]){
    if(id.x>=output.get_width()||id.y>=output.get_height()||id.z>=output.get_depth())return;
    float phi=u.angles.x+float(id.x)*u.angles.y;
    float theta=skyLocalTheta(float(id.y)/float(output.get_height()-1));
    float3 ray=float3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
    float a=localWeatherDistance(float(id.z)/128.f,u.cameraRange.w),b=localWeatherDistance(float(id.z+1)/128.f,u.cameraRange.w);
    float3 p=u.cameraRange.xyz+ray*(a+b)*.5f;
    // Two quadrature samples and noise mip filtering integrate thin wisps
    // across a froxel instead of turning a single hit/miss into a flat cut.
    float footprint=(b-a)*.5f;
    float extinction=.5f*(localCloudDensity(u.cameraRange.xyz+ray*mix(a,b,.25f),u,terrain,shape,detail,regional,footprint)+localCloudDensity(u.cameraRange.xyz+ray*mix(a,b,.75f),u,terrain,shape,detail,regional,footprint));
    if(extinction<1.e-7f){output.write(float4(0,0,0,1),id);return;}
    float transmission=exp(-extinction*(b-a));
    float3 sun=normalize(u.weather.sunDirection_hour.xyz);
    float shadow=localCloudDensity(p+sun*80.f,u,terrain,shape,detail,regional)*80.f;
    shadow+=localCloudDensity(p+sun*240.f,u,terrain,shape,detail,regional)*160.f;
    float ground=localTerrainHeight((p+sun*240.f).xz,terrain,u.terrainBounds);
    float visibility=exp(-shadow)*smoothstep(ground,ground+20.f,p.y+sun.y*240.f);
    SkyAirField air=skyAirField(ray,u.weather,atmosphere);
    float phase=henyey_greenstein(dot(ray,sun),.35f);
    // Consume the existing sky incident-light packet; low clouds have no
    // separate ambient floor or weather-independent self illumination.
    float3 diffuse=max(lighting[1].rgb+lighting[3].rgb*.35f,float3(0));
    float3 source=diffuse+lighting[0].yzw*lighting[0].x*visibility*phase*mix(.06f,2.5f,air.daylight);
    output.write(float4(source*(1.f-transmission),transmission),id);
}
kernel void local_weather_integrate(texture3d<float>segments [[texture(0)]],texture3d<float,access::write>output [[texture(1)]],uint2 id [[thread_position_in_grid]]){
    if(id.x>=output.get_width()||id.y>=output.get_height())return;
    float4 transport=float4(0,0,0,1);output.write(transport,uint3(id,0));
    for(uint z=0;z<128;z++){
        float4 segment=segments.read(uint3(id,z));
        transport.rgb+=transport.a*segment.rgb;transport.a*=segment.a;
        output.write(transport,uint3(id,z+1));
    }
}
)MSL";

struct LocalWeatherVolume {
    static constexpr unsigned height=72;
    static constexpr float angleStep=6.28318530718f/512.f;
    static constexpr unsigned width=513;
    simd_float4 angles{};
    id<MTLTexture> terrain=nil,segments=nil,transport=nil;
    id<MTLComputePipelineState> segmentKernel=nil,integrateKernel=nil;
    uint64_t terrainRevision=~uint64_t(0);
    bool current=false;
    uint64_t viewFrame=~uint64_t(0);
    simd_float4x4 viewProjection{};
    simd_float3 viewOrigin{};
    simd_float4x4 clipFromWorld=matrix_identity_float4x4;
    simd_float4 cameraRange{};
    id<MTLCommandBuffer> computeCommand=nil;
    NSUInteger computeLightingOffset=0;
    std::array<id,12> computeResources{};
    LocalWeatherUniform computeUniform{};
    bool matches(uint64_t frame,simd_float4x4 view,simd_float4x4 projection,simd_float3 origin)const{
        const auto vp=simd_mul(projection,view);
        return current&&viewFrame==frame&&terrainRevision==weatherTerrain.revision&&
            simd_all(origin==viewOrigin)&&std::memcmp(&vp,&viewProjection,sizeof(vp))==0;
    }
    static simd_float4 angularWindow(simd_float4x4 view,simd_float4x4 projection){
        // A projected frustum is a convex cone. Its azimuth extrema lie on
        // corner rays unless it contains a pole, which needs the whole ring.
        const auto vp=simd_mul(projection,view),inverse=simd_inverse(vp);
        const auto camera=simd_inverse(view).columns[3].xyz;
        for(float y:{-1.f,1.f}){
            auto clip=simd_mul(vp,simd_make_float4(0,y,0,0));
            if(clip.w>0&&std::abs(clip.x)<=clip.w&&std::abs(clip.y)<=clip.w)
                return {0,angleStep,513,0};
        }
        float phi[4];unsigned n=0;
        for(float y:{-1.f,1.f})for(float x:{-1.f,1.f}){
            auto near=simd_mul(inverse,simd_make_float4(x,y,0,1));
            auto ray=near.xyz/near.w-camera;
            float a=std::atan2(ray.z,ray.x);phi[n++]=a<0?a+6.28318530718f:a;
        }
        std::sort(phi,phi+4);float gap=-1,start=0;
        for(unsigned i=0;i<4;i++){
            float next=i==3?phi[0]+6.28318530718f:phi[i+1];
            if(next-phi[i]>gap){gap=next-phi[i];start=next;}
        }
        const float first=std::floor(start/angleStep)-1.f;
        const unsigned count=std::min(513u,unsigned(std::ceil((start+6.28318530718f-gap)/angleStep)-first)+2u);
        return {std::fmod(first*angleStep,6.28318530718f),angleStep,float(count),0};
    }
    bool prepare(id<MTLDevice>device,id<MTLLibrary>library){
        if(segmentKernel)return true;
        NSError*error=nil;
        segmentKernel=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"local_weather_segments"] error:&error];
        integrateKernel=[device newComputePipelineStateWithFunction:[library newFunctionWithName:@"local_weather_integrate"] error:&error];
        // Reserve the pole view once; dispatch only the current angular window.
        // Camera motion and reflection faces must not churn in-flight textures.
        auto volume=[&](unsigned depth){auto d=[MTLTextureDescriptor new];d.textureType=MTLTextureType3D;d.pixelFormat=MTLPixelFormatRGBA16Float;d.width=width;d.height=height;d.depth=depth;d.storageMode=MTLStorageModePrivate;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;return [device newTextureWithDescriptor:d];};
        segments=volume(128);transport=volume(129);
        auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float width:WeatherTerrain::side height:WeatherTerrain::side mipmapped:NO];d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead;terrain=[device newTextureWithDescriptor:d];
        if(!segmentKernel||!integrateKernel||!segments||!transport||!terrain){*this={};return false;}
        return true;
    }
    bool encode(id<MTLDevice>device,id<MTLLibrary>library,id<MTLCommandBuffer>command,uint64_t frame,const DynamicSkyUniform&weather,simd_float4x4 view,simd_float4x4 projection,simd_float3 origin,id<MTLTexture>shape,id<MTLTexture>detail,id<MTLTexture>regional,id<MTLTexture>atmosphere,id<MTLBuffer>lighting,bool outdoor,NSUInteger lightingOffset=0){
        auto vp=simd_mul(projection,view);
        const bool reusable=current&&viewFrame==frame&&terrainRevision==weatherTerrain.revision&&command&&computeCommand==command;
        current=false;if(!outdoor)return true;
        auto inverse=simd_inverse(vp);
        if(!std::isfinite(simd_determinant(inverse)))return true;
        if(!prepare(device,library))return false;
        angles=angularWindow(view,projection);
        if(terrainRevision!=weatherTerrain.revision){
            // Replace immutable upload storage, never overwrite bytes referenced
            // by an older in-flight scene after an island load/unload.
            auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float width:WeatherTerrain::side height:WeatherTerrain::side mipmapped:NO];d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead;
            terrain=[device newTextureWithDescriptor:d];
            const std::vector<float>empty(weatherTerrain.heights.empty()?WeatherTerrain::side*WeatherTerrain::side:0,0.f);
            [terrain replaceRegion:MTLRegionMake2D(0,0,WeatherTerrain::side,WeatherTerrain::side) mipmapLevel:0 withBytes:weatherTerrain.heights.empty()?empty.data():weatherTerrain.heights.data() bytesPerRow:WeatherTerrain::side*sizeof(float)];
            terrainRevision=weatherTerrain.revision;
        }
        auto absolute=matrix_identity_float4x4;absolute.columns[3]=simd_make_float4(-origin,1);
        LocalWeatherUniform u{};u.worldFromClip=inverse;
        u.angles=angles;
        cameraRange=simd_make_float4(simd_inverse(view).columns[3].xyz-origin,6000.f);
        u.cameraRange=cameraRange;u.weather=weather;
        if(!weatherTerrain.heights.empty())u.terrainBounds={weatherTerrain.bounds[0],weatherTerrain.bounds[1],weatherTerrain.bounds[2],weatherTerrain.bounds[3]};
        absolute.columns[3]=simd_make_float4(origin,1);clipFromWorld=simd_mul(vp,absolute);
        // Both compute kernels use world rays, not worldFromClip. A different
        // view can reuse the exact field, but its sampling metadata above must
        // still follow that view (including reflection -> main restoration).
        auto computeInput=u;computeInput.worldFromClip=matrix_identity_float4x4;
        const std::array<id,12> resources{device,library,segmentKernel,integrateKernel,terrain,segments,transport,shape,detail,regional,atmosphere,lighting};
        bool known=true;for(id resource:resources)known&=resource!=nil;
        for(auto value:{u.cameraRange,u.terrainBounds,u.angles,u.weather.sunDirection_hour,u.weather.moonDirection_time,u.weather.wind_coverage_density,u.weather.options,u.weather.horizonFog})for(unsigned axis=0;axis<4;++axis)known&=std::isfinite(value[axis]);
        if(!reusable||!known||resources!=computeResources||lightingOffset!=computeLightingOffset||std::memcmp(&computeInput,&computeUniform,sizeof(computeInput))!=0){
            auto e=[command computeCommandEncoder];if(!e)return false;e.label=@"Local weather segments";[e setComputePipelineState:segmentKernel];[e setBytes:&u length:sizeof(u) atIndex:0];
            [e setTexture:segments atIndex:0];[e setTexture:terrain atIndex:1];[e setTexture:shape atIndex:2];[e setTexture:detail atIndex:3];[e setTexture:regional atIndex:4];[e setTexture:atmosphere atIndex:5];[e setBuffer:lighting offset:lightingOffset atIndex:1];
            [e dispatchThreads:MTLSizeMake(unsigned(angles.z),height,128) threadsPerThreadgroup:MTLSizeMake(4,4,4)];[e endEncoding];
            e=[command computeCommandEncoder];if(!e)return false;e.label=@"Local weather integration";[e setComputePipelineState:integrateKernel];[e setTexture:segments atIndex:0];[e setTexture:transport atIndex:1];[e dispatchThreads:MTLSizeMake(unsigned(angles.z),height,1) threadsPerThreadgroup:MTLSizeMake(8,8,1)];[e endEncoding];
            computeCommand=command;computeResources=resources;computeUniform=computeInput;computeLightingOffset=lightingOffset;
        }
        viewFrame=frame;viewProjection=vp;viewOrigin=origin;current=true;return true;
    }
    void bind(id<MTLRenderCommandEncoder>encoder,SkyFogDraw&draw,simd_float3 origin)const{
        draw.options.z=current;auto absolute=matrix_identity_float4x4;absolute.columns[3]=simd_make_float4(-origin,1);
        draw.volumeClipFromWorld=simd_mul(clipFromWorld,absolute);draw.volumeCameraRange=simd_make_float4(cameraRange.xyz+origin,cameraRange.w);
        draw.volumeAngles=angles;
        [encoder setFragmentTexture:transport atIndex:27];
    }
};
}
