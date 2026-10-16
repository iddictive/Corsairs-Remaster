#pragma once
#include "island_forest_bridge.hpp"
#include <string>
namespace storm_island_forest {
// This helper is the only deformation owner. Instanced color vertices and the
// bounded shadow expansion call it with the same frame/instance constants.
inline const std::string shaderSource=std::string("struct ForestVertex {")+
    STORM_FOREST_STRING(STORM_FOREST_VERTEX_FIELDS(packed_float3,packed_float2,uint))+"};\nstruct ForestInstance {"+
    STORM_FOREST_STRING(STORM_FOREST_INSTANCE_FIELDS(packed_float3))+R"MSL(};
struct ForestDeform {float4x4 world;float4 wind;uint vertexCount,instanceCount;uint2 padding;};
ForestVertex forestDeform(ForestVertex v,ForestInstance instance,constant ForestDeform&f){
 float sn=sin(instance.rotation),cs=cos(instance.rotation);float h=max(0.f,float(v.position.y));
 float3 p=float3(v.position)*instance.scale,n=float3(v.normal);
 p.xz=float2(p.x*cs+p.z*sn,-p.x*sn+p.z*cs);n.xz=float2(n.x*cs+n.z*sn,-n.x*sn+n.z*cs);
 float2 direction=f.wind.yz;float lengthSquared=dot(direction,direction);direction=lengthSquared>.0001f?direction*rsqrt(lengthSquared):float2(1,0);
 float gust=sin(f.wind.x*.83f+instance.phase)+.35f*sin(f.wind.x*1.71f+instance.phase*2.3f);
 float bend=.027f*clamp(f.wind.w,0.f,1.f)*gust;
 p.xz+=direction*(instance.scale*bend*h*h);
 n.y-=dot(n.xz,direction)*(2.f*bend*h);n=normalize(n);
 p+=float3(instance.root);v.position=packed_float3((f.world*float4(p,1)).xyz);v.normal=packed_float3(normalize((f.world*float4(n,0)).xyz));
 float4 tint=rawColor(v.color);tint.rgb=saturate(tint.rgb*instance.tint);tint.a=instance.alpha;
 uint4 bytes=uint4(round(tint*255.f));v.color=(bytes.a<<24)|(bytes.r<<16)|(bytes.g<<8)|bytes.b;return v;
}
vertex O island_forest_vs(uint vertexIndex [[vertex_id]],uint instance [[instance_id]],const device ForestVertex*mesh [[buffer(0)]],constant U&u [[buffer(1)]],constant ForestDeform&f [[buffer(2)]],constant RawLandU&r [[buffer(3)]],const device ForestInstance*instances [[buffer(5)]]){
 ForestVertex v=forestDeform(mesh[vertexIndex],instances[instance],f);
 return rawLitVertex(float3(v.position),float3(v.normal),rawColor(v.color),float4(0),float4(float2(v.uv),0,0),float4(0),u,r);
}
kernel void island_forest_expand(uint id [[thread_position_in_grid]],const device ForestVertex*mesh [[buffer(0)]],const device ForestInstance*instances [[buffer(1)]],constant ForestDeform&f [[buffer(2)]],device ForestVertex*out [[buffer(3)]],device uint*indices [[buffer(4)]]){
 if(id>=f.vertexCount*f.instanceCount)return;out[id]=forestDeform(mesh[id%f.vertexCount],instances[id/f.vertexCount],f);indices[id]=id;
}
)MSL";
}
