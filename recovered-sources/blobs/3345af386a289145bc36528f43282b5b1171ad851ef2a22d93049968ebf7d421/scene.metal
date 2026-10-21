#include <metal_stdlib>
using namespace metal;
struct Vertex { packed_float3 position; packed_float3 normal; packed_float2 uv; };
struct Uniform { float4 centerScale; float4 view; float4 options; };
struct Raster { float4 position [[position]]; float2 uv; float3 normal; float3 world; };
vertex Raster vs(uint index [[vertex_id]], const device Vertex *vertices [[buffer(0)]], constant Uniform &u [[buffer(1)]]) {
    Vertex v = vertices[index];
    float3 world = v.position;
    if (u.options.y > 0.5) world.y += .18*sin(world.x*.13+u.options.x)+.12*cos(world.z*.19-u.options.x*.8);
    float3 p = (world-u.centerScale.xyz)*u.centerScale.w;
    float c=cos(u.view.x), s=sin(u.view.x), cp=cos(u.view.z), sp=sin(u.view.z);
    float3 q=float3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);
    float y=q.y*cp-q.z*sp, z=q.y*sp+q.z*cp;
    float d=u.view.w-z;
    Raster out;
    out.position=float4(q.x*2.1/u.view.y,y*2.1,(d-.05)*40./39.95,d);
    out.uv=v.uv; out.normal=v.normal; out.world=world;
    return out;
}
fragment float4 fs(Raster in [[stage_in]], texture2d<float> texture [[texture(0)]], constant Uniform &u [[buffer(1)]]) {
    constexpr sampler linearRepeat(address::repeat,filter::linear);
    if(u.options.y > .5) {
        float time=u.options.x;
        float wave=sin(in.world.x*.13+time)*cos(in.world.z*.17-time*.7);
        float3 n=normalize(float3(-.18*.13*cos(in.world.x*.13+time),1,.12*.19*sin(in.world.z*.19-time*.8)));
        float sparkle=pow(max(0.,dot(n,normalize(float3(.05,1,.04)))),80.);
        float3 color=mix(float3(.025,.19,.25),float3(.08,.38,.43),.5+.5*wave);
        return float4(color+float3(.12,.15,.15)*sparkle,1);
    }
    float4 color=texture.sample(linearRepeat,in.uv);
    if(color.a<.2) discard_fragment();
    float light=.48+.52*abs(dot(normalize(in.normal),normalize(float3(.4,.8,.5))));
    return float4(color.rgb*light,1);
}
