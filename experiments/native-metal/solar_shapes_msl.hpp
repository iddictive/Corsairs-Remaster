#pragma once

namespace storm_metal {
inline constexpr const char *solarShapesMSL=R"MSL(
struct SolarShapeU {float4 cameraRight_shape,cameraUp_extent;};
float4 solarShapeColor(float2 uv,float3 tint,constant SolarShapeU&shape,float4 directionBlend,
                       texture2d<float>cloudFrom,texture2d<float>cloudTo,texture2d<float>transmittance){
    float2 p=uv*2.f-1.f;
    float3 source=normalize(directionBlend.xyz);
    float3 ray=normalize(source+shape.cameraUp_extent.w*
                        (p.x*shape.cameraRight_shape.xyz-p.y*shape.cameraUp_extent.xyz));
    float cosine=clamp(dot(source,ray),-1.f,1.f);
    float angle=atan2(length(cross(source,ray)),cosine);
    float3 spectrum=skySolarSpectrum(max(ray.y,0.f),transmittance);
    float coverage;
    if(shape.cameraRight_shape.w<1.5f){
        float r=angle/skySolarAngularRadius;
        float edge=max(fwidth(r),.0001f);
        float disc=1.f-smoothstep(1.f-edge,1.f+edge,r);
        coverage=disc*(.6f+.4f*sqrt(saturate(1.f-r*r)));
    }else{
        // Filament's skybox.mat uses angular power falloff for its source halo:
        // https://github.com/google/filament/blob/main/filament/src/materials/skybox.mat
        // Evaluate optical spread here, before the LDR target clips the sun;
        // the existing interior-only bloom cannot recover that source energy.
        // Optical spread retains the authored source's apparent footprint;
        // neither lobe changes the photosphere or atmospheric spectrum.
        float radius=angle/skySolarAngularRadius;
        float halo=pow(saturate(1.f-radius*radius/(30.f*30.f)),4.f);
        float optical=exp(-.5f*radius*radius/(4.f*4.f));
        coverage=(1.05f*optical+.22f*halo)*(1.f-smoothstep(.8f,1.f,length(p)));
    }
    // Resolve cloud cover per source pixel. A cloud edge may cover part of the
    // photosphere while the exposed portion retains the same atmospheric tint.
    float visibility=saturate(1.f-skyCloudMoments(ray,cloudFrom,cloudTo,directionBlend.w).a);
    coverage*=visibility;
    return float4(tint*pow(max(spectrum,0.f),float3(1.f/2.2f))*coverage,coverage);
}
)MSL";
}
