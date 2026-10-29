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
float2 phonkNuclearField(float3 q,float age,float4 variant,texture3d<float> noise) {
    // A detonation keeps its shape/advection identity throughout its lifetime.
    q.xz*=float2(.90f+variant.x*.20f,1.10f-variant.x*.20f);
    q.xz-=float2(variant.y-.5f,variant.z-.5f)*min(age,6.f)*.035f;
    float growth=1.f-exp(-age*(2.f+variant.y*.8f)),rise=1.8f*(1.f-exp(-age*(.17f+variant.z*.06f)));
    float height=.26f+rise,spread=.10f+growth*(.94f+variant.w*.11f);
    float radial=length(q.xz),capY=q.y-height;
    float rollRadius=spread*.70f;
    float2 roll=float2(radial-rollRadius,capY);
    float turn=age*(.40f+variant.x*.30f);
    float2 rolled=float2(cos(turn)*roll.x-sin(turn)*roll.y,sin(turn)*roll.x+cos(turn)*roll.y);
    float3 flow=float3(q.x,q.y-age*(.15f+variant.w*.08f),q.z);
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
    float lobes=dot(axis9,float2(variant.y-.5f,variant.z-.5f)*.24f)+dot(axis13,float2(variant.w-.5f,variant.x-.5f)*.14f);
    float head=(length(float3(q.x/spread,capY/(.12f+growth*(.53f+variant.z*.21f)),q.z/spread))-1.f)*spread;
    float curl=length(float2(radial-rollRadius,capY+.13f*growth))-(.08f+growth*.39f)*(1.f+lobes);
    float stemRadius=(.10f+growth*.17f)*(1.f+.35f*sin(q.y*8.f-age*.9f+variant.x*6.28f));
    float2 stemCenter=float2(sin(q.y*3.2f-age*.27f+variant.y*6.28f),cos(q.y*4.1f-age*.21f+variant.z*6.28f))*.075f*growth;
    float stem=max(length(q.xz-stemCenter)-stemRadius,max(-q.y-.04f,q.y-height));
    float ball=length(q-float3(0,.2f*growth,0))-(.08f+growth*.65f);
    float cloud=min(min(head,curl),min(stem,ball+smoothstep(.6f,2.5f,age)*1.6f));
    float density=1.f-smoothstep(-.09f,.055f,cloud+churn);
    density*=.32f+.98f*smoothstep(.28f,.70f,coarse*.30f+medium*.48f+fine*.22f);
    density*=smoothstep(0.f,.16f,age)*(1.f-smoothstep(6.f,9.f,age));
    float core=1.f-smoothstep(-.32f,.025f,cloud+churn*.45f);
    float heat=exp(-age*(.45f+variant.w*.16f))*core*(.52f+coarse*.48f);
    return float2(density,heat);
}
// The pressure front is a finite annular bank of condensation, not a decal.
// Its cross-section rolls behind the authoritative expanding leading edge.
float4 phonkPressureField(float3 p,float front,float width,float age,float4 variation,texture3d<float> noise) {
    float radial=length(p.xz),r=(radial-front)/width,h=p.y/width;
    float2 section=float2((r+.7f)/1.25f,(h-.85f)/1.05f);
    if(length(section)>1.4f)return 0;
    float turn=age*1.1f,cs=cos(turn),sn=sin(turn);
    float2 rolled=float2(cs*section.x-sn*section.y,sn*section.x+cs*section.y);
    float3 flow=p/(width*3.f)+variation.xyz*23.f+float3(0,-age*.2f,0);
    float coarse=phonkNuclearNoise(flow,noise);
    float detail=phonkNuclearNoise(flow*2.7f+float3(rolled*.4f,0),noise);
    float body=1.f-smoothstep(.78f,1.15f,length(section)+(coarse-.5f)*.32f+(detail-.5f)*.16f);
    float density=body*(.65f+.55f*coarse)*(.25f+.9f*smoothstep(.2f,.75f,detail))*1.4f;
    float2 axis=p.xz/max(radial,.001f);
    float3 normal=float3(axis.x*section.x/1.25f,section.y/1.05f,axis.y*section.x/1.25f);
    normal=length(normal)>1.e-5f?normalize(normal):float3(0,1,0);
    return float4(normal,density);
}
float2 phonkPressureCylinder(float3 origin,float3 ray,float radius) {
    float a=dot(ray.xz,ray.xz),b=dot(origin.xz,ray.xz),c=dot(origin.xz,origin.xz)-radius*radius;
    if(a<1.e-8f)return c<=0.f?float2(-1.e8f,1.e8f):float2(1,-1);
    float discriminant=b*b-a*c;
    if(discriminant<0.f)return float2(1,-1);
    float root=sqrt(discriminant);
    return float2(-b-root,-b+root)/a;
}
struct PhonkPressureSample {float4 haze,bend;};
PhonkPressureSample phonkPressureVolume(float3 origin,float3 ray,float stop,float front,float width,float age,
                          float4 variation,texture3d<float> noise) {
    float2 outer=phonkPressureCylinder(origin,ray,front+width*2.8f);
    float near=max(0.f,outer.x),far=min(stop,outer.y);
    if(abs(ray.y)<1.e-7f) {
        if(origin.y<-.1f*width||origin.y>3.f*width)return {};
    } else {
        float2 vertical=(float2(-.1f,3.f)*width-origin.y)/ray.y;
        near=max(near,min(vertical.x,vertical.y));far=min(far,max(vertical.x,vertical.y));
    }
    if(far<=near)return {};
    // Skip the empty center instead of spending a march across the full blast.
    float2 inner=front>width*2.8f?phonkPressureCylinder(origin,ray,front-width*2.8f):float2(far,far);
    float2 ranges[2]={float2(near,min(far,inner.x)),float2(max(near,inner.y),far)};
    if(inner.x>inner.y){ranges[0]=float2(near,far);ranges[1]=float2(far,far);}
    float transmission=1.f;float3 color=0;float4 bend=0;
    const float3 light=normalize(float3(-.48f,.81f,-.32f));
    for(uint section=0;section<2;section++) {
        float step=max(0.f,ranges[section].y-ranges[section].x)/6.f;
        if(step<=0.f)continue;
        for(uint i=0;i<6;i++) {
            float distance=ranges[section].x+(float(i)+.5f)*step;
            float3 point=origin+ray*distance;
            float4 field=phonkPressureField(point,front,width,age,variation,noise);
            float alpha=1.f-exp(-field.w*step/width*.12f);
            if(alpha>.001f) {
                float shadow=phonkPressureField(point+light*width*.9f,front,width,age,variation,noise).w;
                float3 normal=field.xyz;
                float shape=.4f+.6f*saturate(dot(normal,light)*.5f+.5f);
                float lighting=.28f+.75f*shape*exp(-shadow*.85f);
                bend+=transmission*alpha*float4(normal,distance);
                color+=transmission*float3(.74f,.79f,.81f)*lighting*alpha;
                transmission*=1.f-alpha;
            }
        }
    }
    float alpha=1.f-transmission,limit=alpha>.001f?min(alpha,.35f)/alpha:0.f;
    return {float4(color,alpha)*limit,bend*limit};
}
float4 phonkPressureComposite(float4 effect,float4 pressure,float2 pixel,float3 ray,
                              constant PhonkNuclearUniforms&u,depth2d<float,access::read> depth,
                              texture2d<float> scene) {
    if(pressure.w<=.001f)return effect;
    float3 position=u.eye.xyz+ray*pressure.w*u.sourceRadius.w;
    float4 center=u.viewProjection*float4(position,1);
    float4 displaced=u.viewProjection*float4(position+pressure.xyz*u.eye.w*u.sourceRadius.w*3.f,1);
    float2 offset=0;
    if(center.w>1.e-5f && displaced.w>1.e-5f)
        offset=(displaced.xy/displaced.w-center.xy/center.w)*float2(.5f,-.5f);
    offset*=min(1.f,.0125f/max(length(offset),1.e-5f));
    float2 uv=pixel/float2(scene.get_width(),scene.get_height());
    float2 shifted=clamp(uv+offset,float2(0),float2(1));
    uint2 at=min(uint2(shifted*float2(depth.get_width(),depth.get_height())),uint2(depth.get_width()-1,depth.get_height()-1));
    float shiftedDepth=depth.read(at),sourceDepth=depth.read(uint2(pixel));
    bool discontinuity=shiftedDepth<center.z/center.w;
    if(sourceDepth<1.f) {
        discontinuity|=shiftedDepth>=1.f;
        if(shiftedDepth<1.f) {
            float3 source=phonkNuclearUnproject(pixel,sourceDepth,u);
            float3 receiver=phonkNuclearUnproject(shifted*float2(depth.get_width(),depth.get_height()),shiftedDepth,u);
            discontinuity|=length(receiver-source)>u.eye.w*u.sourceRadius.w*8.f;
        }
    }
    if(discontinuity)shifted=uv;
    constexpr sampler image(coord::normalized,address::clamp_to_edge,filter::linear);
    return float4(effect.rgb+scene.sample(image,shifted).rgb*(1.f-effect.a),1);
}
fragment float4 phonk_nuclear_fs(float4 pixel [[position]],constant PhonkNuclearUniforms &u [[buffer(0)]],
                                depth2d<float,access::read> depth [[texture(0)]],texture3d<float> noise [[texture(1)]],
                                texture2d<float> smokeSprite [[texture(2)]],texture2d<float> scene [[texture(3)]]) {
    uint2 location=min(uint2(pixel.xy),uint2(depth.get_width()-1,depth.get_height()-1));
    float sceneDepth=depth.read(location);
    float3 ray=normalize(phonkNuclearUnproject(pixel.xy,1.f,u)-u.eye.xyz);
    float stop=sceneDepth<1.f?max(0.f,dot(phonkNuclearUnproject(pixel.xy,sceneDepth,u)-u.eye.xyz,ray)):1.e8f;
    float radius=u.sourceRadius.w;
    float3 origin=(u.eye.xyz-u.sourceRadius.xyz)/radius;
    // sourceRadius is camera-rebased: it must never seed a blast's identity.
    float seed=shipFireHash(u.params.w*float3(.13f,.71f,.37f));
    float4 variation=fract(sin(float4(seed,seed+1.3f,seed+2.7f,seed+4.1f))*43758.5453f);
    PhonkPressureSample pressure={};float4 ground=0;
    float fade=smoothstep(0.f,.12f,u.params.x)*(1.f-smoothstep(max(0.f,u.params.z-.5f),u.params.z,u.params.x));
    if(u.params.x<u.params.z) {
        // Ordinary depth clips the world volume behind land and ships;
        // never extrude a second painted field over their visible slopes.
        pressure=phonkPressureVolume(origin,ray,stop/radius,u.params.y,u.eye.w,u.params.x,variation,noise);
        ground=pressure.haze*fade;
        pressure.bend.w/=max(1.e-5f,pressure.haze.a);
        pressure.bend.xyz*=fade;
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
    if(far<=near || age>=9.f)return phonkPressureComposite(mix(ground,sprite+ground*(1.f-sprite.a),faceAmount),
        pressure.haze.a*fade>.001f?pressure.bend:float4(0),pixel.xy,ray,u,depth,scene);
    constexpr uint steps=64;
    float step=(far-near)/float(steps),transmission=1.f;
    float3 color=0;
    for(uint i=0;i<steps;i++) {
        float distance=near+(float(i)+.5f)*step;
        float3 point=origin+ray*distance;
        float2 field=phonkNuclearField(point,u.params.x,variation,noise);
        float opacity=1.f-exp(-field.x*step*6.f);
        if(opacity>.001f) {
            // Two bounded optical-depth taps, following the volume-fire model.
            float3 light=float3(-.48f,.81f,-.32f);
            float shadow=phonkNuclearField(point+light*.18f,u.params.x,variation,noise).x*.18f
                        +phonkNuclearField(point+light*.48f,u.params.x,variation,noise).x*.30f;
            float lighting=.20f+.66f*exp(-shadow*3.5f)+.14f*exp(-shadow*.9f);
            float3 soot=float3(.30f,.285f,.26f)*lighting;
            float fire=smoothstep(.04f,.65f,field.y);
            float3 emission=mix(float3(1.8f,.19f,.014f),float3(4.4f,2.5f,.7f),fire);
            color+=transmission*opacity*(soot+emission*pow(field.y,1.7f)*3.f);
        }
        transmission*=1.f-opacity;
        if(transmission<.015f)break;
    }
    float4 volume=float4(color+ground.rgb*transmission,1.f-transmission*(1.f-ground.a));
    return phonkPressureComposite(mix(volume,sprite+ground*(1.f-sprite.a),faceAmount),
        pressure.haze.a*fade>.001f?pressure.bend:float4(0),pixel.xy,ray,u,depth,scene);
}
)MSL";
}
