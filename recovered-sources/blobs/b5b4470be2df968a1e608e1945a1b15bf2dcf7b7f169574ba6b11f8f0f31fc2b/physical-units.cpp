#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/ship_laser_geometry.hpp"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
using storm_hull_damage::BeamSample;
struct Attributes {
    unsigned samples=0;
    std::map<std::string,float>values;
    std::map<std::string,Attributes>children;
    unsigned GetAttributeAsDword(const char*){return samples;}
    float GetAttributeAsFloat(const char*key){return values[key];}
    Attributes*GetAttributeClass(const std::string&key){auto p=children.find(key);return p==children.end()?nullptr:&p->second;}
};
void saved(Attributes&entry,unsigned index,float radius,float depth){auto&b=entry.children["b"+std::to_string(index)];b.values={{"radius",radius},{"depth",depth},{"dz",-1}};entry.samples=index+1;}
int main(){BeamSample physical;physical.direction={0,0,-1};physical.radius=2;physical.depth=1000;
#ifndef PHYSICAL_UNITS_FIXED
    auto local=physical;local.radius=std::nextafter(2.f,3.f);assert(!storm_ship_laser::valid(local));
    local=physical;local.radius/=.5f;local.depth/=.5f;assert(!storm_ship_laser::valid(local));
    std::puts("REPRODUCED: legal physical max rejects after near-one or .5 node scale.");
#else
    using namespace storm_hull_damage;
    assert(validPhysicalBeamDimensions(MinBeamRadius,MinBeamDepth));assert(validPhysicalBeamDimensions(MaxBeamRadius,MaxBeamDepth));
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    for(float radius:{0.f,-1.f,nan,inf,std::nextafter(MinBeamRadius,0.f),std::nextafter(MaxBeamRadius,inf)})assert(!validPhysicalBeamDimensions(radius,.3f));
    for(float depth:{0.f,-1.f,nan,inf,std::nextafter(MinBeamDepth,0.f),std::nextafter(MaxBeamDepth,inf)})assert(!validPhysicalBeamDimensions(.16f,depth));
    for(float scale:{.5f,2.f,std::nextafter(1.f,0.f),std::nextafter(1.f,2.f)})for(bool low:{false,true}){auto beam=physical;beam.radius=(low?MinBeamRadius:MaxBeamRadius)/scale;beam.depth=(low?MinBeamDepth:MaxBeamDepth)/scale;assert(storm_ship_laser::valid(beam));}
    auto local=physical;local.radius=std::nextafter(2.f,3.f);assert(storm_ship_laser::valid(local));
    for(float value:{0.f,-1.f,nan,inf}){local=physical;local.radius=value;assert(!storm_ship_laser::valid(local));local=physical;local.depth=value;assert(!storm_ship_laser::valid(local));}
    local=physical;local.direction={0,0,-2};assert(!storm_ship_laser::valid(local));local=physical;local.tangent={0,0,-1};assert(!storm_ship_laser::valid(local));
    Attributes entry;saved(entry,0,.16f,.3f);std::vector<BeamSample>out;assert(readBeamSamples(&entry,out)&&out.size()==1);const auto prior=out;
    const auto unchanged=[&]{assert(out.size()==prior.size()&&out[0].radius==prior[0].radius&&out[0].depth==prior[0].depth);};
    for(float value:{0.f,-1.f,nan,inf,std::nextafter(MinBeamRadius,0.f),std::nextafter(MaxBeamRadius,inf)}){saved(entry,1,value,.3f);assert(!readBeamSamples(&entry,out));unchanged();}
    for(float value:{0.f,-1.f,nan,inf,std::nextafter(MinBeamDepth,0.f),std::nextafter(MaxBeamDepth,inf)}){saved(entry,1,.16f,value);assert(!readBeamSamples(&entry,out));unchanged();}
    saved(entry,1,MaxBeamRadius,MaxBeamDepth);assert(readBeamSamples(&entry,out)&&out.size()==2);
    std::puts("PASS physical inclusive boundaries/next-float rejects/NaN/Inf, local scales .5/2/near-one at physical min+max, local positive-only dimensions and frame guards, invalid SAVE dimensions preserve prior output atomically.");
#endif
}
