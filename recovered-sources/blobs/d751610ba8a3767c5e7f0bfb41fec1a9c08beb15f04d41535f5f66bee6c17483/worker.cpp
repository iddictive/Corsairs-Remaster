#include "current-helpers.hpp"
#include <future>
void continueDose(storm_world_cut::Cut &next,const storm_world_cut::Cut &prior) {
 storm_ship_laser::continueBeamDose(next.beams,prior.beams);
}
int main(){
 auto original=std::make_shared<const Mesh>(box());Cut first;first.kind=Cut::Beam;first.field=1;storm_hull_damage::BeamSample b;b.start={0,0,-1};b.direction={0,0,1};b.depth=.3;first.beams={b};
 Input input;input.original=original;input.active=true;input.cuts={first};auto future=std::async(std::launch::async,[input]{Projection result;assert(prepare(input,result));std::vector<CapTriangle>accepted;assert(finishCoverage(result,{},accepted));return result;});
 assert(ray(*original,{0,0,-2},{0,0,2})==.25);auto prior=future.get();assert(std::abs(ray(prior.retained,{0,0,-2},{0,0,2})-.325)<1.e-6);assert(!prior.references.empty());
 auto second=first;second.field=2;continueDose(second,first);assert(std::abs(second.beams[0].start[2]+.7)<1e-6);input.cuts.push_back(second);Projection twice;assert(prepare(input,twice));std::vector<CapTriangle>accepted;assert(finishCoverage(twice,{},accepted));assert(std::abs(ray(twice.retained,{0,0,-2},{0,0,2})-.4)<1e-6);
 auto third=first;continueDose(third,first);continueDose(third,second);assert(std::abs(third.beams[0].start[2]+.4)<1e-6);
 auto swept=first;swept.beams[0].start[0]=.5;continueDose(swept,first);assert(swept.beams[0].start[2]==-1);input.cuts={first,swept};Projection gap;assert(prepare(input,gap));assert(ray(gap.retained,{.25f,0,-2},{.25f,0,2})==.25);
 auto invalid=input;invalid.cuts[0].beams[0].depth=NAN;auto before=twice.retained;assert(!prepare(invalid,twice));assert(sameMesh(before,twice.retained));assert(prepare(input,twice));
 input.cuts={first};Projection caps;assert(prepare(input,caps));std::vector<CapTriangle>raw;assert(finishCoverage(caps,{},raw));auto after=caps.retained;assert(!finishCoverage(caps,std::vector<CapTriangle>(1000001),accepted));assert(sameMesh(after,caps.retained));assert(finishCoverage(caps,{},accepted));
 std::cout<<"async immutable snapshot / prior retained / cumulative2+3 dose / sweep gap / invalid prior rollback / subsequent valid / coverage reject recovery PASS"<<std::endl;
}
