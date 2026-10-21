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
// Advected, nested density scales roll around the rising cap instead of
// translating a texture through a fixed cylinder. No per-frame random seed.
float phonkNuclearNoise(float3 p,texture3d<float> noise) {
    constexpr sampler lattice(coord::normalized,address::repeat,filter::linear);
    float3 cell=floor(p),f=fract(p);f=f*f*(3.f-2.f*f);
    return noise.sample(lattice,(cell+f+.5f)/64.f).r;
}
float3 phonkNuclearField(float3 q,float age,float2 facing,texture3d<float> noise,texture3d<float> headShape) {
    float growth=1.f-exp(-age*2.4f),rise=1.8f*(1.f-exp(-age*.20f));
    float height=.26f+rise,spread=.10f+growth*1.05f;
    float radial=length(q.xz),capY=q.y-height;
    float rollRadius=spread*.70f;
    float2 roll=float2(radial-rollRadius,capY);
    float turn=age*.55f;
    float2 rolled=float2(cos(turn)*roll.x-sin(turn)*roll.y,sin(turn)*roll.x+cos(turn)*roll.y);
    float3 flow=float3(q.x,q.y-age*.19f,q.z);
    float rollWeight=exp(-dot(roll,roll)*2.f);
    flow.y+=rollWeight*(rolled.y-roll.y);
    flow.xz*=1.f+rollWeight*(rolled.x-roll.x)/max(radial,.15f);
    float coarse=phonkNuclearNoise(flow*4.1f+float3(8,0,3),noise);
    float medium=phonkNuclearNoise(flow*9.3f+float3(1,7,11),noise);
    float fine=phonkNuclearNoise(flow*21.7f+float3(13,2,5),noise);
    float churn=(coarse-.5f)*.38f+(medium-.5f)*.17f+(fine-.5f)*.065f;
    float2 axis=q.xz/max(radial,.001f);
    float2 axis2=float2(axis.x*axis.x-axis.y*axis.y,2.f*axis.x*axis.y);
    float2 axis4=float2(axis2.x*axis2.x-axis2.y*axis2.y,2.f*axis2.x*axis2.y);
    float2 axis8=float2(axis4.x*axis4.x-axis4.y*axis4.y,2.f*axis4.x*axis4.y);
    float2 axis9=float2(axis8.x*axis.x-axis8.y*axis.y,axis8.x*axis.y+axis8.y*axis.x);
    float2 axis13=float2(axis9.x*axis4.x-axis9.y*axis4.y,axis9.x*axis4.y+axis9.y*axis4.x);
    float lobes=dot(axis9,float2(.0678f,.0990f))+dot(axis13,float2(.0694f,-.0090f));
    float head=(length(float3(q.x/spread,capY/(.12f+growth*.65f),q.z/spread))-1.f)*spread;
    float curl=length(float2(radial-rollRadius,capY+.13f*growth))-(.08f+growth*.39f)*(1.f+lobes);
    // The face is smoke geometry, sharing extinction, turbulence and light taps
    // with the plume. It coalesces after ignition, then disperses into the cap.
    float faceAmount=smoothstep(.35f,1.35f,age)*(1.f-smoothstep(5.5f,8.5f,age));
    float3 face=float3(dot(q.xz,float2(facing.y,-facing.x))/spread,
                      capY/(.12f+growth*.78f),dot(q.xz,facing)/(spread*1.15f));
    constexpr sampler shapeSampler(coord::normalized,address::clamp_to_edge,filter::linear);
    float sculpt=(headShape.sample(shapeSampler,face/2.2f+.5f).r-.5f)*.5f*spread*.8f;
    sculpt=max(sculpt,(max(max(abs(face.x),abs(face.y)),abs(face.z))-1.1f)*spread*.8f);
    float facialFront=smoothstep(.05f,.45f,face.z)
        *(1.f-smoothstep(.70f,1.f,abs(face.x)))*(1.f-smoothstep(.65f,.95f,abs(face.y)));
    // Keep the mushroom as the enclosing volume: the facial relief emerges
    // from its front instead of exposing a separate closed head silhouette.
    float3 capPoint=q-float3(facing.x*.48f,0,facing.y*.48f)*-1.f;
    float capBack=(length(float3(capPoint.x/spread,capY/(.18f+growth*.78f),capPoint.z/spread))-1.f)*spread;
    float blend=clamp(.5f+.5f*(capBack-sculpt)/.24f,0.f,1.f);
    float joined=mix(capBack,sculpt,blend)-.24f*blend*(1.f-blend);
    head=mix(head,joined,faceAmount);
    curl=mix(curl,max(curl,.4f),facialFront*faceAmount);
    // Keep landmark-scale relief, but break the surface into moving smoke.
    float landmarks=1.f-smoothstep(.18f,.45f,face.y);
    churn*=mix(1.f,.20f,facialFront*faceAmount*landmarks);
    float stemRadius=(.10f+growth*.17f)*(1.f+.35f*sin(q.y*8.f-age*.9f));
    float2 stemCenter=float2(sin(q.y*3.2f-age*.27f),cos(q.y*4.1f-age*.21f))*.075f*growth;
    float stem=max(length(q.xz-stemCenter)-stemRadius,max(-q.y-.04f,q.y-height));
    float ball=length(q-float3(0,.2f*growth,0))-(.08f+growth*.65f);
    float cloud=min(min(head,curl),min(stem,ball+smoothstep(.6f,2.5f,age)*1.6f));
    float density=1.f-smoothstep(-.09f,.055f,cloud+churn);
    density*=.32f+.98f*smoothstep(.28f,.70f,coarse*.30f+medium*.48f+fine*.22f);
    float headVolume=faceAmount*(1.f-smoothstep(0.f,.15f,sculpt));
    density*=1.f-headVolume*.50f;
    density*=smoothstep(0.f,.16f,age)*(1.f-smoothstep(6.f,9.f,age));
    float core=1.f-smoothstep(-.32f,.025f,cloud+churn*.45f);
    float heat=exp(-age*.53f)*core*(.52f+coarse*.48f);
    heat*=1.f-headVolume;
    return float3(density,heat,headVolume);
}
fragment float4 phonk_nuclear_fs(float4 pixel [[position]],constant PhonkNuclearUniforms &u [[buffer(0)]],
                                depth2d<float,access::read> depth [[texture(0)]],texture3d<float> noise [[texture(1)]],
                                texture3d<float> headShape [[texture(2)]]) {
    uint2 location=min(uint2(pixel.xy),uint2(depth.get_width()-1,depth.get_height()-1));
    float sceneDepth=depth.read(location);
    float3 ray=normalize(phonkNuclearUnproject(pixel.xy,1.f,u)-u.eye.xyz);
    float stop=sceneDepth<1.f?max(0.f,dot(phonkNuclearUnproject(pixel.xy,sceneDepth,u)-u.eye.xyz,ray)):1.e8f;
    float radius=u.sourceRadius.w;
    float3 origin=(u.eye.xyz-u.sourceRadius.xyz)/radius;
    // The thin ground front is analytic, so its wide footprint does not force
    // empty cloud samples along every ray through the shock ring.
    float4 ground=0;
    float groundDistance=(.025f-origin.y)/select(ray.y,1.e-7f,abs(ray.y)<1.e-7f);
    if(groundDistance>0.f && groundDistance<stop/radius) {
        float3 at=origin+ray*groundDistance;
        float ring=abs(length(at.xz)-min(3.f,u.params.x*1.1f));
        float alpha=(1.f-smoothstep(.025f+u.params.x*.018f,.095f+u.params.x*.018f,ring))
                    *exp(-u.params.x*.72f)*smoothstep(0.f,.16f,u.params.x)*.55f;
        float hot=exp(-u.params.x*1.3f);
        ground=float4(mix(float3(.14f,.13f,.12f),float3(2.f,.50f,.08f),hot)*alpha,alpha);
    }
    float3 inverse=1.f/select(ray,float3(1.e-7f),abs(ray)<1.e-7f);
    float3 first=(float3(-1.6f,-.25f,-1.6f)-origin)*inverse;
    float3 last=(float3(1.6f,3.0f,1.6f)-origin)*inverse;
    float near=max(0.f,max(max(min(first,last).x,min(first,last).y),min(first,last).z));
    float far=min(stop/radius,min(min(max(first,last).x,max(first,last).y),max(first,last).z));
    if(far<=near)return ground;
    constexpr uint steps=64;
    float step=(far-near)/float(steps),transmission=1.f;
    float3 color=0;
    for(uint i=0;i<steps;i++) {
        float distance=near+(float(i)+.5f)*step;
        float3 point=origin+ray*distance;
        float3 field=phonkNuclearField(point,u.params.x,u.params.yz,noise,headShape);
        float opacity=1.f-exp(-field.x*step*6.f);
        if(opacity>.001f) {
            // Two bounded optical-depth taps, following the volume-fire model.
            float2 lightXZ=u.params.yz*.58f-float2(u.params.z,-u.params.y)*.48f;
            float3 light=float3(lightXZ.x,.65f,lightXZ.y);
            float shadow=phonkNuclearField(point+light*mix(.18f,.04f,field.z),u.params.x,u.params.yz,noise,headShape).x*.18f
                        +phonkNuclearField(point+light*mix(.48f,.10f,field.z),u.params.x,u.params.yz,noise,headShape).x*.30f;
            float lighting=.20f+.66f*exp(-shadow*3.5f)+.14f*exp(-shadow*.9f);
            float3 soot=float3(.30f,.285f,.26f)*lighting;
            float fire=smoothstep(.04f,.65f,field.y);
            float3 emission=mix(float3(1.8f,.19f,.014f),float3(4.4f,2.5f,.7f),fire);
            color+=transmission*opacity*(soot+emission*pow(field.y,1.7f)*3.f);
        }
        transmission*=1.f-opacity;
        if(transmission<.015f)break;
    }
    return float4(color+ground.rgb*transmission,1.f-transmission*(1.f-ground.a));
}
)MSL";
}
