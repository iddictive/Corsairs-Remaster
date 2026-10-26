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
float2 phonkNuclearField(float3 q,float age,texture3d<float> noise) {
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
    float stemRadius=(.10f+growth*.17f)*(1.f+.35f*sin(q.y*8.f-age*.9f));
    float2 stemCenter=float2(sin(q.y*3.2f-age*.27f),cos(q.y*4.1f-age*.21f))*.075f*growth;
    float stem=max(length(q.xz-stemCenter)-stemRadius,max(-q.y-.04f,q.y-height));
    float ball=length(q-float3(0,.2f*growth,0))-(.08f+growth*.65f);
    float cloud=min(min(head,curl),min(stem,ball+smoothstep(.6f,2.5f,age)*1.6f));
    float density=1.f-smoothstep(-.09f,.055f,cloud+churn);
    density*=.32f+.98f*smoothstep(.28f,.70f,coarse*.30f+medium*.48f+fine*.22f);
    density*=smoothstep(0.f,.16f,age)*(1.f-smoothstep(6.f,9.f,age));
    float core=1.f-smoothstep(-.32f,.025f,cloud+churn*.45f);
    float heat=exp(-age*.53f)*core*(.52f+coarse*.48f);
    return float2(density,heat);
}
// Integrate a thin pressure shell analytically: a world-space hemisphere,
// clipped by both opaque scene depth and the ground, without extra march taps.
float3 phonkNuclearPressure(float3 origin,float3 ray,float stop,float age,
                           thread float2 &distances,thread float2 &opacity) {
    distances=float2(1.e8f);opacity=0.f;
    float fade=smoothstep(.04f,.14f,age)*(1.f-smoothstep(1.4f,3.f,age));
    if(fade<=0.f)return 0.f;
    float front=min(3.f,age*1.1f),width=.025f+age*.012f;
    float along=dot(origin,ray),across=dot(origin,origin)-along*along;
    float outer=(front+width)*(front+width)-across;
    if(outer<=0.f)return 0.f;
    float inner=sqrt(max(0.f,(front-width)*(front-width)-across));
    float edge=sqrt(outer),low=0.f,high=stop;
    if(abs(ray.y)<1.e-7f) { if(origin.y<.025f)return 0.f; }
    else {
        float ground=(.025f-origin.y)/ray.y;
        if(ray.y>0.f)low=max(low,ground);else high=min(high,ground);
    }
    float2 first=max(float2(-along-edge,-along+inner),float2(low));
    float2 last=min(float2(-along-inner,-along+edge),float2(high));
    float2 thickness=max(0.f,last-first);
    opacity=(1.f-exp(-min(thickness/width,float2(4.f))*.10f))*fade;
    distances=(first+last)*.5f;
    float hot=exp(-age*2.f);
    float3 tint=mix(float3(.86f,1.05f,1.20f),float3(1.6f,1.15f,.65f),hot);
    return tint;
}
fragment float4 phonk_nuclear_fs(float4 pixel [[position]],constant PhonkNuclearUniforms &u [[buffer(0)]],
                                depth2d<float,access::read> depth [[texture(0)]],texture3d<float> noise [[texture(1)]],
                                texture2d<float> smokeSprite [[texture(2)]]) {
    uint2 location=min(uint2(pixel.xy),uint2(depth.get_width()-1,depth.get_height()-1));
    float sceneDepth=depth.read(location);
    float3 ray=normalize(phonkNuclearUnproject(pixel.xy,1.f,u)-u.eye.xyz);
    float stop=sceneDepth<1.f?max(0.f,dot(phonkNuclearUnproject(pixel.xy,sceneDepth,u)-u.eye.xyz,ray)):1.e8f;
    float radius=u.sourceRadius.w;
    float3 origin=(u.eye.xyz-u.sourceRadius.xyz)/radius;
    float2 pressureDistances,pressureOpacity;
    float3 pressureColor=phonkNuclearPressure(origin,ray,stop/radius,u.params.x,
        pressureDistances,pressureOpacity);
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
    // A vertical world-space smoke card: scene depth occludes it like the
    // plume. Horizontal facing preserves its readable cap around the blast.
    float4 sprite=0;
    float age=u.params.x;
    float faceAmount=0.f; // Facial smoke variant disabled; keep the procedural mushroom.
    float planeAngle=dot(ray.xz,u.params.yz);
    faceAmount*=smoothstep(.05f,.15f,abs(planeAngle));
    if(abs(planeAngle)>.001f && faceAmount>0.f) {
        float distance=-dot(origin.xz,u.params.yz)/planeAngle;
        if(distance>0.f && distance<stop/radius) {
            float3 point=origin+ray*distance;
            float size=2.6f*(.45f+.55f*(1.f-exp(-age*1.8f)));
            float2 uv=float2(dot(point.xz,float2(u.params.z,-u.params.y))/size+.5f,
                             1.f-point.y/size);
            float3 flow=float3(uv*7.f,age*.18f);
            float2 eddy=float2(phonkNuclearNoise(flow,noise),
                phonkNuclearNoise(flow+float3(8,3,5),noise))-.5f;
            uv+=eddy*(.012f+smoothstep(5.5f,8.5f,age)*.06f);
            if(all(uv>=0.f) && all(uv<=1.f)) {
                constexpr sampler image(coord::normalized,address::clamp_to_zero,filter::linear);
                sprite=smokeSprite.sample(image,uv);
                sprite*=1.f-smoothstep(6.f,9.f,age);
            }
        }
    }
    float3 inverse=1.f/select(ray,float3(1.e-7f),abs(ray)<1.e-7f);
    float3 first=(float3(-1.6f,-.25f,-1.6f)-origin)*inverse;
    float3 last=(float3(1.6f,3.0f,1.6f)-origin)*inverse;
    float near=max(0.f,max(max(min(first,last).x,min(first,last).y),min(first,last).z));
    float far=min(stop/radius,min(min(max(first,last).x,max(first,last).y),max(first,last).z));
    if(far<=near) {
        float alpha=1.f-(1.f-pressureOpacity.x)*(1.f-pressureOpacity.y);
        float4 pressure=float4(pressureColor*alpha,alpha);
        return mix(pressure+ground*(1.f-alpha),sprite+ground*(1.f-sprite.a),faceAmount);
    }
    constexpr uint steps=64;
    float step=(far-near)/float(steps),transmission=1.f;
    float3 color=0;
    uint pressureLayer=any(pressureOpacity>0.f)?0:2;
    for(uint i=0;i<steps;i++) {
        float distance=near+(float(i)+.5f)*step;
        if(pressureLayer<2 && pressureDistances[pressureLayer]<=distance) {
            for(uint layer=0;layer<2;layer++) {
                if(pressureLayer>=2 || pressureDistances[pressureLayer]>distance)break;
                float alpha=pressureOpacity[pressureLayer++];
                color+=transmission*pressureColor*alpha;
                transmission*=1.f-alpha;
            }
        }
        float3 point=origin+ray*distance;
        float2 field=phonkNuclearField(point,u.params.x,noise);
        float opacity=1.f-exp(-field.x*step*6.f);
        if(opacity>.001f) {
            // Two bounded optical-depth taps, following the volume-fire model.
            float3 light=float3(-.48f,.81f,-.32f);
            float shadow=phonkNuclearField(point+light*.18f,u.params.x,noise).x*.18f
                        +phonkNuclearField(point+light*.48f,u.params.x,noise).x*.30f;
            float lighting=.20f+.66f*exp(-shadow*3.5f)+.14f*exp(-shadow*.9f);
            float3 soot=float3(.30f,.285f,.26f)*lighting;
            float fire=smoothstep(.04f,.65f,field.y);
            float3 emission=mix(float3(1.8f,.19f,.014f),float3(4.4f,2.5f,.7f),fire);
            color+=transmission*opacity*(soot+emission*pow(field.y,1.7f)*3.f);
        }
        transmission*=1.f-opacity;
        if(transmission<.015f)break;
    }
    for(uint layer=pressureLayer;layer<2;layer++) {
        color+=transmission*pressureColor*pressureOpacity[layer];
        transmission*=1.f-pressureOpacity[layer];
    }
    float4 volume=float4(color+ground.rgb*transmission,1.f-transmission*(1.f-ground.a));
    return mix(volume,sprite+ground*(1.f-sprite.a),faceAmount);
}
)MSL";
}
