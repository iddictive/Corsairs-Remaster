#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <array>
#include <cstdio>
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
    // Empty preserves the laser's regular octagon. Mechanical fields carry
    // their real convex contact patch in tangent/bitangent metres.
    std::vector<std::array<float,2>> crossSection;
    float backDepth=-1.f; // negative keeps the historical laser entry overlap
    float specificWork=0.f,contactStrength=0.f; // existing TOUCH whole-hull work domain
};
inline bool validCrossSection(const BeamSample&b){
    if(!std::isfinite(b.backDepth)||b.backDepth< -1.f||!std::isfinite(b.specificWork)||b.specificWork<0||
       !std::isfinite(b.contactStrength)||b.contactStrength<0||(b.specificWork>0&&b.contactStrength<=0))return false;
    if(b.crossSection.empty())return true;
    if(b.crossSection.size()<3||b.crossSection.size()>8)return false;
    double area=0;
    for(size_t n=0;n<b.crossSection.size();++n){const auto&a=b.crossSection[n],&c=b.crossSection[(n+1)%b.crossSection.size()],&d=b.crossSection[(n+2)%b.crossSection.size()];
        if(!std::isfinite(a[0])||!std::isfinite(a[1]))return false;
        area+=double(a[0])*c[1]-double(a[1])*c[0];
        if((double(c[0])-a[0])*(d[1]-c[1])-(double(c[1])-a[1])*(d[0]-c[0])< -1.e-9)return false;}
    return area>1.e-8;
}
inline constexpr unsigned MaxBeamSamples=16;
// Keep the former 32x16 finite-sample ceiling, without charging a one-sample
// dose as a whole sixteen-sample record. Legacy split planes keep their32 cap.
inline constexpr unsigned MaxCutRecords=MaxSplitPlanes*MaxBeamSamples;
inline unsigned cutRecordCost(std::size_t samples) {
    return samples ? (samples<=MaxBeamSamples ? unsigned(samples) : MaxCutRecords+1) : MaxBeamSamples;
}
template<class Planes> unsigned cutHistoryCost(const Planes&planes) {
    unsigned cost=0;
    for(const auto&plane:planes){cost+=cutRecordCost(plane.beams.size());if(cost>MaxCutRecords)return MaxCutRecords+1;}
    return cost;
}
template<class Planes> bool canAppendCut(const Planes&planes,std::size_t samples) {
    return planes.size()<MaxCutRecords&&cutHistoryCost(planes)+cutRecordCost(samples)<=MaxCutRecords;
}
// Admission limits are physical metres, before receiver-local conversion.
// Geometry validates local positivity separately: a scaled node can require
// local dimensions outside these physical input limits.
inline constexpr float MinBeamRadius=.01f,MaxBeamRadius=2.f,MinBeamDepth=.001f,MaxBeamDepth=1000.f;
inline bool validPhysicalBeamDimensions(float radius,float depth){
    return std::isfinite(radius)&&radius>=MinBeamRadius&&radius<=MaxBeamRadius&&
        std::isfinite(depth)&&depth>=MinBeamDepth&&depth<=MaxBeamDepth;
}
inline bool validPhysicalBeam(const BeamSample&beam){
    if(!validCrossSection(beam)||!std::isfinite(beam.depth)||beam.depth<MinBeamDepth||beam.depth>MaxBeamDepth||!std::isfinite(beam.radius)||beam.radius<MinBeamRadius)return false;
    if(beam.crossSection.empty())return beam.radius<=MaxBeamRadius;
    // Explicit mechanical contact patches carry their actual root-hull extent;
    // the laser octagon's radius is not their physical footprint authority.
    for(const auto&p:beam.crossSection)if(std::abs(p[0])>MaxBeamDepth||std::abs(p[1])>MaxBeamDepth)return false;return true;
}
// Ship SAVE, island SAVE and the crew consumer share this exact sample schema.
enum class BeamCoordinates { Physical,NodeLocal };
template<class Attributes> bool readBeamSamples(Attributes *entry,std::vector<BeamSample>&output,BeamCoordinates coordinates=BeamCoordinates::Physical) {
    const auto count=entry->GetAttributeAsDword("samples");if(count>MaxBeamSamples)return false;
    std::vector<BeamSample> samples;
    for(unsigned n=0;n<count;++n){auto *sample=entry->GetAttributeClass("b"+std::to_string(n));if(!sample)return false;
        BeamSample beam;
        beam.start={sample->GetAttributeAsFloat("x"),sample->GetAttributeAsFloat("y"),sample->GetAttributeAsFloat("z")};
        beam.direction={sample->GetAttributeAsFloat("dx"),sample->GetAttributeAsFloat("dy"),sample->GetAttributeAsFloat("dz")};
        beam.tangent={sample->GetAttributeAsFloat("tx"),sample->GetAttributeAsFloat("ty"),sample->GetAttributeAsFloat("tz")};
        beam.radius=sample->GetAttributeAsFloat("radius");beam.depth=sample->GetAttributeAsFloat("depth");
        beam.backDepth=sample->GetAttributeAsFloat("back",-1.f);const auto corners=sample->GetAttributeAsDword("corners");if(corners>8)return false;
        beam.specificWork=sample->GetAttributeAsFloat("work");beam.contactStrength=sample->GetAttributeAsFloat("strength");
        for(unsigned c=0;c<corners;++c){auto*p=sample->GetAttributeClass("p"+std::to_string(c));if(!p)return false;beam.crossSection.push_back({p->GetAttributeAsFloat("x"),p->GetAttributeAsFloat("y")});}
        if(coordinates==BeamCoordinates::Physical?!validPhysicalBeam(beam):!std::isfinite(beam.radius)||beam.radius<=0||!std::isfinite(beam.depth)||beam.depth<=0||!validCrossSection(beam))return false;samples.push_back(beam);}
    output=std::move(samples);return true;
}
template<class Attributes> void writeFloat(Attributes*entry,const char*name,float value){char text[32];std::snprintf(text,sizeof(text),"%.9g",double(value));entry->SetAttribute(name,text);}
template<class Attributes> void writeBeamSamples(Attributes *entry,const std::vector<BeamSample>&samples) {
    entry->SetAttributeUseDword("samples",unsigned(samples.size()));
    for(unsigned n=0;n<samples.size();++n){const auto&b=samples[n];auto *sample=entry->VerifyAttributeClass("b"+std::to_string(n));
        writeFloat(sample,"x",b.start[0]);writeFloat(sample,"y",b.start[1]);writeFloat(sample,"z",b.start[2]);
        writeFloat(sample,"dx",b.direction[0]);writeFloat(sample,"dy",b.direction[1]);writeFloat(sample,"dz",b.direction[2]);
        writeFloat(sample,"tx",b.tangent[0]);writeFloat(sample,"ty",b.tangent[1]);writeFloat(sample,"tz",b.tangent[2]);
        writeFloat(sample,"radius",b.radius);writeFloat(sample,"depth",b.depth);
        writeFloat(sample,"back",b.backDepth);sample->SetAttributeUseDword("corners",unsigned(b.crossSection.size()));
        writeFloat(sample,"work",b.specificWork);writeFloat(sample,"strength",b.contactStrength);
        for(unsigned c=0;c<b.crossSection.size();++c){auto*p=sample->VerifyAttributeClass("p"+std::to_string(c));writeFloat(p,"x",b.crossSection[c][0]);writeFloat(p,"y",b.crossSection[c][1]);}}
}
}
