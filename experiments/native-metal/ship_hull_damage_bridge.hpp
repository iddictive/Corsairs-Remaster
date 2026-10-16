#pragma once
#include <cstdint>
#include <cmath>
#include <array>
#include <string>
#include <vector>

namespace storm_hull_damage {
// Native accepted-hit message; the existing MSG_BLOTS_HIT remains a decal.
// lffffffslffll: code, world position xyz, outward world normal xyz,
// exact traced node name (empty means root), object index, world radius,
// confirmed HP loss, ship ExecuteLayer, ship RealizeLayer.
// Capture node/triangle before the script callback; send only after HP admission.
inline constexpr int32_t HullHitMessage=0x53484431;
inline constexpr int32_t ChipBurstMessage=0x53484331;
inline constexpr float MinHitRadius=.08f,MaxHitRadius=.36f,MaxMergedRadius=.36f;
inline constexpr unsigned MaxSplitPlanes=32,MaxFragmentGroups=8;
inline constexpr unsigned MaxFragmentBuffers=MaxSplitPlanes*MaxFragmentGroups;
// A finite dose delivered by the muzzle ray. Coordinates belong to the receiver.
struct BeamSample {
    std::array<float,3> start{}, direction{}, tangent{};
    float radius=.16f, depth=0.f;
};
inline constexpr unsigned MaxBeamSamples=16;
// Admission limits are physical metres, before receiver-local conversion.
// Geometry validates local positivity separately: a scaled node can require
// local dimensions outside these physical input limits.
inline constexpr float MinBeamRadius=.01f,MaxBeamRadius=2.f,MinBeamDepth=.001f,MaxBeamDepth=1000.f;
inline bool validPhysicalBeamDimensions(float radius,float depth){
    return std::isfinite(radius)&&radius>=MinBeamRadius&&radius<=MaxBeamRadius&&
        std::isfinite(depth)&&depth>=MinBeamDepth&&depth<=MaxBeamDepth;
}
// Ship SAVE, island SAVE and the crew consumer share this exact sample schema.
template<class Attributes> bool readBeamSamples(Attributes *entry,std::vector<BeamSample>&output) {
    const auto count=entry->GetAttributeAsDword("samples");if(count>MaxBeamSamples)return false;
    std::vector<BeamSample> samples;
    for(unsigned n=0;n<count;++n){auto *sample=entry->GetAttributeClass("b"+std::to_string(n));if(!sample)return false;
        BeamSample beam;
        beam.start={sample->GetAttributeAsFloat("x"),sample->GetAttributeAsFloat("y"),sample->GetAttributeAsFloat("z")};
        beam.direction={sample->GetAttributeAsFloat("dx"),sample->GetAttributeAsFloat("dy"),sample->GetAttributeAsFloat("dz")};
        beam.tangent={sample->GetAttributeAsFloat("tx"),sample->GetAttributeAsFloat("ty"),sample->GetAttributeAsFloat("tz")};
        beam.radius=sample->GetAttributeAsFloat("radius");beam.depth=sample->GetAttributeAsFloat("depth");
        if(!validPhysicalBeamDimensions(beam.radius,beam.depth))return false;samples.push_back(beam);}
    output=std::move(samples);return true;
}
template<class Attributes> void writeBeamSamples(Attributes *entry,const std::vector<BeamSample>&samples) {
    entry->SetAttributeUseDword("samples",unsigned(samples.size()));
    for(unsigned n=0;n<samples.size();++n){const auto&b=samples[n];auto *sample=entry->VerifyAttributeClass("b"+std::to_string(n));
        sample->SetAttributeUseFloat("x",b.start[0]);sample->SetAttributeUseFloat("y",b.start[1]);sample->SetAttributeUseFloat("z",b.start[2]);
        sample->SetAttributeUseFloat("dx",b.direction[0]);sample->SetAttributeUseFloat("dy",b.direction[1]);sample->SetAttributeUseFloat("dz",b.direction[2]);
        sample->SetAttributeUseFloat("tx",b.tangent[0]);sample->SetAttributeUseFloat("ty",b.tangent[1]);sample->SetAttributeUseFloat("tz",b.tangent[2]);
        sample->SetAttributeUseFloat("radius",b.radius);sample->SetAttributeUseFloat("depth",b.depth);}
}
}
