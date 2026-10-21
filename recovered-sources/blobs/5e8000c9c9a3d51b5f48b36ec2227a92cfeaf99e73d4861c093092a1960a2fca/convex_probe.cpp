#ifndef CONVEX_HEADER
#define CONVEX_HEADER "ship_hull_convex_material.hpp"
#endif
#include CONVEX_HEADER
#include <chrono>
#include <cstdio>
using namespace storm::ship::material::convex;
int main(){auto material=std::make_shared<MaterialCellSource>();
 for(unsigned n=0;n<120;++n){MaterialPrism p;p.object=0;const float x=n<100?float(n)*.002f:10.f+float(n-100)*.002f;p.vertices={{{x,0,0},{x+1,0,0},{x,1,0},{x,0,.1f},{x+1,0,.1f},{x,1,.1f}}};Mesh m;Triangle t{};for(unsigned j=0;j<3;++j)t[j].position=p.vertices[j];m[0].push_back(t);material->prisms.push_back(p);material->solids.push_back(std::make_shared<const Mesh>(std::move(m)));material->bounds.push_back({{x,0,0},{x+1,1,.1f}});}
 material->index.build(material->bounds);
 auto layout=std::make_shared<storm_ship_structure::Layout>();for(unsigned n=0;n<4;++n){storm_ship_structure::Cell c;c.bounds={{float(n)*.35f,-1,-1},{float(n+1)*.35f,2,1}};c.area=1;c.volume=1;layout->cells.push_back(c);}layout->bounds={{0,-1,-1},{1.4f,2,1}};layout->divisions={4,1,1};layout->step={.35f,3,2};layout->gridToCell={0,1,2,3};
 for(int partitioned=0;partitioned<2;++partitioned){const auto start=std::chrono::steady_clock::now();auto s=buildSource(material,partitioned?layout:nullptr);if(!s)return 2;uint64_t hash=1469598103934665603ull;const auto word=[&](uint64_t n){hash^=n;hash*=1099511628211ull;};size_t parts=0;for(unsigned n=0;n<s->pieces.size();++n){word(s->owner[n]);for(const auto&p:*s->pieces[n]){++parts;word(p.supportCell);word(uint64_t(std::llround(p.volume*1.e12)));for(const auto&f:p.faces){word(f.partition);for(const auto&v:f.vertices){word(uint64_t(std::llround(v.position.x*1.e9)));word(uint64_t(std::llround(v.position.y*1.e9)));word(uint64_t(std::llround(v.position.z*1.e9)));}}}}for(const auto&ids:s->supportSources){word(ids.size());for(auto id:ids)word(id);}std::printf("partitioned=%d parts=%zu owner0=%u owner99=%u owner100=%u owner119=%u digest=%llu ms=%.3f\n",partitioned,parts,s->owner[0],s->owner[99],s->owner[100],s->owner[119],(unsigned long long)hash,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());if(!partitioned&&(s->owner[0]!=s->owner[99]||s->owner[0]==s->owner[100]||s->owner[100]!=s->owner[119]))return 3;}
}
