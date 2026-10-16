#pragma once
namespace storm_metal
{
inline constexpr const char *phonkNuclearMSL = R"MSL(
struct PhonkNuclearUniforms {
    float4x4 inverseViewProjection,viewProjection;
    float4 eye,sourceRadius,params,viewport;
};
vertex float4 phonk_nuclear_vs(uint index [[vertex_id]]) {
    constexpr float2 corners[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};
    return float4(corners[index],0,1);
}
float3 phonkNuclearUnproject(float2 pixel,float depth,constant PhonkNuclearUniforms &u) {
    float2 ndc=(pixel-u.viewport.xy)/u.viewport.zw*float2(2,-2)+float2(-1,1);
    float4 p=u.inverseViewProjection*float4(ndc,depth,1);
    return p.xyz/max(abs(p.w),1.e-8f)*sign(p.w);
}
float2 phonkNuclearField(float3 q,float age) {
    float growth=1.f-exp(-age*3.f),rise=min(1.8f,age*.24f);
    float3 flow=q*3.2f+float3(0,-age*.8f,0);
    float noise=shipFireNoise(flow)*.68f+shipFireNoise(flow*2.03f+9.1f)*.32f;
    float churn=(noise-.5f)*.23f;
    float ball=length((q-float3(0,.30f+rise*.32f,0))/float3(1,.86f,1))-(.1f+growth*.83f);
    float3 cap=q-float3(0,.42f+rise,0);
    float head=length(cap/float3(1.13f,.40f,1.13f))-(.18f+growth*.90f);
    float stem=max(length(q.xz)-(.15f+noise*.11f),max(-q.y,q.y-(.42f+rise)));
    float cloud=min(mix(ball,max(ball,1.f),smoothstep(1.f,3.f,age)),min(head,stem));
    float density=(1.f-smoothstep(-.08f,.16f,cloud+churn))*min(1.f,age*9.f);
    float ringRadius=min(3.f,age*1.1f);
    float ring=length(float2(length(q.xz)-ringRadius,(q.y-.025f)*4.f))-(.025f+age*.018f);
    density=max(density,(1.f-smoothstep(-.015f,.07f,ring+churn*.12f))*exp(-age*.72f));
    density*=1.f-smoothstep(6.f,9.f,age);
    float heat=exp(-age*.36f)*(.5f+.5f*noise);
    heat=max(heat,(1.f-smoothstep(-.01f,.04f,ring))*exp(-age*1.3f));
    return float2(density,heat);
}
fragment float4 phonk_nuclear_fs(float4 pixel [[position]],constant PhonkNuclearUniforms &u [[buffer(0)]],
                                depth2d<float,access::read> depth [[texture(0)]]) {
    uint2 location=min(uint2(pixel.xy),uint2(depth.get_width()-1,depth.get_height()-1));
    float sceneDepth=depth.read(location);
    float3 ray=normalize(phonkNuclearUnproject(pixel.xy,1.f,u)-u.eye.xyz);
    float stop=sceneDepth<1.f?max(0.f,dot(phonkNuclearUnproject(pixel.xy,sceneDepth,u)-u.eye.xyz,ray)):1.e8f;
    float radius=u.sourceRadius.w;
    float3 origin=(u.eye.xyz-u.sourceRadius.xyz)/radius;
    float3 inverse=1.f/select(ray,float3(1.e-7f),abs(ray)<1.e-7f);
    float3 first=(float3(-3.2f,-.25f,-3.2f)-origin)*inverse;
    float3 last=(float3(3.2f,3.7f,3.2f)-origin)*inverse;
    float near=max(0.f,max(max(min(first,last).x,min(first,last).y),min(first,last).z));
    float far=min(stop/radius,min(min(max(first,last).x,max(first,last).y),max(first,last).z));
    if(far<=near)discard_fragment();
    constexpr uint steps=32;
    float step=(far-near)/float(steps),transmission=1.f;
    float3 color=0;
    for(uint i=0;i<steps;i++) {
        float distance=near+(float(i)+.5f)*step;
        float2 field=phonkNuclearField(origin+ray*distance,u.params.x);
        float opacity=1.f-exp(-field.x*step*6.f);
        float fire=smoothstep(.10f,.62f,field.y);
        float3 soot=mix(float3(.075f,.060f,.050f),float3(.23f,.18f,.12f),field.y);
        float3 emission=mix(float3(1.5f,.13f,.015f),float3(3.7f,2.5f,1.05f),fire);
        color+=transmission*opacity*mix(soot,emission,smoothstep(.13f,.39f,field.y));
        transmission*=1.f-opacity;
        if(transmission<.015f)break;
    }
    return float4(color,1.f-transmission);
}
)MSL";
}
